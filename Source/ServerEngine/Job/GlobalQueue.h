#pragma once
#include "Common/Types.h"
#include "Job/JobQueue.h"
#include "Lock/SpinLock.h"
#include <queue>

/*------------------
    GlobalQueue
-------------------*/
class GlobalQueue
{
public:
	static void Push(JobQueueRef jobQueue);
	static JobQueueRef Pop();
	static int32 GetCount();

private:
	static SpinLock _lock;
	static std::queue<JobQueueRef> _jobQueues;
};
