#include "Network/Session.h"
#include "Network/SocketUtils.h"
#include "Network/Service.h"
#include "Common/Macros.h"

Session::Session() : _recvBuffer(RECV_BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
	_sessionId = ID.fetch_add(1);
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (sendBuffer == nullptr || !IsConnected())
		return;

	bool registerSend = false;

	{
		SpinLockGuard guard(_sendLock);
		_sendQueue.push(sendBuffer);

		if (!_isSending.exchange(true))
			registerSend = true;
	}

	if (registerSend)
		RegisterSend();
}

void Session::Send(const std::vector<SendBufferRef>& sendBuffers)
{
	if (sendBuffers.empty() || !IsConnected())
		return;

	bool registerSend = false;

	{
		SpinLockGuard guard(_sendLock);
		for (const auto& sb : sendBuffers)
			_sendQueue.push(sb);

		if (!_isSending.exchange(true))
			registerSend = true;
	}

	if (registerSend)
		RegisterSend();
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const std::wstring& /*cause*/)
{
	if (!_connected.exchange(false))
		return;

	RegisterDisconnect();
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (false == SocketUtils::BindAnyAddress(_socket, 0))
		return false;

	if (false == GetService()->GetIocpCore()->Register(shared_from_this()))
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this();

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = _netAddress.GetSockAddr();

	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), NULL, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_connectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this();

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_disconnectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (!IsConnected())
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this();

	WSABUF wsabufs[2];
	int32 spanCount = _recvBuffer.GetWriteSpan(wsabufs);

	if (spanCount == 0)
	{
		Disconnect(L"RecvBuffer Overflow");
		return;
	}

	DWORD numOfBytes = 0;
	DWORD flags = 0;

	if (SOCKET_ERROR == ::WSARecv(_socket, wsabufs, spanCount, &numOfBytes, &flags, &_recvEvent, NULL))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr;
		}
	}
}

void Session::RegisterSend()
{
	if (!IsConnected())
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();

	std::vector<WSABUF> wsaBufs;

	{
		SpinLockGuard guard(_sendLock);
		while (!_sendQueue.empty())
		{
			SendBufferRef sendBuffer = _sendQueue.front();
			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);

			WSABUF wsaBuf;
			wsaBuf.buf = reinterpret_cast<CHAR*>(sendBuffer->Buffer());
			wsaBuf.len = static_cast<ULONG>(sendBuffer->WriteSize());
			wsaBufs.push_back(wsaBuf);
		}
	}

	if (wsaBufs.empty())
	{
		_isSending.store(false);
		return;
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &numOfBytes, 0, &_sendEvent, NULL))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr;
			_sendEvent.sendBuffers.clear();
			_isSending.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr;
	_connected.store(true);

	SocketUtils::SetUpdateConnectSocket(_socket);

	GetService()->AddSession(GetSessionRef());

	OnConnected();
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;
	OnDisconnected();

	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr;

	if (numOfBytes == 0)
	{
		Disconnect(L"Peer closed connection");
		return;
	}

	if (!_recvBuffer.MoveWritePos(numOfBytes))
	{
		Disconnect(L"RecvBuffer MoveWritePos failed");
		return;
	}

	int32 dataSize = _recvBuffer.GetDataSize();
	std::vector<BYTE> packetData(dataSize);
	_recvBuffer.Peek(packetData.data(), dataSize);

	int32 processedLen = OnRecv(packetData.data(), dataSize);
	if (processedLen < 0 || processedLen > dataSize || !_recvBuffer.MoveReadPos(processedLen))
	{
		Disconnect(L"OnRecv Overflow / Invalid packet");
		return;
	}

	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr;
	_sendEvent.sendBuffers.clear();

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0 bytes");
		return;
	}

	OnSend(numOfBytes);

	bool registerSend = false;

	{
		SpinLockGuard guard(_sendLock);
		if (_sendQueue.empty())
		{
			_isSending.store(false);
		}
		else
		{
			registerSend = true;
		}
	}

	if (registerSend)
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"Connection reset/aborted");
		break;
	default:
		Disconnect(std::to_wstring(errorCode));
		break;
	}
}
