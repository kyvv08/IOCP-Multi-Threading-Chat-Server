#pragma once
#include "Common/Types.h"
#include "Network/IocpEvent.h"

/*-------------------
    IocpCore
--------------------*/
class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE GetHandle() const { return _iocpHandle; }

	bool Register(IocpObjectRef iocpObject);
	bool Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle = INVALID_HANDLE_VALUE;
};
