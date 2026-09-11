#include "Protocol/ClientPacketHandler.h"
#include "Session/ServerSession.h"
#include <iostream>
#include <iomanip>

bool Handle_S_LOGIN(PacketSessionRef session, Protocol::S_LOGIN& pkt)
{
	ServerSessionRef serverSession = std::static_pointer_cast<ServerSession>(session);
	if (pkt.success)
	{
		serverSession->SetLoggedIn(true);
		serverSession->SetPlayerName(pkt.name);
		std::cout << "\033[1;32m[LOGIN SUCCESS] Logged in as '" << pkt.name
			<< "' (Player ID: " << pkt.playerId << ")\033[0m\n"
			<< "\033[1;36m  * " << pkt.message << "\033[0m" << std::endl;
		std::cout << "\033[1;33m[INFO] Type '/list' to view rooms, '/join <id>' to enter a room, '/help' for commands.\033[0m" << std::endl;
	}
	else
	{
		std::cout << "\033[1;31m[LOGIN FAILED] " << pkt.message << "\033[0m" << std::endl;
	}
	return true;
}

bool Handle_S_HEARTBEAT(PacketSessionRef /*session*/, Protocol::S_HEARTBEAT& /*pkt*/)
{
	// 하트비트 응답 수신 (Keep-Alive 유지 확인)
	return true;
}

bool Handle_S_ROOM_LIST(PacketSessionRef /*session*/, Protocol::S_ROOM_LIST& pkt)
{
	std::cout << "\n\033[1;36m====================== AVAILABLE CHAT ROOMS ======================\033[0m\n";
	std::cout << std::left << std::setw(12) << " [Room ID]"
		<< std::setw(30) << "[Room Name]"
		<< "[Active Users]\n";
	std::cout << "------------------------------------------------------------------\n";

	if (pkt.rooms.empty())
	{
		std::cout << "  (No rooms available. Create one with /join <id> <name>)\n";
	}
	else
	{
		int totalUserCount = 0;
		for (const auto& room : pkt.rooms)
		{
			std::cout << "  " << std::left << std::setw(10) << room.roomId
				<< std::setw(30) << room.roomName
				<< room.userCount << " users\n";
			totalUserCount += room.userCount;
		}
		std::cout << "  " << std::left << std::setw(10) << ""
			<< std::setw(30) << ""
			<< totalUserCount << std::endl; 
	}
	std::cout << "\033[1;36m==================================================================\033[0m\n";
	return true;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef session, Protocol::S_ENTER_ROOM& pkt)
{
	ServerSessionRef serverSession = std::static_pointer_cast<ServerSession>(session);
	if (pkt.success)
	{
		serverSession->SetCurrentRoomId(pkt.roomId);
		std::cout << "\n\033[1;32m[ROOM JOINED] Successfully entered Room #" << pkt.roomId
			<< " ('" << pkt.roomName << "')\033[0m\n";

		std::cout << "\033[1;37m  * Current Members (" << pkt.members.size() << "): \033[0m";
		for (size_t i = 0; i < pkt.members.size(); ++i)
		{
			std::cout << pkt.members[i].name << (i + 1 < pkt.members.size() ? ", " : "");
		}
		std::cout << "\n\033[1;33m  (Messages typed now will be sent to this room. Use '/leave' to exit.)\033[0m\n";
	}
	else
	{
		std::cout << "\033[1;31m[ROOM ERROR] Failed to enter room #" << pkt.roomId << "\033[0m\n";
	}
	return true;
}

bool Handle_S_LEAVE_ROOM(PacketSessionRef session, Protocol::S_LEAVE_ROOM& pkt)
{
	ServerSessionRef serverSession = std::static_pointer_cast<ServerSession>(session);
	if (pkt.success)
	{
		int32 prevRoomId = serverSession->GetCurrentRoomId();
		serverSession->SetCurrentRoomId(0);
		std::cout << "\033[1;33m[ROOM LEFT] Left Room #" << prevRoomId << ".\033[0m\n";
	}
	return true;
}

bool Handle_S_ROOM_CHAT(PacketSessionRef session, Protocol::S_ROOM_CHAT& pkt)
{
	ServerSessionRef serverSession = std::static_pointer_cast<ServerSession>(session);
	bool isMe = (serverSession && serverSession->GetPlayerName() == pkt.senderName);

	if (isMe)
	{
		std::cout << "\033[1;32m[Room " << serverSession->GetCurrentRoomId() << "] <" << pkt.senderName << "> : " << pkt.message << "\033[0m\n";
	}
	else
	{
		std::cout << "\033[1;37m[Room " << (serverSession ? serverSession->GetCurrentRoomId() : 0) << "] <\033[1;36m" << pkt.senderName << "\033[1;37m> : " << pkt.message << "\033[0m\n";
	}
	return true;
}

bool Handle_S_GLOBAL_CHAT(PacketSessionRef session, Protocol::S_GLOBAL_CHAT& pkt)
{
	ServerSessionRef serverSession = std::static_pointer_cast<ServerSession>(session);
	bool isMe = (serverSession && serverSession->GetPlayerName() == pkt.senderName);

	if (isMe)
	{
		std::cout << "\033[1;35m[GLOBAL] <" << pkt.senderName << "> : " << pkt.message << "\033[0m\n";
	}
	else
	{
		std::cout << "\033[1;35m[GLOBAL] <\033[1;33m" << pkt.senderName << "\033[1;35m> : " << pkt.message << "\033[0m\n";
	}
	return true;
}

bool Handle_S_NOTICE(PacketSessionRef /*session*/, Protocol::S_NOTICE& pkt)
{
	std::cout << "\033[1;33m[NOTICE] " << pkt.message << "\033[0m\n";
	return true;
}
