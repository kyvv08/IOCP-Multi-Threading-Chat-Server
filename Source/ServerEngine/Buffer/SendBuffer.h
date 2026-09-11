#pragma once
#include "Common/Types.h"
#include <vector>

class SendBufferChunk;

/*-------------------
    SendBuffer
--------------------*/
class SendBuffer : public std::enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, int32 allocSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer; }
	int32 AllocSize() const { return _allocSize; }
	int32 WriteSize() const { return _writeSize; }
	void Close(int32 writeSize);

private:
	BYTE* _buffer;
	int32 _allocSize = 0;
	int32 _writeSize = 0;
	SendBufferChunkRef _owner;
};

/*----------------------
    SendBufferChunk
-----------------------*/
class SendBufferChunk : public std::enable_shared_from_this<SendBufferChunk>
{
	enum { CHUNK_SIZE = 0x10000 }; // 64KB

public:
	SendBufferChunk();
	~SendBufferChunk();

	void Reset();
	SendBufferRef Open(int32 allocSize);
	void Close(int32 writeSize);

	bool IsOpen() const { return _open; }
	BYTE* Buffer() { return &_buffer[_usedSize]; }
	int32 FreeSize() const { return static_cast<int32>(_buffer.size()) - _usedSize; }

private:
	std::vector<BYTE> _buffer;
	bool _open = false;
	int32 _usedSize = 0;
};

/*------------------------
    SendBufferManager
-------------------------*/
class SendBufferManager
{
public:
	static SendBufferRef Open(int32 size);
	static void Close(SendBufferRef sendBuffer, int32 writeSize);

private:
	static SendBufferChunkRef Pop();
	static void Push(SendBufferChunkRef chunk);
};
