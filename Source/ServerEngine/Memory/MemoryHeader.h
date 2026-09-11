#pragma once
#include "Common/Types.h"

/*-------------------
    MemoryHeader
--------------------*/
struct DECLSPEC_ALIGN(16) MemoryHeader : public SLIST_ENTRY
{
	uint32 allocSize = 0;
	uint32 magic = 0xABCD1234;

	MemoryHeader(uint32 size) : allocSize(size), magic(0xABCD1234) {}

	static void* AttachHeader(MemoryHeader* header, uint32 size)
	{
		new(header)MemoryHeader(size);
		return reinterpret_cast<void*>(header + 1);
	}

	static MemoryHeader* DetachHeader(void* ptr)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}
};
