#include "Session/ServerSession.h"
#include "Protocol/ClientPacketHandler.h"
#include <iostream>

ServerSession::ServerSession()
{
}

void ServerSession::OnConnected()
{
	std::cout << "\033[1;32m[SYSTEM] Successfully connected to Chat Server!\033[0m" << std::endl;
	std::cout << "\033[1;33m[SYSTEM] Please login with: /login <nickname>\033[0m" << std::endl;
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	if (false == ClientPacketHandler::HandlePacket(GetPacketSessionRef(), buffer, len))
	{
		std::cerr << "\033[1;31m[ERROR] Failed to parse received server packet.\033[0m" << std::endl;
	}
}

void ServerSession::OnDisconnected()
{
	_isLoggedIn.store(false);
	_currentRoomId.store(0);
	std::cout << "\n\033[1;31m[SYSTEM] Disconnected from Chat Server.\033[0m" << std::endl;
}
