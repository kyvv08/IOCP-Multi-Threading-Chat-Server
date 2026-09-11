#include "Memory/Memory.h"
#include "Memory/MemoryPool.h"
#include "Common/Macros.h"
#include <cstdlib>

std::vector<MemoryPool*> PoolAllocator::_pools;
MemoryPool* PoolAllocator::_poolTable[MAX_ALLOC_SIZE + 1] = { nullptr };
bool PoolAllocator::_initialized = false;

namespace
{
	struct ThreadLocalMemoryCache
	{
		static constexpr int32 BATCH_SIZE = 16;
		static constexpr int32 MAX_LOCAL_CACHE = 48;
		std::vector<MemoryHeader*> cache[64];
	};

	thread_local ThreadLocalMemoryCache LMemoryCache;
}

void PoolAllocator::Init()
{
	if (_initialized)
		return;

	int32 currentPoolIdx = 0;

	// ~1024B: 32B 단위 (32, 64, 96, ..., 1024) -> 32개 풀
	for (int32 size = 32; size <= 1024; size += 32)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (currentPoolIdx <= size)
		{
			_poolTable[currentPoolIdx] = pool;
			++currentPoolIdx;
		}
	}

	// 1025B ~ 2048B: 128B 단위 (1152, 1280, ..., 2048) -> 8개 풀
	for (int32 size = 1024 + 128; size <= 2048; size += 128)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (currentPoolIdx <= size)
		{
			_poolTable[currentPoolIdx] = pool;
			++currentPoolIdx;
		}
	}

	// 2049B ~ 4096B: 256B 단위 (2304, 2560, ..., 4096) -> 8개 풀
	for (int32 size = 2048 + 256; size <= 4096; size += 256)
	{
		MemoryPool* pool = new MemoryPool(size);
		_pools.push_back(pool);

		while (currentPoolIdx <= size)
		{
			_poolTable[currentPoolIdx] = pool;
			++currentPoolIdx;
		}
	}

	_initialized = true;
}

void PoolAllocator::Clear()
{
	for (MemoryPool* pool : _pools)
	{
		delete pool;
	}
	_pools.clear();
	_initialized = false;
}

void* PoolAllocator::Alloc(int32 size)
{
	if (!_initialized)
		Init();

	if (size <= 0)
		return nullptr;

	// 4KB 초과 대형 할당은 OS 가상 메모리 직접 할당
	if (size > MAX_ALLOC_SIZE)
	{
		int32 totalSize = sizeof(MemoryHeader) + size;
		MemoryHeader* header = static_cast<MemoryHeader*>(_aligned_malloc(totalSize, 16));
		ASSERT_CRASH(header != nullptr);
		return MemoryHeader::AttachHeader(header, size);
	}

	MemoryPool* pool = _poolTable[size];
	ASSERT_CRASH(pool != nullptr);

	int32 allocSize = pool->GetAllocSize();
	int32 poolIdx = (allocSize <= 1024) ? ((allocSize / 32) - 1)
		: (allocSize <= 2048) ? (32 + ((allocSize - 1024) / 128) - 1)
		: (40 + ((allocSize - 2048) / 256) - 1);

	auto& localList = LMemoryCache.cache[poolIdx];

	if (!localList.empty())
	{
		MemoryHeader* header = localList.back();
		localList.pop_back();
		return MemoryHeader::AttachHeader(header, allocSize);
	}

	// TLS 캐시가 비었으면 글로벌 풀에서 팝
	MemoryHeader* header = pool->Pop();
	return MemoryHeader::AttachHeader(header, allocSize);
}

void PoolAllocator::Release(void* ptr)
{
	if (ptr == nullptr)
		return;

	MemoryHeader* header = MemoryHeader::DetachHeader(ptr);
	ASSERT_CRASH(header->magic == 0xABCD1234);

	int32 allocSize = header->allocSize;

	if (allocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
		return;
	}

	MemoryPool* pool = _poolTable[allocSize];
	ASSERT_CRASH(pool != nullptr);

	int32 poolIdx = (allocSize <= 1024) ? ((allocSize / 32) - 1)
		: (allocSize <= 2048) ? (32 + ((allocSize - 1024) / 128) - 1)
		: (40 + ((allocSize - 2048) / 256) - 1);

	auto& localList = LMemoryCache.cache[poolIdx];

	if (static_cast<int32>(localList.size()) < ThreadLocalMemoryCache::MAX_LOCAL_CACHE)
	{
		localList.push_back(header);
	}
	else
	{
		// TLS 캐시 상한 초과 시 절반을 글로벌 풀에 일괄 반납
		for (int32 i = 0; i < ThreadLocalMemoryCache::BATCH_SIZE; ++i)
		{
			MemoryHeader* flushHeader = localList.back();
			localList.pop_back();
			pool->Push(flushHeader);
		}
		localList.push_back(header);
	}
}

int32 PoolAllocator::GetTotalAllocCount()
{
	int32 count = 0;
	for (MemoryPool* pool : _pools)
		count += pool->GetAllocCount();
	return count;
}

int32 PoolAllocator::GetTotalUseCount()
{
	int32 count = 0;
	for (MemoryPool* pool : _pools)
		count += pool->GetUseCount();
	return count;
}
