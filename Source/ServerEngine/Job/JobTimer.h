#pragma once
#include "Common/Types.h"
#include "Job/Job.h"
#include "Lock/SpinLock.h"
#include <queue>

class JobQueue;

struct TimerItem
{
	bool operator<(const TimerItem& other) const
	{
		return executeTick > other.executeTick; // Min-Heap
	}

	uint64 executeTick = 0;
	std::weak_ptr<JobQueue> owner;
	JobRef job;
};

/*----------------
    JobTimer
-----------------*/
class JobTimer
{
public:
	static void Reserve(uint64 afterMs, std::weak_ptr<JobQueue> owner, JobRef job);
	static void Distribute(uint64 now);
	static void Clear();

private:
	static SpinLock _lock;
	static std::priority_queue<TimerItem> _items;
	static std::atomic<bool> _distributing;
};
