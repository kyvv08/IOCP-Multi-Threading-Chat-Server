#include "Network/Listener.h"
#include "Network/SocketUtils.h"
#include "Network/Service.h"
#include "Network/Session.h"
#include "Memory/Memory.h"
#include "Common/Macros.h"

Listener::~Listener()
{
	CloseSocket();

	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		xdelete(acceptEvent);
	}
	_acceptEvents.clear();
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_listenSocket);
}

bool Listener::StartAccept(std::shared_ptr<ServerService> service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	_listenSocket = SocketUtils::CreateSocket();
	if (_listenSocket == INVALID_SOCKET)
		return false;

	if (false == SocketUtils::SetReuseAddress(_listenSocket, true))
		return false;

	if (false == SocketUtils::SetLinger(_listenSocket, 0, 0))
		return false;

	if (false == SocketUtils::Bind(_listenSocket, _service->GetNetAddress()))
		return false;

	if (false == SocketUtils::Listen(_listenSocket))
		return false;

	if (false == _service->GetIocpCore()->Register(shared_from_this()))
		return false;

	// 동시 대기 가능한 AcceptEvent 생성 (기본 16개 동시 대기)
	const int32 acceptCount = 16;
	for (int32 i = 0; i < acceptCount; ++i)
	{
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 /*numOfBytes*/)
{
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	if (_listenSocket == INVALID_SOCKET || _service == nullptr)
		return;

	SessionRef session = _service->CreateSession();

	acceptEvent->Init();
	acceptEvent->owner = shared_from_this();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;
	if (false == SocketUtils::AcceptEx(_listenSocket, session->GetSocket(),
		session->_recvBuffer.GetBuffer(), 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 소켓이 닫혔거나 취소된 경우가 아닐 때만 재등록 시도
			if (errorCode != WSAEINTR && errorCode != WSAENOTSOCK && errorCode != ERROR_OPERATION_ABORTED && _listenSocket != INVALID_SOCKET)
			{
				RegisterAccept(acceptEvent);
			}
		}
	}
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	if (_listenSocket == INVALID_SOCKET || _service == nullptr)
		return;

	SessionRef session = acceptEvent->session;

	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _listenSocket))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));

	if (false == _service->GetIocpCore()->Register(session))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->ProcessConnect();

	// 다음 클라이언트 수신을 위해 재등록
	RegisterAccept(acceptEvent);
}
