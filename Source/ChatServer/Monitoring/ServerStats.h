#pragma once
#include "ServerEngine.h"
#include <string>

/*-------------------
    ServerStats
--------------------*/
class ServerStats
{
public:
	static void Init();

	// 연결 수 증감
	static void OnSessionConnected();
	static void OnSessionDisconnected();

	// 트래픽 카운트
	static void OnPacketReceived(int32 bytes);
	static void OnPacketSent(int32 bytes);

	// 어뷰징 카운트
	static void OnAbuseDisconnection();

	// 1초 주기로 초당 처리량(PPS, BPS) 계산
	static void UpdateRates();

	// 통계 수치 조회
	static uint64 GetTotalAccepted() { return _totalAccepted.load(std::memory_order_relaxed); }
	static int32 GetCurrentCcu() { return _currentCcu.load(std::memory_order_relaxed); }
	static uint64 GetTotalRecvPackets() { return _totalRecvPackets.load(std::memory_order_relaxed); }
	static uint64 GetTotalRecvBytes() { return _totalRecvBytes.load(std::memory_order_relaxed); }
	static uint64 GetTotalSendBytes() { return _totalSendBytes.load(std::memory_order_relaxed); }
	static uint32 GetTotalAbuseDisconnects() { return _totalAbuseDisconnects.load(std::memory_order_relaxed); }

	static uint32 GetCurrentPps() { return _currentPps.load(std::memory_order_relaxed); }
	static uint32 GetCurrentRecvBps() { return _currentRecvBps.load(std::memory_order_relaxed); }
	static uint32 GetCurrentSendBps() { return _currentSendBps.load(std::memory_order_relaxed); }

	static uint64 GetUptimeSeconds();
	static std::string GetFormattedUptime();

private:
	static std::atomic<uint64> _totalAccepted;
	static std::atomic<int32> _currentCcu;
	static std::atomic<uint64> _totalRecvPackets;
	static std::atomic<uint64> _totalRecvBytes;
	static std::atomic<uint64> _totalSendBytes;
	static std::atomic<uint32> _totalAbuseDisconnects;

	// 초당 변동량 측정을 위한 이전 틱 스냅샷
	static uint64 _lastRecvPackets;
	static uint64 _lastRecvBytes;
	static uint64 _lastSendBytes;
	static uint64 _lastRateTick;

	static std::atomic<uint32> _currentPps;
	static std::atomic<uint32> _currentRecvBps;
	static std::atomic<uint32> _currentSendBps;

	static uint64 _startTime;
	static SpinLock _lock;
};
