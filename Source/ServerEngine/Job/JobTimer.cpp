#include "Job/JobTimer.h"
#include "Job/JobQueue.h"

SpinLock JobTimer::_lock;
std::priority_queue<TimerItem> JobTimer::_items;
std::atomic<bool> JobTimer::_distributing{ false };

void JobTimer::Reserve(uint64 afterMs, std::weak_ptr<JobQueue> owner, JobRef job)
{
	const uint64 executeTick = ::GetTickCount64() + afterMs;

	TimerItem item;
	item.executeTick = executeTick;
	item.owner = owner;
	item.job = job;

	SpinLockGuard guard(_lock);
	_items.push(item);
}

void JobTimer::Distribute(uint64 now)
{
	// 한 번에 하나의 스레드만 타이머 아이템 분배 수행
	if (_distributing.exchange(true))
		return;

	std::vector<TimerItem> expiredItems;

	{
		SpinLockGuard guard(_lock);
		while (!_items.empty())
		{
			const TimerItem& item = _items.top();
			if (item.executeTick > now)
				break;

			expiredItems.push_back(item);
			_items.pop();
		}
	}

	for (const TimerItem& item : expiredItems)
	{
		if (JobQueueRef owner = item.owner.lock())
		{
			owner->Push(item.job);
		}
	}

	_distributing.store(false);
}

void JobTimer::Clear()
{
	SpinLockGuard guard(_lock);
	while (!_items.empty())
		_items.pop();
}
