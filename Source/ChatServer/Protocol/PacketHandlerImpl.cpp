#include "Protocol/ServerPacketHandler.h"
#include "Session/ClientSession.h"
#include "Room/Room.h"
#include "Room/RoomManager.h"

namespace
{
	std::atomic<uint64> GPlayerIdGenerator = 1000;
}

bool Handle_C_LOGIN(PacketSessionRef session, Protocol::C_LOGIN& pkt)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	uint64 playerId = GPlayerIdGenerator.fetch_add(1);
	clientSession->SetPlayerInfo(playerId, pkt.name);

	Protocol::S_LOGIN loginRes;
	loginRes.success = true;
	loginRes.playerId = playerId;
	loginRes.name = pkt.name;
	loginRes.message = "Welcome to High-Performance IOCP Chat Server!";

	clientSession->Send(loginRes.MakeSendBuffer());
	return true;
}

bool Handle_C_HEARTBEAT(PacketSessionRef session, Protocol::C_HEARTBEAT& pkt)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	clientSession->UpdateHeartbeat(::GetTickCount64());

	Protocol::S_HEARTBEAT heartbeatRes;
	heartbeatRes.serverTick = ::GetTickCount64();

	clientSession->Send(heartbeatRes.MakeSendBuffer());
	return true;
}

bool Handle_C_ROOM_LIST(PacketSessionRef session, Protocol::C_ROOM_LIST& /*pkt*/)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	Protocol::S_ROOM_LIST listRes;
	listRes.rooms = RoomManager::GetRoomListInfo();

	clientSession->Send(listRes.MakeSendBuffer());
	return true;
}


bool Handle_C_ENTER_ROOM(PacketSessionRef session, Protocol::C_ENTER_ROOM& pkt)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	std::string roomName = pkt.roomName.empty() ? ("Room_" + std::to_string(pkt.roomId)) : pkt.roomName;
	RoomRef targetRoom = RoomManager::GetOrCreateRoom(pkt.roomId, roomName);

	// 1. 기존에 참가 중이던 방이 있는 경우 -> 연쇄 파이프라인 실행
	if (RoomRef oldRoom = clientSession->GetRoom())
	{
		if (oldRoom == targetRoom)
		{
			return true; // 이미 같은 방에 있으면 무시
		}
		// [핵심]: oldRoom에게 "퇴장 끝나고 targetRoom으로 보내줘"라고 단 1개의 잡만 요청!
		oldRoom->PushJob(oldRoom, &Room::LeaveAndEnter, clientSession, targetRoom);
	}
	// 2. 이전에 참가 중인 방이 없던 경우 -> 새 방으로 바로 입장
	else
	{
		targetRoom->PushJob(targetRoom, &Room::Enter, clientSession);
	}
	return true;

	//RoomRef currentRoom = clientSession->GetRoom();

	//if (currentRoom != nullptr && currentRoom->GetRoomId() == pkt.roomId)
	//{
	//	return true;
	//}

	//// 기존 참가 중이던 방이 있다면 먼저 퇴장 처리
	//if (currentRoom != nullptr)
	//{
	//	currentRoom->PushJob(currentRoom, &Room::Leave, clientSession);
	//}


	////신규 방 참가 (JobQueue를 통한 비동기 무락 실행)
	//targetRoom->PushJob(targetRoom, &Room::Enter, clientSession);
	//return true;
}

bool Handle_C_LEAVE_ROOM(PacketSessionRef session, Protocol::C_LEAVE_ROOM& /*pkt*/)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	if (RoomRef room = clientSession->GetRoom())
	{
		room->PushJob(room, &Room::Leave, clientSession);
	}

	return true;
}

bool Handle_C_ROOM_CHAT(PacketSessionRef session, Protocol::C_ROOM_CHAT& pkt)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	if (RoomRef room = clientSession->GetRoom())
	{
		room->PushJob(room, &Room::HandleChat, clientSession, pkt.message);
	}

	return true;
}

bool Handle_C_GLOBAL_CHAT(PacketSessionRef session, Protocol::C_GLOBAL_CHAT& pkt)
{
	ClientSessionRef clientSession = std::static_pointer_cast<ClientSession>(session);
	if (clientSession == nullptr)
		return false;

	// 전체 서버 브로드캐스팅 (방 참가 여부와 관계없이 모든 접속 인원 수신)
	Protocol::S_GLOBAL_CHAT globalPkt;
	globalPkt.senderName = clientSession->GetPlayerName();
	globalPkt.message = pkt.message;

	if (auto service = clientSession->GetService())
	{
		service->Broadcast(globalPkt.MakeSendBuffer());
	}

	return true;
}
