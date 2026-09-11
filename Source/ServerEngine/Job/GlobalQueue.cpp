#include "Job/GlobalQueue.h"

SpinLock GlobalQueue::_lock;
std::queue<JobQueueRef> GlobalQueue::_jobQueues;

void GlobalQueue::Push(JobQueueRef jobQueue)
{
	SpinLockGuard guard(_lock);
	_jobQueues.push(jobQueue);
}

JobQueueRef GlobalQueue::Pop()
{
	SpinLockGuard guard(_lock);
	if (_jobQueues.empty())
		return nullptr;

	JobQueueRef jobQueue = _jobQueues.front();
	_jobQueues.pop();
	return jobQueue;
}

int32 GlobalQueue::GetCount()
{
	SpinLockGuard guard(_lock);
	return static_cast<int32>(_jobQueues.size());
}
