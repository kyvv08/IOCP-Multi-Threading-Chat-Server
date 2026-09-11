#include "Thread/ThreadPool.h"
#include "Thread/ThreadManager.h"
#include "Memory/Memory.h"
#include <algorithm>

ThreadPool::ThreadPool(uint32 threadCount)
{
	if (threadCount > 0)
		Start(threadCount);
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start(uint32 threadCount)
{
	if (!_workers.empty())
		return;

	_stop = false;

	if (threadCount == 0)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		_threadCount = sysInfo.dwNumberOfProcessors;
		if (_threadCount == 0)
			_threadCount = 4;
	}
	else
	{
		_threadCount = threadCount;
	}

	_workers.reserve(_threadCount);
	for (uint32 i = 0; i < _threadCount; ++i)
	{
		_workers.emplace_back(&ThreadPool::WorkerLoop, this);
	}
}

void ThreadPool::Stop()
{
	{
		std::unique_lock<std::mutex> lock(_queueLock);
		if (_stop)
			return;
		_stop = true;
	}

	_cv.notify_all();

	for (std::thread& worker : _workers)
	{
		if (worker.joinable())
			worker.join();
	}

	_workers.clear();
}

void ThreadPool::Enqueue(std::function<void()> task)
{
	{
		std::unique_lock<std::mutex> lock(_queueLock);
		if (_stop)
			return;
		_tasks.push(std::move(task));
	}

	_cv.notify_one();
}

size_t ThreadPool::GetPendingTaskCount()
{
	std::unique_lock<std::mutex> lock(_queueLock);
	return _tasks.size();
}

void ThreadPool::WorkerLoop()
{
	// TLS 초기화
	PoolAllocator::Init();

	while (true)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(_queueLock);
			_cv.wait(lock, [this]() { return _stop || !_tasks.empty(); });

			if (_stop && _tasks.empty())
				return;

			task = std::move(_tasks.front());
			_tasks.pop();
		}

		_busyThreads.fetch_add(1, std::memory_order_relaxed);
		task();
		_busyThreads.fetch_sub(1, std::memory_order_relaxed);
	}
}
