#include "Memory/MemoryPool.h"
#include "Common/Macros.h"
#include <cstdlib>

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	InitializeSListHead(&_header);
}

MemoryPool::~MemoryPool()
{
	while (PSLIST_ENTRY entry = InterlockedPopEntrySList(&_header))
	{
		MemoryHeader* header = static_cast<MemoryHeader*>(entry);
		_aligned_free(header);
	}
}

void MemoryPool::Push(MemoryHeader* ptr)
{
	ASSERT_CRASH(ptr != nullptr);
	ASSERT_CRASH(ptr->magic == 0xABCD1234);

	_useCount.fetch_sub(1, std::memory_order_relaxed);
	InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));
}

MemoryHeader* MemoryPool::Pop()
{
	PSLIST_ENTRY entry = InterlockedPopEntrySList(&_header);
	MemoryHeader* header = static_cast<MemoryHeader*>(entry);

	if (header == nullptr)
	{
		// 풀에 가용 블록이 없으면 16바이트 정렬 메모리 새로 할당
		int32 totalSize = sizeof(MemoryHeader) + _allocSize;
		header = static_cast<MemoryHeader*>(_aligned_malloc(totalSize, 16));
		ASSERT_CRASH(header != nullptr);

		new(header)MemoryHeader(_allocSize);
		_allocCount.fetch_add(1, std::memory_order_relaxed);
	}

	_useCount.fetch_add(1, std::memory_order_relaxed);
	return header;
}
