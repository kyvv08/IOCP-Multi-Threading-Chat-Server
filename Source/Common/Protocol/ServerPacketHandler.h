#pragma once
#include "PacketProtocol.h"

#define TIME_OUT_MS 30000 //30초 타임 아웃

// 사용자 정의 패킷 핸들러 전방 선언
bool Handle_C_LOGIN(PacketSessionRef session, Protocol::C_LOGIN& pkt);
bool Handle_C_HEARTBEAT(PacketSessionRef session, Protocol::C_HEARTBEAT& pkt);
bool Handle_C_ROOM_LIST(PacketSessionRef session, Protocol::C_ROOM_LIST& pkt);
bool Handle_C_ENTER_ROOM(PacketSessionRef session, Protocol::C_ENTER_ROOM& pkt);
bool Handle_C_LEAVE_ROOM(PacketSessionRef session, Protocol::C_LEAVE_ROOM& pkt);
bool Handle_C_ROOM_CHAT(PacketSessionRef session, Protocol::C_ROOM_CHAT& pkt);
bool Handle_C_GLOBAL_CHAT(PacketSessionRef session, Protocol::C_GLOBAL_CHAT& pkt);


class ServerPacketHandler
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
		case Protocol::PKT_C_LOGIN:
			{
				Protocol::C_LOGIN pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_LOGIN(session, pkt);
			}
		case Protocol::PKT_C_HEARTBEAT:
			{
				Protocol::C_HEARTBEAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_HEARTBEAT(session, pkt);
			}
		case Protocol::PKT_C_ROOM_LIST:
			{
				Protocol::C_ROOM_LIST pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_ROOM_LIST(session, pkt);
			}
		case Protocol::PKT_C_ENTER_ROOM:
			{
				Protocol::C_ENTER_ROOM pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_ENTER_ROOM(session, pkt);
			}
		case Protocol::PKT_C_LEAVE_ROOM:
			{
				Protocol::C_LEAVE_ROOM pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_LEAVE_ROOM(session, pkt);
			}
		case Protocol::PKT_C_ROOM_CHAT:
			{
				Protocol::C_ROOM_CHAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_ROOM_CHAT(session, pkt);
			}
		case Protocol::PKT_C_GLOBAL_CHAT:
			{
				Protocol::C_GLOBAL_CHAT pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_C_GLOBAL_CHAT(session, pkt);
			}

		default:
			return false;
		}

		return true;
	}
};
