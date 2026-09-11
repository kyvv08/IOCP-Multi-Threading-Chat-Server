#pragma once
#include "Common/Types.h"
#include "Network/IocpEvent.h"
#include "Network/NetAddress.h"
#include <vector>

class ServerService;

/*------------------
    Listener
-------------------*/
class Listener : public IocpObject
{
public:
	Listener() = default;
	virtual ~Listener();

	bool StartAccept(std::shared_ptr<ServerService> service);
	void CloseSocket();

public:
	// IocpObject 인터페이스
	HANDLE GetHandle() override { return reinterpret_cast<HANDLE>(_listenSocket); }
	void Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

private:
	SOCKET _listenSocket = INVALID_SOCKET;
	std::vector<AcceptEvent*> _acceptEvents;
	std::shared_ptr<ServerService> _service;
};
