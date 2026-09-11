#include "Room/RoomManager.h"

ReadWriteLock RoomManager::_lock;
std::unordered_map<int32, RoomRef> RoomManager::_rooms;

void RoomManager::Init()
{
	WriteLockGuard guard(_lock);
	_rooms.clear();

	// 기본 채널 방 3개 사전 생성
	_rooms[1] = std::make_shared<Room>(1, "Lobby Channel 1");
	_rooms[2] = std::make_shared<Room>(2, "Chat Room 2");
	_rooms[3] = std::make_shared<Room>(3, "Free Talk 3");
}

RoomRef RoomManager::CreateRoom(int32 roomId, const std::string& name)
{
	WriteLockGuard guard(_lock);
	auto it = _rooms.find(roomId);
	if (it != _rooms.end())
		return it->second;

	RoomRef room = std::make_shared<Room>(roomId, name);
	_rooms[roomId] = room;
	return room;
}

RoomRef RoomManager::FindRoom(int32 roomId)
{
	ReadLockGuard guard(_lock);
	auto it = _rooms.find(roomId);
	if (it != _rooms.end())
		return it->second;

	return nullptr;
}

RoomRef RoomManager::GetOrCreateRoom(int32 roomId, const std::string& defaultName)
{
	{
		ReadLockGuard guard(_lock);
		auto it = _rooms.find(roomId); 
		if (it != _rooms.end())
			return it->second;
	}

	return CreateRoom(roomId, defaultName);
}

void RoomManager::RemoveRoom(int32 roomId)
{
	WriteLockGuard guard(_lock);
	_rooms.erase(roomId);
}

std::vector<Protocol::S_ROOM_LIST::RoomsInfo> RoomManager::GetRoomListInfo()
{
	ReadLockGuard guard(_lock);
	std::vector<Protocol::S_ROOM_LIST::RoomsInfo> list;
	list.reserve(_rooms.size());

	for (const auto& [id, room] : _rooms)
	{
		Protocol::S_ROOM_LIST::RoomsInfo info;
		info.roomId = room->GetRoomId();
		info.roomName = room->GetRoomName();
		info.userCount = room->GetUserCount();
		list.push_back(info);
	}

	return list;
}

int32 RoomManager::GetTotalRoomCount()
{
	ReadLockGuard guard(_lock);
	return static_cast<int32>(_rooms.size());
}
