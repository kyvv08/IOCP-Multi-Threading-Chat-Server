#pragma once
#include "ServerEngine.h"
#include "Session/ClientSession.h"
#include <unordered_map>
#include <string>
#include <vector>

/*--------------
    Room
---------------*/
class Room : public JobQueue
{
public:
	Room(int32 roomId, const std::string& name);
	virtual ~Room() = default;

	// 비즈니스 로직 (JobQueue를 통해 워커 스레드가 단일 스레드로 직렬화 실행 -> No Lock Needed!)
	void Enter(ClientSessionRef session);
	void Leave(ClientSessionRef session);
	void HandleChat(ClientSessionRef session, const std::string& message);
	void Broadcast(SendBufferRef sendBuffer);
	
	// [신규 추가] 이전 방 퇴장 후 신규 방 입장 연쇄 실행 함수
	void LeaveAndEnter(ClientSessionRef session, std::shared_ptr<Room> targetRoom);

	int32 GetRoomId() const { return _roomId; }
	const std::string& GetRoomName() const { return _roomName; }
	int32 GetUserCount() const { return static_cast<int32>(_members.size()); }

	std::vector<std::string> GetMemberNames() const;

private:
	ReadWriteLock _lock;
	int32 _roomId = 0;
	std::string _roomName;
	std::unordered_map<uint64, ClientSessionRef> _members;
};

using RoomRef = std::shared_ptr<Room>;
