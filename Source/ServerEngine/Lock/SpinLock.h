#pragma once
#include "Common/Types.h"
#include <immintrin.h>

/*----------------
    SpinLock
-----------------*/
class SpinLock
{
private:
	std::atomic<bool> _locked;
public:
	SpinLock() : _locked(false) {}
	~SpinLock() = default;

	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;

	void Lock()
	{
		// 1차 CAS 시도 (경합이 없을 때 즉시 획득)
		if (!_locked.exchange(true, std::memory_order_acquire))
			return;

		// 경합 발생 시 Exponential Backoff를 적용하여 CPU 버스 경합 최소화
		uint32 spinCount = 0;
		while (true)
		{
			// Test and Test-and-Set: 캐시 라인 무효화를 막기 위해 읽기 전용으로 대기
			while (_locked.load(std::memory_order_relaxed))
			{
				_mm_pause(); // 하이퍼스레딩 파이프라인 지연 및 전력 절감
				++spinCount;

				if (spinCount > 2048)
				{
					std::this_thread::yield();
					spinCount = 0;
				}
			}

			if (!_locked.exchange(true, std::memory_order_acquire))
				return;
		}
	}

	bool TryLock()
	{
		return !_locked.exchange(true, std::memory_order_acquire);
	}

	void Unlock()
	{
		_locked.store(false, std::memory_order_release);
	}
};

/*-------------------
    SpinLockGuard
--------------------*/
class SpinLockGuard
{
public:
	explicit SpinLockGuard(SpinLock& lock) : _lock(lock)
	{
		_lock.Lock();
	}

	~SpinLockGuard()
	{
		_lock.Unlock();
	}

	SpinLockGuard(const SpinLockGuard&) = delete;
	SpinLockGuard& operator=(const SpinLockGuard&) = delete;

private:
	SpinLock& _lock;
};

/*-------------------
    SRWLockWrapper
--------------------*/
class ReadWriteLock
{
public:
	ReadWriteLock()
	{
		InitializeSRWLock(&_srwLock);
	}

	~ReadWriteLock() = default;

	void LockWrite()
	{
		AcquireSRWLockExclusive(&_srwLock);
	}

	void UnlockWrite()
	{
		ReleaseSRWLockExclusive(&_srwLock);
	}

	void LockRead()
	{
		AcquireSRWLockShared(&_srwLock);
	}

	void UnlockRead()
	{
		ReleaseSRWLockShared(&_srwLock);
	}

private:
	SRWLOCK _srwLock;
};

class ReadLockGuard
{
public:
	explicit ReadLockGuard(ReadWriteLock& lock) : _lock(lock) { _lock.LockRead(); }
	~ReadLockGuard() { _lock.UnlockRead(); }
private:
	ReadWriteLock& _lock;
};

class WriteLockGuard
{
public:
	explicit WriteLockGuard(ReadWriteLock& lock) : _lock(lock) { _lock.LockWrite(); }
	~WriteLockGuard() { _lock.UnlockWrite(); }
private:
	ReadWriteLock& _lock;
};
