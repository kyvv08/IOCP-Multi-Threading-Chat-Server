#include "Service/AbuseMonitor.h"
#include "Monitoring/ServerStats.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>

SpinLock AbuseMonitor::_lock;
std::vector<AbuseLogEntry> AbuseMonitor::_logs;
std::atomic<uint32> AbuseMonitor::_abuseCount{ 0 };

void AbuseMonitor::Init()
{
	SpinLockGuard guard(_lock);
	_logs.clear();
	_abuseCount.store(0);
}

void AbuseMonitor::ReportAbuse(uint64 sessionId, const std::string& playerName, const std::string& ipAddress, uint16 port, const std::wstring& reason)
{
	AbuseLogEntry entry;
	entry.sessionId = sessionId;
	entry.playerName = playerName.empty() ? "(Unauth)" : playerName;
	entry.ipAddress = ipAddress.empty() ? "0.0.0.0" : ipAddress;
	entry.port = port;
	entry.reason = reason;
	entry.timestamp = ::GetTickCount64();
	entry.timeStr = GetCurrentTimeString();

	_abuseCount.fetch_add(1, std::memory_order_relaxed);
	ServerStats::OnAbuseDisconnection();

	{
		SpinLockGuard guard(_lock);
		_logs.push_back(entry);
	}
}

void AbuseMonitor::CheckHeartbeatTimeouts(ServerServiceRef service, uint64 timeoutMs)
{
	if (service == nullptr)
		return;

	const uint64 now = ::GetTickCount64();
	auto sessions = service->GetSessionsSnapshot();

	for (const auto& session : sessions)
	{
		if (!session->IsConnected())
			continue;

		const uint64 lastHeartbeat = session->GetLastHeartbeatTick();
		if (lastHeartbeat == 0)
			continue; // 아직 첫 하트비트 전 (연결 직후 유예)

		// [중요 방어] now 측정 직후 다른 워커 스레드에서 패킷을 수신하여 lastHeartbeat가 더 미래 시점인 경우
		// unsigned int 언더플로우(말도 안 되게 큰 값으로 치솟는 현상) 방지
		if (now <= lastHeartbeat)
			continue;

		if (now - lastHeartbeat > timeoutMs)
		{
			std::wstring ipW = session->GetNetAddress().GetIpAddress();
			std::string ip;
			for (wchar_t c : ipW)
				ip += static_cast<char>(c);
			uint16 port = session->GetNetAddress().GetPort();

			ReportAbuse(session->GetSessionId(), "", ip, port, L"Heartbeat Timeout (Keep-alive Failed)");
			session->Disconnect(L"Heartbeat Timeout");
		}
	}
}

std::vector<AbuseLogEntry> AbuseMonitor::GetLogs()
{
	SpinLockGuard guard(_lock);
	return _logs;
}

std::vector<AbuseLogEntry> AbuseMonitor::GetRecentLogs(size_t maxCount)
{
	SpinLockGuard guard(_lock);
	if (_logs.empty())
		return {};

	size_t count = (std::min)(maxCount, _logs.size());
	std::vector<AbuseLogEntry> recent(_logs.end() - count, _logs.end());
	return recent;
}

bool AbuseMonitor::DumpLogsToFile(const std::string& filePath)
{
	std::vector<AbuseLogEntry> logs = GetLogs();

	std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
	if (!outFile.is_open())
		return false;

	outFile << "========================================================================================\n";
	outFile << "                        Chat Server Abuse & Disconnect Log Dump                        \n";
	outFile << "                          Generated At: " << GetCurrentTimeString() << "\n";
	outFile << "========================================================================================\n";
	outFile << "Total Forced Disconnections: " << logs.size() << "\n\n";

	outFile << std::left << std::setw(22) << "[Time]"
		<< std::setw(14) << "[Session ID]"
		<< std::setw(18) << "[Player Name]"
		<< std::setw(24) << "[Client IP:Port]"
		<< "[Reason]\n";
	outFile << "----------------------------------------------------------------------------------------\n";

	for (const auto& log : logs)
	{
		std::string reasonStr;
		for (wchar_t wc : log.reason)
			reasonStr += static_cast<char>(wc);

		std::string endpoint = log.ipAddress + ":" + std::to_string(log.port);

		outFile << std::left << std::setw(22) << log.timeStr
			<< std::setw(14) << log.sessionId
			<< std::setw(18) << log.playerName
			<< std::setw(24) << endpoint
			<< reasonStr << "\n";
	}

	outFile << "========================================================================================\n";
	outFile.close();
	return true;
}

std::string AbuseMonitor::GetCurrentTimeString()
{
	auto now = std::chrono::system_clock::now();
	std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

	std::tm buf;
	localtime_s(&buf, &in_time_t);

	std::stringstream ss;
	ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}
