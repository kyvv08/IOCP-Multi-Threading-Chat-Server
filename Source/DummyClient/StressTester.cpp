#include "StressTester.h"
#include "Monitoring/LatencyMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

void StressTester::RunScenario(BotScenario scenario, int32 botCount, int32 durationSec, const std::wstring& ip, uint16 port)
{
	StressStats::Reset();
	LatencyMonitor::Reset();

	std::cout << "\n\033[1;36m========================================================================================\033[0m\n";
	std::cout << "\033[1;37m                 STARTING STRESS TEST SCENARIO: " << GetScenarioName(scenario) << "\033[0m\n";
	std::string ipStr;
	for (wchar_t c : ip)
		ipStr += static_cast<char>(c);

	std::cout << " [Configuration] Target: " << ipStr << ":" << port
		<< " | Bot Count: " << botCount
		<< " | Duration: " << durationSec << "s\n\n";

	std::shared_ptr<IocpCore> iocpCore = std::make_shared<IocpCore>();
	std::vector<DummySessionRef> sessions;
	sessions.reserve(botCount);

	std::atomic<int32> botIndexGen{ 1 };

	ClientServiceRef clientService = std::make_shared<ClientService>(
		NetAddress(ip, port),
		iocpCore,
		[&sessions, scenario, &botIndexGen]()
		{
			int32 idx = botIndexGen.fetch_add(1);
			DummySessionRef session = std::make_shared<DummySession>(scenario, idx);
			sessions.push_back(session);
			return session;
		},
		botCount
	);

	if (false == clientService->Start())
	{
		std::cerr << "\033[1;31m[ERROR] Failed to start ClientService for Stress Test!\033[0m\n";
		return;
	}

	std::atomic<bool> isRunning{ true };

	// 1. IOCP 네트워크 스레드 가동
	const uint32 threadCount = 4;
	std::vector<std::thread> workerThreads;
	for (uint32 i = 0; i < threadCount; ++i)
	{
		workerThreads.emplace_back([&iocpCore, &isRunning]()
		{
			PoolAllocator::Init();
			while (isRunning.load())
			{
				iocpCore->Dispatch(10);
			}
		});
	}

	// 2. 주기적 봇 상호작용 및 진행 상태 모니터링 루프
	auto startTime = std::chrono::steady_clock::now();

	for (int32 sec = 1; sec <= durationSec; ++sec)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		// 정상 봇인 경우 주기적 채팅 및 하트비트 전송
		if (scenario == BotScenario::Normal)
		{
			for (auto& session : sessions)
			{
				if (session && session->IsConnected())
				{
					if (sec % 1 == 0)
						session->SendLatencyPingChat();
					if (rand() % 3 == 0)
						session->SendHeartbeat();
					if (sec % 10 == 0) 
						session->SendEnterRoom("", rand() % 100 + 1);
				}
			}
		}

		RenderProgress(scenario, botCount, sec, durationSec);
	}

	isRunning.store(false);
	clientService->Stop();

	for (auto& w : workerThreads)
	{
		if (w.joinable())
			w.join();
	}

	// 3. 최종 결과 검증 및 리포트 출력
	std::cout << "\n\n\033[1;36m========================================================================================\033[0m\n";
	std::cout << "\033[1;37m                               STRESS TEST RESULT SUMMARY                               \033[0m\n";
	std::cout << "\033[1;36m========================================================================================\033[0m\n";
	std::cout << "  * Target Scenario           : " << GetScenarioName(scenario) << "\n";
	std::cout << "  * Total Bots Spawned        : " << botCount << "\n";
	std::cout << "  * Total Connected Sessions  : " << StressStats::totalConnected.load() << "\n";
	std::cout << "  * Total Disconnected        : " << StressStats::totalDisconnected.load() << "\n";
	std::cout << "  * Abuse Forced Disconnects  : " << StressStats::abuseDisconnected.load() << "\n";
	std::cout << "  * Total Packets Sent / Recv : " << StressStats::totalSentPackets.load() << " / " << StressStats::totalRecvPackets.load() << "\n";

	if (scenario == BotScenario::Normal && LatencyMonitor::GetTotalCount() > 0)
	{
		std::cout << "----------------------------------------------------------------------------------------\n";
		std::cout << "\033[1;35m  [Latency (RTT) Distribution & Statistics (Total Samples: " << LatencyMonitor::GetTotalCount() << ")]\033[0m\n";
		std::cout << "   * Average (Mean) RTT     : \033[1;32m" << std::fixed << std::setprecision(2) << LatencyMonitor::GetAverageMs() << " ms\033[0m\n";
		std::cout << "   * Minimum RTT            : " << std::fixed << std::setprecision(2) << LatencyMonitor::GetMinMs() << " ms\n";
		std::cout << "   * 50th Percentile (P50)  : " << std::fixed << std::setprecision(2) << LatencyMonitor::GetPercentileMs(50.0) << " ms (Median)\n";
		std::cout << "   * 90th Percentile (P90)  : " << std::fixed << std::setprecision(2) << LatencyMonitor::GetPercentileMs(90.0) << " ms\n";
		std::cout << "   * 95th Percentile (P95)  : \033[1;33m" << std::fixed << std::setprecision(2) << LatencyMonitor::GetPercentileMs(95.0) << " ms\033[0m\n";
		std::cout << "   * 99th Percentile (P99)  : \033[1;31m" << std::fixed << std::setprecision(2) << LatencyMonitor::GetPercentileMs(99.0) << " ms (Tail Latency)\033[0m\n";
		std::cout << "   * Maximum RTT            : " << std::fixed << std::setprecision(2) << LatencyMonitor::GetMaxMs() << " ms\n";
	}
	std::cout << "----------------------------------------------------------------------------------------\n";

	bool testPassed = false;
	switch (scenario)
	{
	case BotScenario::Normal:
		testPassed = (StressStats::totalConnected.load() > 0 && StressStats::totalSentPackets.load() > 0);
		break;
	case BotScenario::Slowloris:
		testPassed = (StressStats::totalConnected.load() > 0);
		break;
	case BotScenario::ConnectSpam:
		testPassed = (StressStats::totalConnected.load() == botCount && StressStats::totalDisconnected.load() == botCount);
		break;
	case BotScenario::PacketFlood:
		testPassed = (StressStats::abuseDisconnected.load() > 0 || StressStats::totalDisconnected.load() > 0);
		break;
	}

	if (testPassed)
	{
		std::cout << "  * \033[1;32mVERDICT: [PASS] System behaved according to expected security & capacity criteria.\033[0m\n";
	}
	else
	{
		std::cout << "  * \033[1;31mVERDICT: [FAIL] Scenario expectations were not fully met.\033[0m\n";
	}
	std::cout << "\033[1;36m========================================================================================\033[0m\n\n";
}

std::string StressTester::GetScenarioName(BotScenario scenario)
{
	switch (scenario)
	{
	case BotScenario::Normal: return "Normal Chatting Load Test";
	case BotScenario::Slowloris: return "Slowloris / Idle Keep-Alive Timeout Test";
	case BotScenario::ConnectSpam: return "Connect / Disconnect Rapid Spam Stress Test";
	case BotScenario::PacketFlood: return "Packet Flooding Attack Defense Test";
	default: return "Unknown Scenario";
	}
}

void StressTester::RenderProgress(BotScenario scenario, int32 botCount, int32 elapsedSec, int32 totalSec)
{
	int32 active = StressStats::activeSessions.load();
	int32 disconnected = StressStats::totalDisconnected.load();
	int32 abuse = StressStats::abuseDisconnected.load();
	uint64 sent = StressStats::totalSentPackets.load();
	uint64 recv = StressStats::totalRecvPackets.load();

	uint64 latencySamples = LatencyMonitor::GetTotalCount();
	double avgLatency = LatencyMonitor::GetAverageMs();
	double p95Latency = LatencyMonitor::GetPercentileMs(95.0);
	double p99Latency = LatencyMonitor::GetPercentileMs(99.0);

	int32 progressPercent = (elapsedSec * 100) / totalSec;

	std::cout << "\r\033[1;33m[" << std::setw(3) << progressPercent << "%]\033[0m "
		<< "Active CCU: \033[1;32m" << std::setw(4) << active << "\033[0m | "
		<< "Sent/Recv: \033[1;36m" << sent << "/" << recv << "\033[0m | ";

	if (latencySamples > 0)
	{
		std::cout << "RTT(ms) Avg:\033[1;32m" << std::fixed << std::setprecision(1) << avgLatency << "\033[0m "
			<< "P95:\033[1;33m" << p95Latency << "\033[0m "
			<< "P99:\033[1;31m" << p99Latency << "\033[0m "
			<< "(N=" << latencySamples << ") ";
	}
	else
	{
		std::cout << "Disconns:\033[1;37m" << disconnected << "\033[0m | Abuse:\033[1;31m" << abuse << "\033[0m ";
	}

	std::cout << "(" << elapsedSec << "s/" << totalSec << "s)   " << std::flush;
}
