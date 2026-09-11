#include "ServerEngine.h"
#include "Session/ServerSession.h"
#include "Protocol/ClientPacketHandler.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>

void PrintHelp()
{
	std::cout << "\n\033[1;36m====================== CLIENT COMMAND GUIDE ======================\033[0m\n"
		<< "  \033[1;33m/login <nickname>\033[0m         : Log into the server with nickname\n"
		<< "  \033[1;33m/list\033[0m                     : Request available chat room list\n"
		<< "  \033[1;33m/join <roomId> [name]\033[0m     : Join or create a chat room\n"
		<< "  \033[1;33m/leave\033[0m                    : Leave current chat room\n"
		<< "  \033[1;33m/global <message>\033[0m         : Send message to all users on server\n"
		<< "  \033[1;33m/flood <count>\033[0m            : Send rapid packet burst (Flooding test)\n"
		<< "  \033[1;33m/help\033[0m                     : Display this help guide\n"
		<< "  \033[1;33m/quit\033[0m                     : Disconnect and exit client\n"
		<< "  \033[1;37m<text message>\033[0m            : Send message to currently joined room\n"
		<< "\033[1;36m==================================================================\033[0m\n";
}

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

	std::cout << "\033[1;36m=================================================================\033[0m\n";
	std::cout << "\033[1;37m        High-Performance IOCP Interactive Chat Client            \033[0m\n";
	std::cout << "\033[1;36m=================================================================\033[0m\n";

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

	std::shared_ptr<IocpCore> iocpCore = std::make_shared<IocpCore>();

	ServerSessionRef serverSession = nullptr;
	ClientServiceRef clientService = std::make_shared<ClientService>(
		NetAddress(serverIp, serverPort),
		iocpCore,
		[&serverSession]()
		{
			serverSession = std::make_shared<ServerSession>();
			return serverSession;
		},
		1
	);

	if (false == clientService->Start())
	{
		std::cerr << "\033[1;31m[ERROR] Failed to start ClientService!\033[0m" << std::endl;
		return 1;
	}

	std::atomic<bool> isRunning{ true };

	// 1. IOCP 네트워크 디스패치 백그라운드 스레드
	std::thread networkThread([&iocpCore, &isRunning]()
	{
		PoolAllocator::Init();
		while (isRunning.load())
		{
			iocpCore->Dispatch(10);
		}
	});

	// 2. 하트비트 Keep-Alive 자동 송신 스레드 (5초 주기)
	std::thread heartbeatThread([&serverSession, &isRunning]()
	{
		PoolAllocator::Init();
		while (isRunning.load())
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			if (serverSession && serverSession->IsConnected())
			{
				Protocol::C_HEARTBEAT heartbeatPkt;
				heartbeatPkt.clientTick = ::GetTickCount64();
				serverSession->Send(heartbeatPkt.MakeSendBuffer());
			}
		}
	});

	// 초기 가이드 출력
	PrintHelp();

	// 3. 대화형 명령어 콘솔 입력 루프
	std::string line;
	while (isRunning.load() && std::getline(std::cin, line))
	{
		if (line.empty())
			continue;

		if (serverSession == nullptr || !serverSession->IsConnected())
		{
			std::cout << "\033[1;31m[ERROR] Not connected to server.\033[0m\n";
			continue;
		}

		if (line[0] == '/')
		{
			std::stringstream ss(line);
			std::string cmd;
			ss >> cmd;

			if (cmd == "/help")
			{
				PrintHelp();
			}
			else if (cmd == "/login")
			{
				std::string name;
				ss >> name;
				if (name.empty())
				{
					std::cout << "\033[1;31m[USAGE] /login <nickname>\033[0m\n";
					continue;
				}

				Protocol::C_LOGIN loginPkt;
				loginPkt.name = name;
				serverSession->Send(loginPkt.MakeSendBuffer());
			}
			else if (cmd == "/list")
			{
				Protocol::C_ROOM_LIST listPkt;
				serverSession->Send(listPkt.MakeSendBuffer());
			}
			else if (cmd == "/join")
			{
				int32 roomId = 0;
				std::string roomName;
				ss >> roomId;
				ss >> roomName;

				if (roomId <= 0)
				{
					std::cout << "\033[1;31m[USAGE] /join <roomId> [roomName]\033[0m\n";
					continue;
				}

				Protocol::C_ENTER_ROOM enterPkt;
				enterPkt.roomId = roomId;
				enterPkt.roomName = roomName;
				serverSession->Send(enterPkt.MakeSendBuffer());
			}
			else if (cmd == "/leave")
			{
				Protocol::C_LEAVE_ROOM leavePkt;
				serverSession->Send(leavePkt.MakeSendBuffer());
			}
			else if (cmd == "/global")
			{
				std::string msg;
				std::getline(ss >> std::ws, msg);
				if (msg.empty())
				{
					std::cout << "\033[1;31m[USAGE] /global <message>\033[0m\n";
					continue;
				}

				Protocol::C_GLOBAL_CHAT globalPkt;
				globalPkt.message = msg;
				serverSession->Send(globalPkt.MakeSendBuffer());
			}
			else if (cmd == "/flood")
			{
				int32 count = 50;
				ss >> count;
				std::cout << "\033[1;31m[FLOOD TEST] Sending " << count << " rapid chat packets...\033[0m\n";
				for (int32 i = 0; i < count; ++i)
				{
					Protocol::C_ROOM_CHAT floodPkt;
					floodPkt.message = "Flooding packet #" + std::to_string(i + 1);
					serverSession->Send(floodPkt.MakeSendBuffer());
				}
			}
			else if (cmd == "/quit")
			{
				std::cout << "\033[1;33m[SYSTEM] Exiting client...\033[0m\n";
				isRunning.store(false);
				break;
			}
			else
			{
				std::cout << "\033[1;31m[UNKNOWN COMMAND] Type '/help' for available commands.\033[0m\n";
			}
		}
		else
		{
			// 방에 참가 중이면 일반 텍스트를 방 채팅으로 전송
			if (serverSession->GetCurrentRoomId() > 0)
			{
				Protocol::C_ROOM_CHAT chatPkt;
				chatPkt.message = line;
				serverSession->Send(chatPkt.MakeSendBuffer());
			}
			else
			{
				std::cout << "\033[1;33m[INFO] You are not in any room. Use '/join <id>' to enter a room or '/global <msg>' to broadcast.\033[0m\n";
			}
		}
	}

	clientService->Stop();

	if (heartbeatThread.joinable())
		heartbeatThread.join();

	if (networkThread.joinable())
		networkThread.join();

	SocketUtils::Clear();
	PoolAllocator::Clear();

	return 0;
}
