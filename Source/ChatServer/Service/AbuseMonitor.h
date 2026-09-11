#pragma once
#include "ServerEngine.h"
#include <string>
#include <vector>

struct AbuseLogEntry
{
	uint64 sessionId = 0;
	std::string playerName;
	std::string ipAddress;
	uint16 port = 0;
	std::wstring reason;
	uint64 timestamp = 0;
	std::string timeStr;
};

/*-------------------
    AbuseMonitor
--------------------*/
class AbuseMonitor
{
public:
	static void Init();

	// 비정상 유저 제재 및 사유 기록
	static void ReportAbuse(uint64 sessionId, const std::string& playerName, const std::string& ipAddress, uint16 port, const std::wstring& reason);

	// 전체 세션 대상 하트비트(Keep-Alive) 타임아웃 검사
	static void CheckHeartbeatTimeouts(ServerServiceRef service, uint64 timeoutMs = 15000);

	// 통계 데이터 및 최근 로그 조회
	static uint32 GetAbuseCount() { return _abuseCount.load(); }
	static std::vector<AbuseLogEntry> GetLogs();
	static std::vector<AbuseLogEntry> GetRecentLogs(size_t maxCount = 5);

	// 로그 파일 덤프 출력 (Req #11 연동)
	static bool DumpLogsToFile(const std::string& filePath = "Abuse_Disconnect_Log.txt");

private:
	static std::string GetCurrentTimeString();

private:
	static SpinLock _lock;
	static std::vector<AbuseLogEntry> _logs;
	static std::atomic<uint32> _abuseCount;
};
