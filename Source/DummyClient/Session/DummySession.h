#pragma once
#include "ServerEngine.h"
#include <string>
#include <atomic>

enum class BotScenario
{
	Normal,         // 정상 챗봇 (로그인 -> 룸 입장 -> 주기적 채팅 및 하트비트)
	Slowloris,      // 아이들 봇 (접속 후 무응답 -> 서버 하트비트 타임아웃 감지 검증)
	ConnectSpam,    // 접속/해제 난사 봇 (초고속 Connect/Disconnect 반복 -> 소켓 풀링 검증)
	PacketFlood     // 패킷 플러딩 봇 (초당 80+ 패킷 폭주 -> 토큰 버킷 차단 검증)
};

struct StressStats
{
	static std::atomic<int32> activeSessions;
	static std::atomic<int32> totalConnected;
	static std::atomic<int32> totalDisconnected;
	static std::atomic<int32> abuseDisconnected;
	static std::atomic<uint64> totalSentPackets;
	static std::atomic<uint64> totalRecvPackets;

	static void Reset()
	{
		activeSessions.store(0);
		totalConnected.store(0);
		totalDisconnected.store(0);
		abuseDisconnected.store(0);
		totalSentPackets.store(0);
		totalRecvPackets.store(0);
	}
};

/*-------------------
    DummySession
--------------------*/
class DummySession : public PacketSession
{
public:
	DummySession(BotScenario scenario = BotScenario::Normal, int32 botIndex = 0);
	virtual ~DummySession() = default;

	BotScenario GetScenario() const { return _scenario; }
	int32 GetBotIndex() const { return _botIndex; }

	void SetLoggedIn(bool val) { _isLoggedIn.store(val); }
	bool IsLoggedIn() const { return _isLoggedIn.load(); }

	void SetInRoom(bool val) { _isInRoom.store(val); }
	bool IsInRoom() const { return _isInRoom.load(); }

	void SendChat(const std::string& msg);
	void SendEnterRoom(const std::string& msg, int);
	void SendHeartbeat();
	void ExecuteFloodAttack(int32 count = 80);

	std::shared_ptr<DummySession> GetDummySessionRef()
	{
		return std::static_pointer_cast<DummySession>(shared_from_this());
	}

protected:
	void OnConnected() override;
	void OnRecvPacket(BYTE* buffer, int32 len) override;
	void OnDisconnected() override;

private:
	BotScenario _scenario;
	int32 _botIndex;
	std::string _botName;
	std::atomic<bool> _isLoggedIn{ false };
	std::atomic<bool> _isInRoom{ false };
	uint64 _connectedTick = 0;
};

using DummySessionRef = std::shared_ptr<DummySession>;
