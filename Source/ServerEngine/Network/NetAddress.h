#pragma once
#include "Common/Types.h"

/*-------------------
    NetAddress
--------------------*/
class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(const std::wstring& ip, uint16 port);

	SOCKADDR_IN& GetSockAddr() { return _sockAddr; }
	const SOCKADDR_IN& GetSockAddr() const { return _sockAddr; }

	std::wstring GetIpAddress() const;
	uint16 GetPort() const { return ::ntohs(_sockAddr.sin_port); }

	static IN_ADDR Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN _sockAddr = {};
};
