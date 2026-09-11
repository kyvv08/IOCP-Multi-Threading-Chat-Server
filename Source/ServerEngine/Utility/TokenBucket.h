#pragma once
#include "Common/Types.h"
#include <algorithm>

/*-------------------
    TokenBucket
--------------------*/
class TokenBucket
{
public:
	// capacity: 최대 허용 버스트 토큰 수, refillRatePerSec: 초당 충전되는 토큰 수
	explicit TokenBucket(int32 capacity = 50, int32 refillRatePerSec = 20)
		: _capacity(capacity), _refillRatePerSec(refillRatePerSec), _tokens(capacity), _lastRefillTick(::GetTickCount64())
	{
	}

	bool Consume(int32 count = 1)
	{
		Refill();

		int32 currentTokens = _tokens.load(std::memory_order_relaxed);
		while (true)
		{
			if (currentTokens < count)
				return false; // 토큰 고갈 (플러딩 감지)

			if (_tokens.compare_exchange_weak(currentTokens, currentTokens - count, std::memory_order_release, std::memory_order_relaxed))
				return true;
		}
	}

	void Reset()
	{
		_tokens.store(_capacity, std::memory_order_relaxed);
		_lastRefillTick.store(::GetTickCount64(), std::memory_order_relaxed);
	}

private:
	void Refill()
	{
		const uint64 now = ::GetTickCount64();
		const uint64 last = _lastRefillTick.load(std::memory_order_relaxed);

		if (now <= last)
			return;

		const uint64 elapsedMs = now - last;

		if (elapsedMs < 50) // 50ms 미만은 충전 생략
			return;

		if (_lastRefillTick.compare_exchange_strong(const_cast<uint64&>(last), now, std::memory_order_release, std::memory_order_relaxed))
		{
			const int32 addTokens = static_cast<int32>((elapsedMs * _refillRatePerSec) / 1000);
			if (addTokens > 0)
			{
				int32 cur = _tokens.load(std::memory_order_relaxed);
				while (true)
				{
					int32 next = (std::min)(_capacity, cur + addTokens);
					if (_tokens.compare_exchange_weak(cur, next, std::memory_order_release, std::memory_order_relaxed))
						break;
				}
			}
		}
	}

private:
	int32 _capacity;
	int32 _refillRatePerSec;
	std::atomic<int32> _tokens;
	std::atomic<uint64> _lastRefillTick;
};
