#include "Protocol/ClientPacketHandler.h"
#include "Session/DummySession.h"
#include "Monitoring/LatencyMonitor.h"
#include <chrono>

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

bool Handle_S_ROOM_CHAT(PacketSessionRef session, Protocol::S_ROOM_CHAT& pkt)
{
	DummySessionRef dummy = std::static_pointer_cast<DummySession>(session);
	if (dummy && pkt.senderName == dummy->GetBotName())
	{
		// 자신이 전송한 레이턴시 태그 패킷인지 확인 ("LAT:<seq>:<timestampUs>")
		if (pkt.message.rfind("LAT:", 0) == 0)
		{
			size_t firstColon = pkt.message.find(':', 4);
			if (firstColon != std::string::npos)
			{
				try
				{
					uint64 sentUs = std::stoull(pkt.message.substr(firstColon + 1));
					auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::high_resolution_clock::now().time_since_epoch()).count();

					if (nowUs >= sentUs)
					{
						uint64 rttUs = static_cast<uint64>(nowUs - sentUs);
						LatencyMonitor::RecordSample(rttUs);
					}
				}
				catch (...)
				{
				}
			}
		}
	}
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
