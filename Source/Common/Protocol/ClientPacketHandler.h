#pragma once
#include "PacketProtocol.h"

// 사용자 정의 패킷 핸들러 전방 선언
bool Handle_S_LOGIN(PacketSessionRef session, Protocol::S_LOGIN& pkt);
bool Handle_S_HEARTBEAT(PacketSessionRef session, Protocol::S_HEARTBEAT& pkt);
bool Handle_S_ROOM_LIST(PacketSessionRef session, Protocol::S_ROOM_LIST& pkt);
bool Handle_S_ENTER_ROOM(PacketSessionRef session, Protocol::S_ENTER_ROOM& pkt);
bool Handle_S_LEAVE_ROOM(PacketSessionRef session, Protocol::S_LEAVE_ROOM& pkt);
bool Handle_S_ROOM_CHAT(PacketSessionRef session, Protocol::S_ROOM_CHAT& pkt);
bool Handle_S_GLOBAL_CHAT(PacketSessionRef session, Protocol::S_GLOBAL_CHAT& pkt);
bool Handle_S_NOTICE(PacketSessionRef session, Protocol::S_NOTICE& pkt);


class ClientPacketHandler
{
public:
	static void Init()
	{
	}

	static bool HandlePacket(PacketSessionRef session, BYTE* buffer, int32 len)
	{
		BufferReader reader(buffer, len);
		PacketHeader header;
		if (reader.Read(header) == false)
			return false;

		switch (header.id)
		{
		case Protocol::PKT_S_LOGIN:
			{
				Protocol::S_LOGIN pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_LOGIN(session, pkt);
			}
		case Protocol::PKT_S_HEARTBEAT:
			{
				Protocol::S_HEARTBEAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_HEARTBEAT(session, pkt);
			}
		case Protocol::PKT_S_ROOM_LIST:
			{
				Protocol::S_ROOM_LIST pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_ROOM_LIST(session, pkt);
			}
		case Protocol::PKT_S_ENTER_ROOM:
			{
				Protocol::S_ENTER_ROOM pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_ENTER_ROOM(session, pkt);
			}
		case Protocol::PKT_S_LEAVE_ROOM:
			{
				Protocol::S_LEAVE_ROOM pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_LEAVE_ROOM(session, pkt);
			}
		case Protocol::PKT_S_ROOM_CHAT:
			{
				Protocol::S_ROOM_CHAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_ROOM_CHAT(session, pkt);
			}
		case Protocol::PKT_S_GLOBAL_CHAT:
			{
				Protocol::S_GLOBAL_CHAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_GLOBAL_CHAT(session, pkt);
			}
		case Protocol::PKT_S_NOTICE:
			{
				Protocol::S_NOTICE pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_S_NOTICE(session, pkt);
			}

		default:
			return false;
		}

		return true;
	}
};
