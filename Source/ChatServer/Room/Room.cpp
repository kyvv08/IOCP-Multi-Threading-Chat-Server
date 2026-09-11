#include "Room/Room.h"
#include "Protocol/PacketProtocol.h"

Room::Room(int32 roomId, const std::string& name) : _roomId(roomId), _roomName(name)
{
}

void Room::Enter(ClientSessionRef session)
{
	if (session == nullptr)
		return;
	{
		//_lock.LockWrite();
		_members[session->GetSessionId()] = session;
		session->SetRoom(std::static_pointer_cast<Room>(shared_from_this()));

		// 1. 입장한 유저에게 방 정보 및 기존 멤버 목록 응답 전송
		Protocol::S_ENTER_ROOM enterRes;
		enterRes.success = true;
		enterRes.roomId = _roomId;
		enterRes.roomName = _roomName;

		for (const auto& [id, member] : _members)
		{
			enterRes.members.push_back({ member->GetPlayerName() });
		}

		//_lock.UnlockWrite();
		session->Send(enterRes.MakeSendBuffer());
	}
	// 2. 방 내부 전체 인원에게 입장 알림 공지 브로드캐스트
	Protocol::S_NOTICE noticePkt;
	noticePkt.message = "[" + session->GetPlayerName() + "] joined the room.";
	Broadcast(noticePkt.MakeSendBuffer());
}

void Room::Leave(ClientSessionRef session)
{
	if (session == nullptr)
		return;
	{
		//_lock.LockWrite();
		_members.erase(session->GetSessionId());
		//_lock.UnlockWrite();
	}
	if (session->GetRoom().get() == this)
	{
		session->SetRoom(nullptr);
	}
	// 1. 퇴장 유저에게 응답
	Protocol::S_LEAVE_ROOM leaveRes;
	leaveRes.success = true;
	session->Send(leaveRes.MakeSendBuffer());

	// 2. 남은 방 인원에게 퇴장 공지 브로드캐스트
	Protocol::S_NOTICE noticePkt;
	noticePkt.message = "[" + session->GetPlayerName() + "] left the room.";
	Broadcast(noticePkt.MakeSendBuffer());
}

// [신규 구현] 이전 방 퇴장 완료 직후 새 방으로 입장 잡을 토스
void Room::LeaveAndEnter(ClientSessionRef session, RoomRef targetRoom)
{
	if (session == nullptr)
		return;
	// 1. 내 방(이전 방)에서 세션을 완전히 삭제하고 퇴장 공지 브로드캐스트
	Leave(session);
	// 2. 퇴장이 100% 완료된 시점에 새 방의 JobQueue로 Enter를 푸시!
	if (targetRoom != nullptr)
	{
		targetRoom->PushJob(targetRoom, &Room::Enter, session);
	}
}

void Room::HandleChat(ClientSessionRef session, const std::string& message)
{
	if (session == nullptr)
		return;

	// 방 내부 룸 채팅 브로드캐스트
	Protocol::S_ROOM_CHAT chatPkt;
	chatPkt.senderName = session->GetPlayerName();
	chatPkt.message = message;

	Broadcast(chatPkt.MakeSendBuffer());
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	//ReadLockGuard gurad(_lock);
	for (const auto& [id, member] : _members)
	{
		member->Send(sendBuffer);
	}
}

std::vector<std::string> Room::GetMemberNames() const
{
	std::vector<std::string> names;
	names.reserve(_members.size());
	for (const auto& [id, member] : _members)
	{
		names.push_back(member->GetPlayerName());
	}
	return names;
}
