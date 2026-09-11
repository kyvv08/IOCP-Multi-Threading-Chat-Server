#include "Protocol/ClientPacketHandler.h"
#include "Session/DummySession.h"

bool Handle_S_LOGIN(PacketSessionRef session, Protocol::S_LOGIN& pkt)
{
	DummySessionRef dummy = std::static_pointer_cast<DummySession>(session);
	if (dummy && pkt.success)
	{
		dummy->SetLoggedIn(true);

		// 정상 봇인 경우 룸 #1 (Lobby) 자동 입장 요청
		if (dummy->GetScenario() == BotScenario::Normal)
		{
			Protocol::C_ENTER_ROOM enterPkt;
			enterPkt.roomId = rand() % 50 + 1;
			//enterPkt.roomName = "Lobby Channel 1";
			dummy->Send(enterPkt.MakeSendBuffer());
			StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
		}
	}
	return true;
}

bool Handle_S_HEARTBEAT(PacketSessionRef /*session*/, Protocol::S_HEARTBEAT& /*pkt*/)
{
	return true;
}

bool Handle_S_ROOM_LIST(PacketSessionRef /*session*/, Protocol::S_ROOM_LIST& /*pkt*/)
{
	return true;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef session, Protocol::S_ENTER_ROOM& pkt)
{
	DummySessionRef dummy = std::static_pointer_cast<DummySession>(session);
	if (dummy && pkt.success)
	{
		dummy->SetInRoom(true);
	}
	return true;
}

bool Handle_S_LEAVE_ROOM(PacketSessionRef session, Protocol::S_LEAVE_ROOM& /*pkt*/)
{
	DummySessionRef dummy = std::static_pointer_cast<DummySession>(session);
	if (dummy)
	{
		dummy->SetInRoom(false);
	}
	return true;
}

bool Handle_S_ROOM_CHAT(PacketSessionRef /*session*/, Protocol::S_ROOM_CHAT& /*pkt*/)
{
	return true;
}

bool Handle_S_GLOBAL_CHAT(PacketSessionRef /*session*/, Protocol::S_GLOBAL_CHAT& /*pkt*/)
{
	return true;
}

bool Handle_S_NOTICE(PacketSessionRef /*session*/, Protocol::S_NOTICE& /*pkt*/)
{
	return true;
}
