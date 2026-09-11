#pragma once
#include "Common/Types.h"
#include <string>
#include <vector>
#include <type_traits>

/*-------------------
    BufferReader
--------------------*/
class BufferReader
{
public:
	BufferReader() = default;
	BufferReader(BYTE* buffer, int32 size, int32 pos = 0)
		: _buffer(buffer), _size(size), _pos(pos) {}

	BYTE* Buffer() const { return _buffer; }
	int32 Size() const { return _size; }
	int32 ReadSize() const { return _pos; }
	int32 FreeSize() const { return _size - _pos; }
	bool IsValid() const { return _isValid; }

	template<typename T>
	bool Peek(T& dest)
	{
		static_assert(!std::is_pointer_v<T>, "Cannot peek pointer directly");
		if (FreeSize() < sizeof(T))
		{
			_isValid = false;
			return false;
		}
		::memcpy(&dest, &_buffer[_pos], sizeof(T));
		return true;
	}

	template<typename T>
	bool Read(T& dest)
	{
		if (Peek(dest) == false)
			return false;
		_pos += sizeof(T);
		return true;
	}

	template<typename T>
	T Read()
	{
		T ret{};
		Read(ret);
		return ret;
	}

	bool Read(void* dest, int32 len)
	{
		if (FreeSize() < len)
		{
			_isValid = false;
			return false;
		}
		::memcpy(dest, &_buffer[_pos], len);
		_pos += len;
		return true;
	}

	bool ReadString(std::string& str)
	{
		uint16 len = 0;
		if (Read(len) == false)
			return false;

		if (FreeSize() < len)
		{
			_isValid = false;
			return false;
		}

		str.resize(len);
		if (len > 0)
		{
			::memcpy(&str[0], &_buffer[_pos], len);
			_pos += len;
		}
		return true;
	}

	template<typename T>
	BufferReader& operator>>(T& dest)
	{
		Read(dest);
		return *this;
	}

private:
	BYTE* _buffer = nullptr;
	int32 _size = 0;
	int32 _pos = 0;
	bool _isValid = true;
};
