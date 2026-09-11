#pragma once
#include "ServerEngine.h"
#include "Room/Room.h"
#include "Protocol/PacketProtocol.h"
#include <unordered_map>
#include <vector>

/*-------------------
    RoomManager
--------------------*/
class RoomManager
{
public:
	static void Init();

	static RoomRef CreateRoom(int32 roomId, const std::string& name);
	static RoomRef FindRoom(int32 roomId);
	static RoomRef GetOrCreateRoom(int32 roomId, const std::string& defaultName);
	static void RemoveRoom(int32 roomId);

	static std::vector<Protocol::S_ROOM_LIST::RoomsInfo> GetRoomListInfo();
	static int32 GetTotalRoomCount();

private:
	static ReadWriteLock _lock;
	static std::unordered_map<int32, RoomRef> _rooms;
};
