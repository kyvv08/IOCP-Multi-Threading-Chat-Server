#pragma once

#define CRASH(cause)						\
{											\
	uint32* crashVal = nullptr;				\
	__analysis_assume(crashVal != nullptr);	\
	*crashVal = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}

// Custom Memory Allocation Macros (연결 전 기본 매크로)
#define xalloc(size)				PoolAllocator::Alloc(size)
#define xrelease(ptr)				PoolAllocator::Release(ptr)

template<typename Type, typename... Args>
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(PoolAllocator::Alloc(sizeof(Type)));
	new(memory)Type(std::forward<Args>(args)...);
	return memory;
}

template<typename Type>
void xdelete(Type* obj)
{
	if (obj != nullptr)
	{
		obj->~Type();
		PoolAllocator::Release(obj);
	}
}

template<typename Type, typename... Args>
std::shared_ptr<Type> xmake_shared(Args&&... args)
{
	return std::shared_ptr<Type>(xnew<Type>(std::forward<Args>(args)...), xdelete<Type>);
}
