#include "Network/IocpCore.h"
#include "Common/Macros.h"

IocpCore::IocpCore()
{
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	if (_iocpHandle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(_iocpHandle);
		_iocpHandle = INVALID_HANDLE_VALUE;
	}
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0) != NULL;
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocpHandle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		if (iocpEvent != nullptr)
		{
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpEvent->owner = nullptr;
			if (iocpObject)
				iocpObject->Dispatch(iocpEvent, numOfBytes);
		}
		return true;
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			if (iocpEvent != nullptr)
			{
				IocpObjectRef iocpObject = iocpEvent->owner;
				iocpEvent->owner = nullptr;
				if (iocpObject)
					iocpObject->Dispatch(iocpEvent, numOfBytes);
			}
			return false;
		}
	}
}
