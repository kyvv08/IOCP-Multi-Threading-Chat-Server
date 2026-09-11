#pragma once
#include "Common/Types.h"
#include "Job/Job.h"
#include "Lock/SpinLock.h"
#include <queue>

/*----------------
    JobQueue
-----------------*/
class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
	JobQueue() = default;
	virtual ~JobQueue() = default;

	void Push(JobRef job, bool pushOnly = false);
	void Execute();
	void Clear();

	// 람다 기반 편리한 잡 등록
	void PushJob(CallbackType&& callback)
	{
		Push(std::make_shared<Job>(std::move(callback)));
	}

	template<typename T, typename Ret, typename... FArgs, typename... Args>
	void PushJob(std::shared_ptr<T> owner, Ret(T::*memFunc)(FArgs...), Args&&... args)
	{
		Push(std::make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	void DoTimer(uint64 afterMs, CallbackType&& callback);

	template<typename T, typename Ret, typename... FArgs, typename... Args>
	void DoTimer(uint64 afterMs, std::shared_ptr<T> owner, Ret(T::*memFunc)(FArgs...), Args&&... args)
	{
		DoTimer(afterMs, [owner, memFunc, args = std::make_tuple(std::forward<Args>(args)...)]() mutable
		{
			std::apply([&](auto&&... unpackedArgs)
			{
				(owner.get()->*memFunc)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
			}, std::move(args));
		});
	}

private:
	SpinLock _lock;
	std::queue<JobRef> _jobs;
	std::atomic<int32> _jobCount = 0;
};

using JobQueueRef = std::shared_ptr<JobQueue>;
