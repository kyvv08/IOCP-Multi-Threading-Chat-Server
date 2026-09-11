#include "Monitoring/ConsoleDashboard.h"
#include "Monitoring/ServerStats.h"
#include "Service/AbuseMonitor.h"
#include "Room/RoomManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>

#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_CYAN       "\033[1;36m"
#define ANSI_GREEN      "\033[1;32m"
#define ANSI_YELLOW     "\033[1;33m"
#define ANSI_RED        "\033[1;31m"
#define ANSI_BLUE       "\033[1;34m"
#define ANSI_MAGENTA    "\033[1;35m"
#define ANSI_WHITE      "\033[1;37m"
#define ANSI_BG_DARK    "\033[40m"

void ConsoleDashboard::Init()
{
	EnableAnsiEscapeCodes();
}

void ConsoleDashboard::EnableAnsiEscapeCodes()
{
	HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return;

	DWORD dwMode = 0;
	if (!::GetConsoleMode(hOut, &dwMode)) return;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(hOut, dwMode);
}

void ConsoleDashboard::Render(ServerServiceRef service)
{
	if (service == nullptr)
		return;

	ServerStats::UpdateRates();

	const int32 ccu = ServerStats::GetCurrentCcu();
	const int32 maxSessions = service->GetMaxSessionCount();
	const uint64 totalAccepted = ServerStats::GetTotalAccepted();
	const uint32 pps = ServerStats::GetCurrentPps();
	const double rxKb = ServerStats::GetCurrentRecvBps() / 1024.0;
	const double txKb = ServerStats::GetCurrentSendBps() / 1024.0;
	const uint32 totalAbuse = ServerStats::GetTotalAbuseDisconnects();
	const int32 activeRooms = RoomManager::GetTotalRoomCount();
	const std::string uptime = ServerStats::GetFormattedUptime();

	std::stringstream ss;

	// 커서를 (1, 1)로 이동하여 깜빡임(Flicker) 없이 부드럽게 덮어쓰기
	ss << "\033[H";

	ss << ANSI_CYAN << "========================================================================================\n" << ANSI_RESET;
	ss << ANSI_BOLD << ANSI_WHITE << "               HIGH-PERFORMANCE IOCP CHAT SERVER MONITORING DASHBOARD                   \n" << ANSI_RESET;
	ss << ANSI_CYAN << "========================================================================================\n" << ANSI_RESET;

	ss << ANSI_BOLD << " [Server Status] " << ANSI_GREEN << "ONLINE" << ANSI_RESET
		<< " | Port: " << ANSI_YELLOW << service->GetNetAddress().GetPort() << ANSI_RESET
		<< " | Uptime: " << ANSI_WHITE << uptime << ANSI_RESET
		<< " | Threads: " << ANSI_CYAN << "Worker + Timer" << ANSI_RESET << "\n";
	ss << "----------------------------------------------------------------------------------------\n";

	// 1. 핵심 트래픽 및 접속자 지표 카드
	ss << ANSI_BOLD << " [1] Network & User Metrics\n" << ANSI_RESET;
	ss << "  * Current CCU (Active)     : " << ANSI_GREEN << std::setw(6) << ccu << ANSI_RESET << " / " << maxSessions << "\n";
	ss << "  * Total Accepted Conns     : " << ANSI_WHITE << std::setw(6) << totalAccepted << ANSI_RESET << "\n";
	ss << "  * Throughput (PPS)         : " << ANSI_YELLOW << std::setw(6) << pps << " pkts/sec" << ANSI_RESET << "\n";
	ss << "  * Bandwidth (RX / TX)      : " << ANSI_CYAN << std::fixed << std::setprecision(2) << rxKb << " KB/s" << ANSI_RESET
		<< " / " << ANSI_MAGENTA << txKb << " KB/s\n" << ANSI_RESET;
	ss << "----------------------------------------------------------------------------------------\n";

	// 2. 콘텐츠(채팅방) 및 보안 지표
	ss << ANSI_BOLD << " [2] Chat Rooms & Security Defense\n" << ANSI_RESET;
	ss << "  * Active Chat Rooms (Actor): " << ANSI_CYAN << std::setw(6) << activeRooms << " Rooms" << ANSI_RESET << "\n";
	ss << "  * Abuse Force Disconnects  : " << (totalAbuse > 0 ? ANSI_RED : ANSI_GREEN) << std::setw(6) << totalAbuse << " cases (Flooding/Timeout)" << ANSI_RESET << "\n";
	ss << "----------------------------------------------------------------------------------------\n";

	// 3. 최근 어뷰징 제재 이벤트 테이블 (최근 5건)
	ss << ANSI_BOLD << " [3] Recent Abuse & Forced Disconnect Events (Latest 5)\n" << ANSI_RESET;
	ss << ANSI_WHITE << std::left
		<< std::setw(22) << " [Timestamp]"
		<< std::setw(12) << "[Session]"
		<< std::setw(16) << "[Player]"
		<< std::setw(22) << "[Endpoint]"
		<< "[Reason]\n" << ANSI_RESET;
	ss << " --------------------------------------------------------------------------------------\n";

	auto recentLogs = AbuseMonitor::GetRecentLogs(5);
	if (recentLogs.empty())
	{
		ss << "  (No abuse or forced disconnections detected. System running cleanly.)\n";
	}
	else
	{
		for (const auto& log : recentLogs)
		{
			std::string reasonStr;
			for (wchar_t wc : log.reason)
				reasonStr += static_cast<char>(wc);

			std::string endpoint = log.ipAddress + ":" + std::to_string(log.port);

			ss << ANSI_RED << "  "
				<< std::left << std::setw(20) << log.timeStr
				<< std::setw(12) << log.sessionId
				<< std::setw(16) << log.playerName
				<< std::setw(22) << endpoint
				<< reasonStr << ANSI_RESET << "\n";
		}
	}

	ss << "========================================================================================\n";
	ss << ANSI_BOLD << " [Commands] "
		<< ANSI_YELLOW << "[S]" << ANSI_RESET << " Save Log File | "
		<< ANSI_CYAN << "[C]" << ANSI_RESET << " Clear Screen | "
		<< ANSI_RED << "[Q]" << ANSI_RESET << " Graceful Shutdown\n";
	ss << "========================================================================================\n";

	std::cout << ss.str() << std::flush;
}
