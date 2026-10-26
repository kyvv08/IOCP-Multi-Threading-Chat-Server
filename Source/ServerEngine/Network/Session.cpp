#include "Network/Session.h"
#include "Network/SocketUtils.h"
#include "Network/Service.h"
#include "Common/Macros.h"

Session::Session(int32 recvBufferSize) : _recvBuffer(recvBufferSize)
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
	bool queueOverflow = false;

	{
		SpinLockGuard guard(_sendLock);
		if (_sendQueue.size() >= MAX_SEND_QUEUE_SIZE)
		{
			queueOverflow = true;
		}
		else
		{
			_sendQueue.push_back(sendBuffer);
			if (!_isSending.exchange(true))
				registerSend = true;
		}
	}

	if (queueOverflow)
	{
		Disconnect(L"SendQueue Overflow (High-Water Mark Exceeded)");
		return;
	}

	if (registerSend)
		RegisterSend();
}

void Session::Send(const std::vector<SendBufferRef>& sendBuffers)
{
	if (sendBuffers.empty() || !IsConnected())
		return;

	bool registerSend = false;
	bool queueOverflow = false;

	{
		SpinLockGuard guard(_sendLock);
		if (_sendQueue.size() + sendBuffers.size() >= MAX_SEND_QUEUE_SIZE)
		{
			queueOverflow = true;
		}
		else
		{
			for (const auto& sb : sendBuffers)
				_sendQueue.push_back(sb);

			if (!_isSending.exchange(true))
				registerSend = true;
		}
	}

	if (queueOverflow)
	{
		Disconnect(L"SendQueue Overflow (High-Water Mark Exceeded)");
		return;
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

	constexpr int32 MAX_GATHER = 64;
	WSABUF wsaBufs[MAX_GATHER];
	int32 bufCount = 0;

	{
		SpinLockGuard guard(_sendLock);
		while (!_sendQueue.empty() && bufCount < MAX_GATHER)
		{
			SendBufferRef sendBuffer = _sendQueue.front();
			_sendQueue.pop_front();
			_sendEvent.sendBuffers.push_back(sendBuffer);

			WSABUF wsaBuf;
			if (bufCount == 0 && _sendOffset > 0)
			{
				wsaBuf.buf = reinterpret_cast<CHAR*>(sendBuffer->Buffer() + _sendOffset);
				wsaBuf.len = static_cast<ULONG>(sendBuffer->WriteSize() - _sendOffset);
			}
			else
			{
				wsaBuf.buf = reinterpret_cast<CHAR*>(sendBuffer->Buffer());
				wsaBuf.len = static_cast<ULONG>(sendBuffer->WriteSize());
			}

			wsaBufs[bufCount++] = wsaBuf;
		}
	}

	if (bufCount == 0)
	{
		_isSending.store(false);
		_sendEvent.owner = nullptr;
		return;
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs, static_cast<DWORD>(bufCount), &numOfBytes, 0, &_sendEvent, NULL))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			if (!IsRecoverableErrorCode(errorCode))
			{
				HandleError(errorCode);
				_sendEvent.owner = nullptr;
				_sendEvent.sendBuffers.clear();
				_isSending.store(false);
			}
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

	if (numOfBytes == 0)
	{
		_sendEvent.sendBuffers.clear();
		Disconnect(L"Send 0 bytes (Connection Broken)");
		return;
	}

	OnSend(numOfBytes);

	int32 bytesRemaining = numOfBytes;
	std::vector<SendBufferRef> uncompletedBuffers;
	int32 nextOffset = 0;

	// [정책 1]: 완료된 바이트만큼 버퍼 리스트를 소모
	for (size_t i = 0; i < _sendEvent.sendBuffers.size(); ++i)
	{
		SendBufferRef sb = _sendEvent.sendBuffers[i];
		int32 actualPendingBytes = sb->WriteSize() - (i == 0 ? _sendOffset : 0);

		if (bytesRemaining >= actualPendingBytes)
		{
			// 이 버퍼는 완전 전송됨
			bytesRemaining -= actualPendingBytes;
		}
		else
		{
			// [부분 송신 발생]: 이 버퍼에서 bytesRemaining만큼만 전송되고 남음
			nextOffset = (i == 0 ? _sendOffset : 0) + bytesRemaining;
			for (size_t j = i; j < _sendEvent.sendBuffers.size(); ++j)
				uncompletedBuffers.push_back(_sendEvent.sendBuffers[j]);
			bytesRemaining = 0;
			break;
		}
	}

	_sendEvent.sendBuffers.clear();

	bool registerSend = false;
	{
		SpinLockGuard guard(_sendLock);

		if (!uncompletedBuffers.empty())
		{
			// 미완료된 버퍼들을 _sendQueue 맨 앞(front)에 역순으로 재삽입하여 순서 보장!
			for (auto it = uncompletedBuffers.rbegin(); it != uncompletedBuffers.rend(); ++it)
			{
				_sendQueue.push_front(*it);
			}
			_sendOffset = nextOffset;
			registerSend = true;
		}
		else
		{
			_sendOffset = 0;
			if (_sendQueue.empty())
			{
				_isSending.store(false);
			}
			else
			{
				registerSend = true;
			}
		}
	}

	if (registerSend)
		RegisterSend();
}

bool Session::IsRecoverableErrorCode(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAEWOULDBLOCK:
	case WSA_IO_INCOMPLETE:
		return true;
	default:
		return false;
	}
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"Connection reset/aborted");
		break;
	case WSAENETDOWN:
	case WSAENETUNREACH:
		Disconnect(L"Network subsystem failed");
		break;
	default:
		Disconnect(L"Socket Error: " + std::to_wstring(errorCode));
		break;
	}
}
