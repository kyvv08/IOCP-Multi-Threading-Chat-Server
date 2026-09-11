#pragma once
#include "Common/Types.h"
#include <vector>

/*-------------------
    RingBuffer
--------------------*/
class RingBuffer
{
public:
	explicit RingBuffer(int32 capacity = 0x10000); // 기본 64KB
	~RingBuffer();

	RingBuffer(const RingBuffer&) = delete;
	RingBuffer& operator=(const RingBuffer&) = delete;

	void Clear();

	// 데이터 크기 및 남은 여유 공간
	int32 GetDataSize() const;
	int32 GetFreeSize() const;
	int32 GetCapacity() const { return _capacity; }

	// 일반 복사 기반 Read / Write / Peek
	bool Write(const BYTE* data, int32 size);
	bool Read(BYTE* dest, int32 size);
	bool Peek(BYTE* dest, int32 size);

	// Zero-Copy Scatter-Gather I/O용 Span 계산 (WSARecv / WSASend 직접 전달)
	int32 GetWriteSpan(WSABUF outWsabufs[2], int32 maxBytes = 0);
	int32 GetReadSpan(WSABUF outWsabufs[2], int32 maxBytes = 0);

	// 비동기 I/O 완료 후 포인터 이동
	bool MoveWritePos(int32 size);
	bool MoveReadPos(int32 size);

	BYTE* GetBuffer() { return _buffer; }
	BYTE* GetWritePtr() { return &_buffer[_writePos]; }
	BYTE* GetReadPtr() { return &_buffer[_readPos]; }

private:
	BYTE* _buffer = nullptr;
	int32 _capacity = 0;
	int32 _readPos = 0;
	int32 _writePos = 0;
};
