#pragma once
#include "Common/Types.h"
#include "Memory/Memory.h"
#include <utility>

/*-------------------
    ObjectPool
--------------------*/
template<typename Type>
class ObjectPool
{
public:
	template<typename... Args>
	static Type* Pop(Args&&... args)
	{
		Type* memory = static_cast<Type*>(PoolAllocator::Alloc(sizeof(Type)));
		new(memory)Type(std::forward<Args>(args)...);
		return memory;
	}

	static void Push(Type* obj)
	{
		if (obj != nullptr)
		{
			obj->~Type();
			PoolAllocator::Release(obj);
		}
	}

	template<typename... Args>
	static std::shared_ptr<Type> MakeShared(Args&&... args)
	{
		return std::shared_ptr<Type>(Pop(std::forward<Args>(args)...), Push);
	}
};
