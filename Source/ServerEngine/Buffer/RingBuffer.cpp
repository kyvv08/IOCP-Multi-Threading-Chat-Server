#include "Buffer/RingBuffer.h"
#include "Memory/Memory.h"
#include "Common/Macros.h"
#include <algorithm>
#include <cstring>

RingBuffer::RingBuffer(int32 capacity) : _capacity(capacity), _readPos(0), _writePos(0)
{
	ASSERT_CRASH(capacity > 0);
	_buffer = static_cast<BYTE*>(PoolAllocator::Alloc(_capacity));
}

RingBuffer::~RingBuffer()
{
	if (_buffer)
	{
		PoolAllocator::Release(_buffer);
		_buffer = nullptr;
	}
}

void RingBuffer::Clear()
{
	_readPos = 0;
	_writePos = 0;
}

int32 RingBuffer::GetDataSize() const
{
	if (_writePos >= _readPos)
		return _writePos - _readPos;
	return _capacity - _readPos + _writePos;
}

int32 RingBuffer::GetFreeSize() const
{
	return _capacity - 1 - GetDataSize();
}

bool RingBuffer::Write(const BYTE* data, int32 size)
{
	if (size <= 0 || size > GetFreeSize())
		return false;

	if (_writePos + size <= _capacity)
	{
		std::memcpy(&_buffer[_writePos], data, size);
		_writePos = (_writePos + size) % _capacity;
	}
	else
	{
		int32 firstPart = _capacity - _writePos;
		std::memcpy(&_buffer[_writePos], data, firstPart);

		int32 secondPart = size - firstPart;
		std::memcpy(&_buffer[0], data + firstPart, secondPart);
		_writePos = secondPart;
	}

	return true;
}

bool RingBuffer::Read(BYTE* dest, int32 size)
{
	if (!Peek(dest, size))
		return false;

	return MoveReadPos(size);
}

bool RingBuffer::Peek(BYTE* dest, int32 size)
{
	if (size <= 0 || size > GetDataSize())
		return false;

	if (_readPos + size <= _capacity)
	{
		std::memcpy(dest, &_buffer[_readPos], size);
	}
	else
	{
		int32 firstPart = _capacity - _readPos;
		std::memcpy(dest, &_buffer[_readPos], firstPart);

		int32 secondPart = size - firstPart;
		std::memcpy(dest + firstPart, &_buffer[0], secondPart);
	}

	return true;
}

int32 RingBuffer::GetWriteSpan(WSABUF outWsabufs[2], int32 maxBytes)
{
	int32 freeSize = GetFreeSize();
	if (maxBytes > 0)
		freeSize = std::min(freeSize, maxBytes);

	if (freeSize <= 0)
		return 0;

	int32 spanCount = 0;

	if (_writePos >= _readPos)
	{
		int32 tailFree = _capacity - _writePos;
		if (_readPos == 0)
			tailFree -= 1; // 1바이트 갭 유지

		tailFree = std::min(tailFree, freeSize);

		if (tailFree > 0)
		{
			outWsabufs[0].buf = reinterpret_cast<CHAR*>(&_buffer[_writePos]);
			outWsabufs[0].len = tailFree;
			spanCount = 1;
			freeSize -= tailFree;
		}

		if (freeSize > 0 && _readPos > 0)
		{
			int32 headFree = std::min(_readPos - 1, freeSize);
			if (headFree > 0)
			{
				outWsabufs[1].buf = reinterpret_cast<CHAR*>(&_buffer[0]);
				outWsabufs[1].len = headFree;
				spanCount = 2;
			}
		}
	}
	else
	{
		int32 directFree = std::min(_readPos - _writePos - 1, freeSize);
		if (directFree > 0)
		{
			outWsabufs[0].buf = reinterpret_cast<CHAR*>(&_buffer[_writePos]);
			outWsabufs[0].len = directFree;
			spanCount = 1;
		}
	}

	return spanCount;
}

int32 RingBuffer::GetReadSpan(WSABUF outWsabufs[2], int32 maxBytes)
{
	int32 dataSize = GetDataSize();
	if (maxBytes > 0)
		dataSize = std::min(dataSize, maxBytes);

	if (dataSize <= 0)
		return 0;

	int32 spanCount = 0;

	if (_writePos >= _readPos)
	{
		outWsabufs[0].buf = reinterpret_cast<CHAR*>(&_buffer[_readPos]);
		outWsabufs[0].len = dataSize;
		spanCount = 1;
	}
	else
	{
		int32 tailData = _capacity - _readPos;
		tailData = std::min(tailData, dataSize);

		outWsabufs[0].buf = reinterpret_cast<CHAR*>(&_buffer[_readPos]);
		outWsabufs[0].len = tailData;
		spanCount = 1;
		dataSize -= tailData;

		if (dataSize > 0 && _writePos > 0)
		{
			int32 headData = std::min(_writePos, dataSize);
			if (headData > 0)
			{
				outWsabufs[1].buf = reinterpret_cast<CHAR*>(&_buffer[0]);
				outWsabufs[1].len = headData;
				spanCount = 2;
			}
		}
	}

	return spanCount;
}

bool RingBuffer::MoveWritePos(int32 size)
{
	if (size <= 0 || size > GetFreeSize())
		return false;

	_writePos = (_writePos + size) % _capacity;
	return true;
}

bool RingBuffer::MoveReadPos(int32 size)
{
	if (size <= 0 || size > GetDataSize())
		return false;

	_readPos = (_readPos + size) % _capacity;
	return true;
}
