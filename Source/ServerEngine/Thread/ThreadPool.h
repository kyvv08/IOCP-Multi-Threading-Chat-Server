#pragma once
#include "Common/Types.h"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/*-------------------
    ThreadPool
--------------------*/
class ThreadPool
{
public:
	explicit ThreadPool(uint32 threadCount = 0);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	void Start(uint32 threadCount = 0);
	void Stop();

	void Enqueue(std::function<void()> task);

	uint32 GetThreadCount() const { return _threadCount; }
	uint32 GetBusyThreadCount() const { return _busyThreads.load(); }
	size_t GetPendingTaskCount();

private:
	void WorkerLoop();

private:
	std::vector<std::thread> _workers;
	std::queue<std::function<void()>> _tasks;

	std::mutex _queueLock;
	std::condition_variable _cv;

	std::atomic<bool> _stop{ false };
	std::atomic<uint32> _busyThreads{ 0 };
	uint32 _threadCount = 0;
};
