#include "ServerEngine.h"
#include "StressTester.h"
#include "Protocol/ClientPacketHandler.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	// ANSI 터미널 지원 활성화
	HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	if (::GetConsoleMode(hOut, &dwMode))
	{
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		::SetConsoleMode(hOut, dwMode);
	}

	SocketUtils::Init();
	PoolAllocator::Init();
	ClientPacketHandler::Init();

	std::wstring serverIp = L"127.0.0.1";
	uint16 serverPort = 7777;

	if (argc >= 2)
	{
		std::string ipStr = argv[1];
		serverIp = std::wstring(ipStr.begin(), ipStr.end());
	}
	if (argc >= 3)
	{
		serverPort = static_cast<uint16>(std::stoi(argv[2]));
	}

	while (true)
	{
		std::cout << "\n\033[1;36m=================================================================\033[0m\n";
		std::cout << "\033[1;37m       HIGH-PERFORMANCE IOCP CHAT SERVER STRESS TEST SUITE       \033[0m\n";
		std::cout << "\033[1;36m=================================================================\033[0m\n";
		std::cout << "  \033[1;33m[1]\033[0m Normal Chatting Load Test (500 Bots, 15 sec)\n";
		std::cout << "  \033[1;33m[2]\033[0m Slowloris / Idle Keep-Alive Timeout Test (100 Bots, 20 sec)\n";
		std::cout << "  \033[1;33m[3]\033[0m Connect / Disconnect Rapid Spam Stress Test (200 Bots, 10 sec)\n";
		std::cout << "  \033[1;33m[4]\033[0m Packet Flooding Attack Defense Test (100 Bots, 10 sec)\n";
		std::cout << "  \033[1;33m[5]\033[0m All-in-One Comprehensive Chaos Test (All Scenarios Sequenced)\n";
		std::cout << "  \033[1;31m[0]\033[0m Exit Stress Tester\n";
		std::cout << "\033[1;36m=================================================================\033[0m\n";
		std::cout << "Select Scenario Number >> ";

		int choice = -1;
		if (!(std::cin >> choice))
		{
			std::cin.clear();
			std::string discard;
			std::cin >> discard;
			continue;
		}

		if (choice == 0)
			break;

		switch (choice)
		{
		case 1:
			StressTester::RunScenario(BotScenario::Normal, 6000, 3600*10, serverIp, serverPort);
			break;
		case 2:
			StressTester::RunScenario(BotScenario::Slowloris, 100, 20, serverIp, serverPort);
			break;
		case 3:
			StressTester::RunScenario(BotScenario::ConnectSpam, 200, 10, serverIp, serverPort);
			break;
		case 4:
			StressTester::RunScenario(BotScenario::PacketFlood, 100, 10, serverIp, serverPort);
			break;
		case 5:
			std::cout << "\n\033[1;35m>>> RUNNING ALL-IN-ONE CHAOS STRESS SUITE <<<\033[0m\n";
			StressTester::RunScenario(BotScenario::ConnectSpam, 200, 8, serverIp, serverPort);
			StressTester::RunScenario(BotScenario::PacketFlood, 100, 8, serverIp, serverPort);
			StressTester::RunScenario(BotScenario::Slowloris, 100, 18, serverIp, serverPort);
			StressTester::RunScenario(BotScenario::Normal, 500, 15, serverIp, serverPort);
			std::cout << "\n\033[1;32m>>> ALL-IN-ONE CHAOS TEST COMPLETED SUCCESSFULLY! <<<\033[0m\n";
			break;
		default:
			std::cout << "\033[1;31m[ERROR] Invalid scenario number.\033[0m\n";
			break;
		}
	}

	SocketUtils::Clear();
	PoolAllocator::Clear();

	std::cout << "\033[1;32m[SYSTEM] Stress Tester terminated cleanly.\033[0m\n";
	return 0;
}
