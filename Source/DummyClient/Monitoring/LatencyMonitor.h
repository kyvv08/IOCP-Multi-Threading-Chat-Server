#pragma once
#include "Common/Types.h"
#include <atomic>
#include <algorithm>
#include <chrono>

/*-----------------------
    LatencyMonitor
------------------------*/
class LatencyMonitor
{
public:
	// 0.0ms ~ 1000.0ms (0.1ms = 100us 단위, 10,000개 버킷) + 1000ms 초과 1개 버킷
	static constexpr int32 BUCKET_RESOLUTION_US = 100; // 0.1ms = 100us
	static constexpr int32 MAX_TRACKED_US = 1'000'000;  // 1,000ms = 1,000,000us
	static constexpr int32 NUM_BUCKETS = (MAX_TRACKED_US / BUCKET_RESOLUTION_US) + 1; // 10,001개

	static void Reset()
	{
		for (int32 i = 0; i < NUM_BUCKETS; ++i)
		{
			_buckets[i].store(0, std::memory_order_relaxed);
		}
		_totalCount.store(0, std::memory_order_relaxed);
		_totalLatencyUs.store(0, std::memory_order_relaxed);
		_minLatencyUs.store(UINT32_MAX, std::memory_order_relaxed);
		_maxLatencyUs.store(0, std::memory_order_relaxed);
	}

	// 샘플 기록 (마이크로초 단위 RTT 입력) - Lock-Free O(1)
	static void RecordSample(uint64 rttUs)
	{
		_totalCount.fetch_add(1, std::memory_order_relaxed);
		_totalLatencyUs.fetch_add(rttUs, std::memory_order_relaxed);

		// Min / Max 갱신 (CAS Loop)
		uint32 currentMin = _minLatencyUs.load(std::memory_order_relaxed);
		while (rttUs < currentMin && !_minLatencyUs.compare_exchange_weak(currentMin, static_cast<uint32>(rttUs), std::memory_order_relaxed))
		{
		}

		uint32 currentMax = _maxLatencyUs.load(std::memory_order_relaxed);
		while (rttUs > currentMax && !_maxLatencyUs.compare_exchange_weak(currentMax, static_cast<uint32>(rttUs), std::memory_order_relaxed))
		{
		}

		// 버킷 인덱스 계산
		int32 bucketIdx = static_cast<int32>(rttUs / BUCKET_RESOLUTION_US);
		if (bucketIdx >= NUM_BUCKETS)
			bucketIdx = NUM_BUCKETS - 1;

		_buckets[bucketIdx].fetch_add(1, std::memory_order_relaxed);
	}

	static uint64 GetTotalCount()
	{
		return _totalCount.load(std::memory_order_relaxed);
	}

	static double GetAverageMs()
	{
		uint64 count = _totalCount.load(std::memory_order_relaxed);
		if (count == 0)
			return 0.0;
		return static_cast<double>(_totalLatencyUs.load(std::memory_order_relaxed)) / (count * 1000.0);
	}

	static double GetMinMs()
	{
		uint32 minUs = _minLatencyUs.load(std::memory_order_relaxed);
		if (minUs == UINT32_MAX)
			return 0.0;
		return static_cast<double>(minUs) / 1000.0;
	}

	static double GetMaxMs()
	{
		return static_cast<double>(_maxLatencyUs.load(std::memory_order_relaxed)) / 1000.0;
	}

	// 백분위수 (p: 0.0 ~ 100.0, 예: 50.0, 95.0, 99.0)
	static double GetPercentileMs(double percentile)
	{
		uint64 total = _totalCount.load(std::memory_order_relaxed);
		if (total == 0)
			return 0.0;

		uint64 targetRank = static_cast<uint64>(total * (percentile / 100.0));
		if (targetRank == 0)
			targetRank = 1;

		uint64 accumulated = 0;
		for (int32 i = 0; i < NUM_BUCKETS; ++i)
		{
			accumulated += _buckets[i].load(std::memory_order_relaxed);
			if (accumulated >= targetRank)
			{
				// 해당 버킷의 중앙값(ms) 반환
				return (static_cast<double>(i * BUCKET_RESOLUTION_US) + (BUCKET_RESOLUTION_US / 2.0)) / 1000.0;
			}
		}

		return GetMaxMs();
	}

private:
	inline static std::atomic<uint32> _buckets[NUM_BUCKETS];
	inline static std::atomic<uint64> _totalCount{ 0 };
	inline static std::atomic<uint64> _totalLatencyUs{ 0 };
	inline static std::atomic<uint32> _minLatencyUs{ UINT32_MAX };
	inline static std::atomic<uint32> _maxLatencyUs{ 0 };
};
