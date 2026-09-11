#pragma once
#include "Common/Types.h"
#include "Memory/MemoryHeader.h"

/*-------------------
    MemoryPool
--------------------*/
class MemoryPool
{
public:
	explicit MemoryPool(int32 allocSize);
	~MemoryPool();

	MemoryPool(const MemoryPool&) = delete;
	MemoryPool& operator=(const MemoryPool&) = delete;

	void Push(MemoryHeader* ptr);
	MemoryHeader* Pop();

	int32 GetAllocSize() const { return _allocSize; }
	int32 GetAllocCount() const { return _allocCount.load(); }
	int32 GetUseCount() const { return _useCount.load(); }

private:
	SLIST_HEADER _header;
	int32 _allocSize = 0;
	std::atomic<int32> _allocCount = 0;
	std::atomic<int32> _useCount = 0;
};
