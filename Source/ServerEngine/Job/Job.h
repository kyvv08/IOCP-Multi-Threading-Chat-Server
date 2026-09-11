#pragma once
#include "Common/Types.h"
#include <functional>
#include <tuple>

using CallbackType = std::function<void()>;

/*------------
    Job
-------------*/
class Job
{
public:
	Job(CallbackType&& callback) : _callback(std::move(callback)) {}

	template<typename T, typename Ret, typename... FArgs, typename... Args>
	Job(std::shared_ptr<T> owner, Ret(T::*memFunc)(FArgs...), Args&&... args)
	{
		_callback = [owner, memFunc, args = std::make_tuple(std::forward<Args>(args)...)]() mutable
		{
			std::apply([&](auto&&... unpackedArgs)
			{
				(owner.get()->*memFunc)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
			}, std::move(args));
		};
	}

	void Execute()
	{
		if (_callback)
			_callback();
	}

private:
	CallbackType _callback;
};

using JobRef = std::shared_ptr<Job>;
