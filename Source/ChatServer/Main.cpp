#include "ServerEngine.h"
#include "Session/ClientSession.h"
#include "Room/RoomManager.h"
#include "Service/AbuseMonitor.h"
#include "Monitoring/ServerStats.h"
#include "Monitoring/ConsoleDashboard.h"
#include "Protocol/ServerPacketHandler.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <conio.h>

int main(int argc, char* argv[])
{
	uint16 serverPort = 7777;
	if (argc >= 2)
	{
		serverPort = static_cast<uint16>(std::stoi(argv[1]));
	}

	// 1. 코어 서브시스템 초기화
	SocketUtils::Init();
	PoolAllocator::Init();
	RoomManager::Init();
	AbuseMonitor::Init();
	ServerStats::Init();
	ConsoleDashboard::Init();
	ServerPacketHandler::Init();

	std::shared_ptr<IocpCore> iocpCore = std::make_shared<IocpCore>();

	// 2. 서버 네트워크 서비스 생성 (동적 포트 바인딩, 최대 10,000 세션)
	NetAddress listenAddr(L"0.0.0.0", serverPort);
	ServerServiceRef serverService = std::make_shared<ServerService>(
		listenAddr,
		iocpCore,
		[]() { return std::make_shared<ClientSession>(); },
		10000
	);

	if (false == serverService->Start())
	{
		std::cerr << "[ERROR] Failed to start ServerService on port " << serverPort << "!" << std::endl;
		return 1;
	}

	std::atomic<bool> isRunning{ true };

	// 3. IOCP 및 JobQueue 병합 워커 스레드 가동
	const uint32 threadCount = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() * 2 : 4;
	std::vector<std::thread> workerThreads;

	for (uint32 i = 0; i < threadCount; ++i)
	{
		workerThreads.emplace_back([&iocpCore, &isRunning]()
		{
			PoolAllocator::Init();

			while (isRunning.load())
			{
				// 1) IOCP 비동기 완료 이벤트 디스패치 (10ms 타임아웃)
				iocpCore->Dispatch(10);

				// 2) Actor JobQueue에 등록된 룸 일감들 분산 처리
				while (JobQueueRef jobQueue = GlobalQueue::Pop())
				{
					jobQueue->Execute();
				}
			}
		});
	}

	// 4. 타이머 및 하트비트/타임아웃 감시 백그라운드 스레드
	std::thread timerThread([&serverService, &isRunning]()
	{
		PoolAllocator::Init();
		uint64 lastCheckTick = ::GetTickCount64();

		while (isRunning.load())
		{
			const uint64 now = ::GetTickCount64();

			// 1) 지연/예약 태스크 분배
			JobTimer::Distribute(now);

			// 2) 1초 주기로 하트비트 Keep-Alive 타임아웃 검사 (15초 무응답 시 Disconnect)
			if (now - lastCheckTick >= 1000)
			{
				AbuseMonitor::CheckHeartbeatTimeouts(serverService, TIME_OUT_MS);
				lastCheckTick = now;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	});

	// 초기 화면 클리어
	::system("cls");

	uint64 lastRenderTick = 0;

	// 5. 실시간 TUI 모니터링 대시보드 렌더링 및 키보드 인터랙션 루프
	while (isRunning.load())
	{
		const uint64 now = ::GetTickCount64();

		// 1초 주기로 실시간 TUI 대시보드 갱신
		if (now - lastRenderTick >= 1000)
		{
			ConsoleDashboard::Render(serverService);
			lastRenderTick = now;
		}

		if (_kbhit())
		{
			int key = _getch();
			if (key == 'q' || key == 'Q')
			{
				std::cout << "\n\033[1;33m[INFO] Graceful Server Shutdown Initiated...\033[0m\n";
				isRunning.store(false);
				break;
			}
			else if (key == 's' || key == 'S')
			{
				if (AbuseMonitor::DumpLogsToFile("Abuse_Disconnect_Log.txt"))
				{
					std::cout << "\n\033[1;32m[LOG DUMP] Successfully saved 'Abuse_Disconnect_Log.txt' (Total: "
						<< AbuseMonitor::GetAbuseCount() << " entries)\033[0m\n";
					std::this_thread::sleep_for(std::chrono::milliseconds(1500));
					::system("cls");
				}
			}
			else if (key == 'c' || key == 'C')
			{
				::system("cls");
				ConsoleDashboard::Render(serverService);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// 6. 안전한 서비스 종료 및 리소스 해제
	serverService->Stop();

	if (timerThread.joinable())
		timerThread.join();

	for (auto& worker : workerThreads)
	{
		if (worker.joinable())
			worker.join();
	}

	// 최종 종료 시 잔여 어뷰징 로그 자동 파일 덤프
	AbuseMonitor::DumpLogsToFile("Abuse_Disconnect_Log.txt");

	SocketUtils::Clear();
	PoolAllocator::Clear();

	std::cout << "\033[1;32m[INFO] Server shutdown completed cleanly.\033[0m" << std::endl;
	return 0;
}
