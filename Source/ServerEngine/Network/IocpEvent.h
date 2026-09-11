#pragma once
#include "Common/Types.h"
#include "Buffer/SendBuffer.h"
#include <vector>

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

/*------------------
    IocpObject
-------------------*/
class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	virtual ~IocpObject() = default;
	virtual HANDLE GetHandle() = 0;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) = 0;
};

using IocpObjectRef = std::shared_ptr<IocpObject>;

/*------------------
    IocpEvent
-------------------*/
class IocpEvent : public OVERLAPPED
{
public:
	explicit IocpEvent(EventType type);

	void Init();

public:
	EventType eventType;
	IocpObjectRef owner;
};

/*-------------------
    ConnectEvent
--------------------*/
class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) {}
};

/*----------------------
    DisconnectEvent
-----------------------*/
class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) {}
};

/*-------------------
    AcceptEvent
--------------------*/
class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) {}

public:
	SessionRef session = nullptr;
};

/*-------------------
    RecvEvent
--------------------*/
class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) {}
};

/*-------------------
    SendEvent
--------------------*/
class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) {}

public:
	std::vector<SendBufferRef> sendBuffers;
};
