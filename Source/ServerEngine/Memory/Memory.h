#pragma once
#include "Common/Types.h"
#include "Memory/MemoryHeader.h"
#include <vector>

class MemoryPool;

/*-------------------
    PoolAllocator
--------------------*/
class PoolAllocator
{
	enum
	{
		POOL_COUNT = (1024 / 32) + (2048 / 128) + (4096 / 256),
		MAX_ALLOC_SIZE = 4096
	};

public:
	static void Init();
	static void Clear();

	static void* Alloc(int32 size);
	static void Release(void* ptr);

	static int32 GetTotalAllocCount();
	static int32 GetTotalUseCount();

private:
	static std::vector<MemoryPool*> _pools;
	static MemoryPool* _poolTable[MAX_ALLOC_SIZE + 1];
	static bool _initialized;
};
