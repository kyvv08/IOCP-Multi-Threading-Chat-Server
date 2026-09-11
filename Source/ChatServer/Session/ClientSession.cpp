#include "Session/ClientSession.h"
#include "Room/Room.h"
#include "Service/AbuseMonitor.h"
#include "Monitoring/ServerStats.h"
#include "Protocol/ServerPacketHandler.h"

ClientSession::ClientSession()
{
	UpdateHeartbeat(::GetTickCount64());
}

void ClientSession::OnConnected()
{
	UpdateHeartbeat(::GetTickCount64());
	_rateLimiter.Reset();
	ServerStats::OnSessionConnected();
}

void ClientSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerStats::OnPacketReceived(len);

	std::wstring ipW = GetNetAddress().GetIpAddress();
	std::string ip;
	for (wchar_t c : ipW)
		ip += static_cast<char>(c);
	uint16 port = GetNetAddress().GetPort();

	// 1. 패킷 Flooding 어뷰징 감지 (TokenBucket 검사)
	if (!_rateLimiter.Consume(1))
	{
		AbuseMonitor::ReportAbuse(GetSessionId(), _playerName, ip, port, L"Packet Flooding Detected (Exceeded Rate Limit)");
		Disconnect(L"Packet Flooding Detected");
		return;
	}

	// 2. 정상 패킷 수신 시 하트비트 타임스탬프 갱신
	UpdateHeartbeat(::GetTickCount64());

	// 3. 자동 생성된 패킷 핸들러로 디스패치
	if (false == ServerPacketHandler::HandlePacket(GetPacketSessionRef(), buffer, len))
	{
		AbuseMonitor::ReportAbuse(GetSessionId(), _playerName, ip, port, L"Invalid Packet Framing / Malformed Data");
		Disconnect(L"Invalid Packet Format");
	}
}

void ClientSession::OnDisconnected()
{
	ServerStats::OnSessionDisconnected();

	// 접속 종료 시 현재 참가 중이던 방에서 자동 퇴장 처리 (JobQueue로 비동기 안전 처리)
	if (RoomRef room = GetRoom())
	{
		room->PushJob(room, &Room::Leave, GetClientSessionRef());
		SetRoom(nullptr);
	}
}
