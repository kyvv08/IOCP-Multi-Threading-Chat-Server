#include "Thread/ThreadManager.h"
#include "Memory/Memory.h"

thread_local uint32 LThreadId = 0;

std::mutex ThreadManager::_lock;
std::vector<std::thread> ThreadManager::_threads;
std::atomic<uint32> ThreadManager::_threadCount = 0;
std::atomic<uint32> ThreadManager::_nextThreadId = 1;

void ThreadManager::InitTLS()
{
	LThreadId = _nextThreadId.fetch_add(1);
	PoolAllocator::Init();
}

void ThreadManager::DestroyTLS()
{
}

void ThreadManager::Launch(std::function<void()> callback)
{
	std::lock_guard<std::mutex> guard(_lock);

	_threads.emplace_back([callback]()
	{
		InitTLS();
		_threadCount.fetch_add(1);

		callback();

		_threadCount.fetch_sub(1);
		DestroyTLS();
	});
}

void ThreadManager::Join()
{
	for (std::thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}
