#include "Buffer/SendBuffer.h"
#include "Common/Macros.h"
#include "Lock/SpinLock.h"
#include <vector>

namespace
{
	thread_local SendBufferChunkRef LSendBufferChunk;
	SpinLock GChunkLock;
	std::vector<SendBufferChunkRef> GChunkPool;
}

/*-------------------
    SendBuffer
--------------------*/
SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, int32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(int32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*----------------------
    SendBufferChunk
-----------------------*/
SendBufferChunk::SendBufferChunk()
{
	_buffer.resize(CHUNK_SIZE);
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(int32 allocSize)
{
	ASSERT_CRASH(allocSize > 0 && allocSize <= CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize())
		return nullptr;

	_open = true;
	return std::make_shared<SendBuffer>(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(int32 writeSize)
{
	ASSERT_CRASH(_open == true);
	_open = false;
	_usedSize += writeSize;
}

/*------------------------
    SendBufferManager
-------------------------*/
SendBufferRef SendBufferManager::Open(int32 size)
{
	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	return LSendBufferChunk->Open(size);
}

void SendBufferManager::Close(SendBufferRef sendBuffer, int32 writeSize)
{
	sendBuffer->Close(writeSize);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	SpinLockGuard guard(GChunkLock);
	if (!GChunkPool.empty())
	{
		SendBufferChunkRef chunk = GChunkPool.back();
		GChunkPool.pop_back();
		return chunk;
	}

	return std::make_shared<SendBufferChunk>();
}

void SendBufferManager::Push(SendBufferChunkRef chunk)
{
	SpinLockGuard guard(GChunkLock);
	GChunkPool.push_back(chunk);
}
