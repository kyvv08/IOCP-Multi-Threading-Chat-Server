#include "Session/DummySession.h"
#include "Protocol/ClientPacketHandler.h"
#include <iostream>

std::atomic<int32> StressStats::activeSessions{ 0 };
std::atomic<int32> StressStats::totalConnected{ 0 };
std::atomic<int32> StressStats::totalDisconnected{ 0 };
std::atomic<int32> StressStats::abuseDisconnected{ 0 };
std::atomic<uint64> StressStats::totalSentPackets{ 0 };
std::atomic<uint64> StressStats::totalRecvPackets{ 0 };

DummySession::DummySession(BotScenario scenario, int32 botIndex)
	: _scenario(scenario), _botIndex(botIndex)
{
	_botName = "Bot_" + std::to_string(botIndex);
}

void DummySession::OnConnected()
{
	StressStats::activeSessions.fetch_add(1, std::memory_order_relaxed);
	StressStats::totalConnected.fetch_add(1, std::memory_order_relaxed);
	_connectedTick = ::GetTickCount64();

	switch (_scenario)
	{
	case BotScenario::Normal:
		{
			// 1. 로그인 패킷 전송
			Protocol::C_LOGIN loginPkt;
			loginPkt.name = _botName;
			Send(loginPkt.MakeSendBuffer());
			StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
		}
		break;

	case BotScenario::Slowloris:
		{
			// 아무것도 보내지 않고 세션만 점유 -> 15초 후 서버 타임아웃 강제 퇴장 대기
		}
		break;

	case BotScenario::ConnectSpam:
		{
			// 접속 즉시 Disconnect 호출하여 빠른 세션 순환 유도
			Disconnect(L"Spam Disconnect");
		}
		break;

	case BotScenario::PacketFlood:
		{
			// 1. 로그인 전송 후 패킷 폭주 난사
			Protocol::C_LOGIN loginPkt;
			loginPkt.name = _botName;
			Send(loginPkt.MakeSendBuffer());
			StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);

			// 즉시 80개 패킷 연속 송신 (토큰 버킷 한도 초과 유발)
			ExecuteFloodAttack(80);
		}
		break;
	}
}

void DummySession::OnRecvPacket(BYTE* buffer, int32 len)
{
	StressStats::totalRecvPackets.fetch_add(1, std::memory_order_relaxed);
	ClientPacketHandler::HandlePacket(GetPacketSessionRef(), buffer, len);
}

void DummySession::OnDisconnected()
{
	StressStats::activeSessions.fetch_sub(1, std::memory_order_relaxed);
	StressStats::totalDisconnected.fetch_add(1, std::memory_order_relaxed);

	if (_scenario == BotScenario::Slowloris || _scenario == BotScenario::PacketFlood)
	{
		// 비정상 봇이 서버로부터 강제 퇴장당했음을 기록
		StressStats::abuseDisconnected.fetch_add(1, std::memory_order_relaxed);
	}
}

void DummySession::SendChat(const std::string& msg)
{
	if (!IsConnected() || !_isInRoom.load())
		return;

	Protocol::C_ROOM_CHAT chatPkt;
	chatPkt.message = msg;
	Send(chatPkt.MakeSendBuffer());
	StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
}

void DummySession::SendEnterRoom(const std::string& msg, int roomId)
{
	if (!IsConnected())
		return;

	/*if (_isInRoom.load()) {
		Protocol::C_LEAVE_ROOM leavePkt;
		Send(leavePkt.MakeSendBuffer());
		StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
	}*/

	Protocol::C_ENTER_ROOM enterPkt;
	enterPkt.roomId = roomId;
	enterPkt.roomName = msg;
	Send(enterPkt.MakeSendBuffer());
	StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
}

void DummySession::SendHeartbeat()
{
	if (!IsConnected())
		return;

	Protocol::C_HEARTBEAT heartbeatPkt;
	heartbeatPkt.clientTick = ::GetTickCount64();
	Send(heartbeatPkt.MakeSendBuffer());
	StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
}

void DummySession::ExecuteFloodAttack(int32 count)
{
	for (int32 i = 0; i < count; ++i)
	{
		Protocol::C_ROOM_CHAT chatPkt;
		chatPkt.message = "Flooding attack packet #" + std::to_string(i + 1);
		Send(chatPkt.MakeSendBuffer());
		StressStats::totalSentPackets.fetch_add(1, std::memory_order_relaxed);
	}
}
