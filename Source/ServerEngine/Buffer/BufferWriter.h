#pragma once
#include "Common/Types.h"
#include <string>
#include <vector>
#include <type_traits>

/*-------------------
    BufferWriter
--------------------*/
class BufferWriter
{
public:
	BufferWriter() = default;
	BufferWriter(BYTE* buffer, int32 size, int32 pos = 0)
		: _buffer(buffer), _size(size), _pos(pos) {}

	BYTE* Buffer() const { return _buffer; }
	int32 Size() const { return _size; }
	int32 WriteSize() const { return _pos; }
	int32 FreeSize() const { return _size - _pos; }

	template<typename T>
	bool Write(const T& src)
	{
		static_assert(!std::is_pointer_v<T>, "Cannot write pointer directly to buffer!");
		if (FreeSize() < sizeof(T))
			return false;
		::memcpy(&_buffer[_pos], &src, sizeof(T));
		_pos += sizeof(T);
		return true;
	}

	bool Write(const void* src, int32 len)
	{
		if (FreeSize() < len)
			return false;
		::memcpy(&_buffer[_pos], src, len);
		_pos += len;
		return true;
	}

	bool WriteString(const std::string& str)
	{
		uint16 len = static_cast<uint16>(str.size());
		if (Write(len) == false)
			return false;
		if (len > 0)
		{
			if (Write(str.data(), len) == false)
				return false;
		}
		return true;
	}

	template<typename T>
	T* Reserve()
	{
		if (FreeSize() < sizeof(T))
			return nullptr;
		T* ret = reinterpret_cast<T*>(&_buffer[_pos]);
		_pos += sizeof(T);
		return ret;
	}

	template<typename T>
	BufferWriter& operator<<(const T& src)
	{
		Write(src);
		return *this;
	}

private:
	BYTE* _buffer = nullptr;
	int32 _size = 0;
	int32 _pos = 0;
};
