#pragma once
#include "Common/Types.h"
#include <vector>
#include <thread>
#include <functional>
#include <mutex>

/*-------------------
    ThreadManager
--------------------*/
class ThreadManager
{
public:
	static void Launch(std::function<void()> callback);
	static void Join();

	static uint32 GetTotalThreadCount() { return _threadCount.load(); }

private:
	static void InitTLS();
	static void DestroyTLS();

private:
	static std::mutex _lock;
	static std::vector<std::thread> _threads;
	static std::atomic<uint32> _threadCount;
	static std::atomic<uint32> _nextThreadId;
};
