#include "Network/SocketUtils.h"
#include "Common/Macros.h"

LPFN_CONNECTEX SocketUtils::ConnectEx = nullptr;
LPFN_DISCONNECTEX SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX SocketUtils::AcceptEx = nullptr;

void SocketUtils::Init()
{
	WSADATA wsaData;
	ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);

	// 확장 함수 포인터 로드를 위한 임시 더미 소켓 생성
	SOCKET dummySocket = CreateSocket();
	ASSERT_CRASH(dummySocket != INVALID_SOCKET);

	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));

	Close(dummySocket);
}

void SocketUtils::Clear()
{
	::WSACleanup();
}

bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn), &bytes, NULL, NULL) != SOCKET_ERROR;
}

SOCKET SocketUtils::CreateSocket()
{
	return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;
	return ::setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&option), sizeof(option)) != SOCKET_ERROR;
}

bool SocketUtils::SetReuseAddress(SOCKET socket, bool flag)
{
	int32 opt = flag ? 1 : 0;
	return ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt)) != SOCKET_ERROR;
}

bool SocketUtils::SetRecvBufferSize(SOCKET socket, int32 size)
{
	return ::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&size), sizeof(size)) != SOCKET_ERROR;
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int32 size)
{
	return ::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&size), sizeof(size)) != SOCKET_ERROR;
}

bool SocketUtils::SetTcpNoDelay(SOCKET socket, bool flag)
{
	int32 opt = flag ? 1 : 0;
	return ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&opt), sizeof(opt)) != SOCKET_ERROR;
}

bool SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{
	return ::setsockopt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&listenSocket), sizeof(listenSocket)) != SOCKET_ERROR;
}

bool SocketUtils::SetUpdateConnectSocket(SOCKET socket)
{
	return ::setsockopt(socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) != SOCKET_ERROR;
}

bool SocketUtils::Bind(SOCKET socket, NetAddress netAddr)
{
	return ::bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.GetSockAddr()), sizeof(SOCKADDR_IN)) != SOCKET_ERROR;
}

bool SocketUtils::BindAnyAddress(SOCKET socket, uint16 port)
{
	SOCKADDR_IN myAddress;
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = ::htonl(INADDR_ANY);
	myAddress.sin_port = ::htons(port);

	return ::bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddress), sizeof(myAddress)) != SOCKET_ERROR;
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog)
{
	return ::listen(socket, backlog) != SOCKET_ERROR;
}

void SocketUtils::Close(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
	{
		::closesocket(socket);
		socket = INVALID_SOCKET;
	}
}
