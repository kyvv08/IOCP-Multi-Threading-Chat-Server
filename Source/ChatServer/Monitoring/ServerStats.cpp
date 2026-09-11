#include "Monitoring/ServerStats.h"
#include <iomanip>
#include <sstream>

std::atomic<uint64> ServerStats::_totalAccepted{ 0 };
std::atomic<int32> ServerStats::_currentCcu{ 0 };
std::atomic<uint64> ServerStats::_totalRecvPackets{ 0 };
std::atomic<uint64> ServerStats::_totalRecvBytes{ 0 };
std::atomic<uint64> ServerStats::_totalSendBytes{ 0 };
std::atomic<uint32> ServerStats::_totalAbuseDisconnects{ 0 };

uint64 ServerStats::_lastRecvPackets = 0;
uint64 ServerStats::_lastRecvBytes = 0;
uint64 ServerStats::_lastSendBytes = 0;
uint64 ServerStats::_lastRateTick = 0;

std::atomic<uint32> ServerStats::_currentPps{ 0 };
std::atomic<uint32> ServerStats::_currentRecvBps{ 0 };
std::atomic<uint32> ServerStats::_currentSendBps{ 0 };

uint64 ServerStats::_startTime = 0;
SpinLock ServerStats::_lock;

void ServerStats::Init()
{
	_totalAccepted.store(0);
	_currentCcu.store(0);
	_totalRecvPackets.store(0);
	_totalRecvBytes.store(0);
	_totalSendBytes.store(0);
	_totalAbuseDisconnects.store(0);

	_lastRecvPackets = 0;
	_lastRecvBytes = 0;
	_lastSendBytes = 0;

	_currentPps.store(0);
	_currentRecvBps.store(0);
	_currentSendBps.store(0);

	_startTime = ::GetTickCount64();
	_lastRateTick = _startTime;
}

void ServerStats::OnSessionConnected()
{
	_totalAccepted.fetch_add(1, std::memory_order_relaxed);
	_currentCcu.fetch_add(1, std::memory_order_relaxed);
}

void ServerStats::OnSessionDisconnected()
{
	_currentCcu.fetch_sub(1, std::memory_order_relaxed);
}

void ServerStats::OnPacketReceived(int32 bytes)
{
	_totalRecvPackets.fetch_add(1, std::memory_order_relaxed);
	_totalRecvBytes.fetch_add(bytes, std::memory_order_relaxed);
}

void ServerStats::OnPacketSent(int32 bytes)
{
	_totalSendBytes.fetch_add(bytes, std::memory_order_relaxed);
}

void ServerStats::OnAbuseDisconnection()
{
	_totalAbuseDisconnects.fetch_add(1, std::memory_order_relaxed);
}

void ServerStats::UpdateRates()
{
	const uint64 now = ::GetTickCount64();
	SpinLockGuard guard(_lock);

	const uint64 elapsedMs = now - _lastRateTick;
	if (elapsedMs < 500)
		return;

	const uint64 curRecvPackets = _totalRecvPackets.load(std::memory_order_relaxed);
	const uint64 curRecvBytes = _totalRecvBytes.load(std::memory_order_relaxed);
	const uint64 curSendBytes = _totalSendBytes.load(std::memory_order_relaxed);

	const double elapsedSec = static_cast<double>(elapsedMs) / 1000.0;

	if (elapsedSec > 0.0)
	{
		uint32 pps = static_cast<uint32>((curRecvPackets - _lastRecvPackets) / elapsedSec);
		uint32 recvBps = static_cast<uint32>((curRecvBytes - _lastRecvBytes) / elapsedSec);
		uint32 sendBps = static_cast<uint32>((curSendBytes - _lastSendBytes) / elapsedSec);

		_currentPps.store(pps, std::memory_order_relaxed);
		_currentRecvBps.store(recvBps, std::memory_order_relaxed);
		_currentSendBps.store(sendBps, std::memory_order_relaxed);
	}

	_lastRecvPackets = curRecvPackets;
	_lastRecvBytes = curRecvBytes;
	_lastSendBytes = curSendBytes;
	_lastRateTick = now;
}

uint64 ServerStats::GetUptimeSeconds()
{
	if (_startTime == 0) return 0;
	return (::GetTickCount64() - _startTime) / 1000;
}

std::string ServerStats::GetFormattedUptime()
{
	uint64 totalSec = GetUptimeSeconds();
	uint64 hours = totalSec / 3600;
	uint64 mins = (totalSec % 3600) / 60;
	uint64 secs = totalSec % 60;

	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << hours << ":"
		<< std::setfill('0') << std::setw(2) << mins << ":"
		<< std::setfill('0') << std::setw(2) << secs;
	return ss.str();
}
