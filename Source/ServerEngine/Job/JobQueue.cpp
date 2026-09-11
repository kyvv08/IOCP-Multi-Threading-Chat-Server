#include "Job/JobQueue.h"
#include "Job/GlobalQueue.h"
#include "Job/JobTimer.h"

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);

	{
		SpinLockGuard guard(_lock);
		_jobs.push(job);
	}

	// 기존에 대기 중인 일감이 없었을 때만 GlobalQueue에 등록
	if (prevCount == 0)
	{
		if (!pushOnly)
		{
			GlobalQueue::Push(shared_from_this());
		}
	}
}

void JobQueue::Execute()
{
	const int32 maxExecuteCount = 64; // 한 번에 실행할 일감 상한 (스레드 독점 방지)
	std::vector<JobRef> executeList;

	{
		SpinLockGuard guard(_lock);
		while (!_jobs.empty() && static_cast<int32>(executeList.size()) < maxExecuteCount)
		{
			executeList.push_back(_jobs.front());
			_jobs.pop();
		}
	}

	for (const auto& job : executeList)
	{
		job->Execute();
	}

	const int32 executedCount = static_cast<int32>(executeList.size());
	const int32 remain = _jobCount.fetch_sub(executedCount) - executedCount;

	if (remain > 0)
	{
		// 아직 남은 일감이 있으면 다시 GlobalQueue에 인큐하여 워커 스레드들이 이어서 처리하도록 분산
		GlobalQueue::Push(shared_from_this());
	}
}

void JobQueue::Clear()
{
	SpinLockGuard guard(_lock);
	while (!_jobs.empty())
		_jobs.pop();
	_jobCount.store(0);
}

void JobQueue::DoTimer(uint64 afterMs, CallbackType&& callback)
{
	JobRef job = std::make_shared<Job>(std::move(callback));
	JobTimer::Reserve(afterMs, shared_from_this(), job);
}
