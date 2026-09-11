#pragma once
#include "Network/Session.h"

#pragma pack(push, 1)
struct PacketHeader
{
	uint16 size;
	uint16 id;
};
#pragma pack(pop)

/*--------------------
    PacketSession
---------------------*/
class PacketSession : public Session
{
public:
	PacketSession() = default;
	virtual ~PacketSession() = default;

	PacketSessionRef GetPacketSessionRef() { return std::static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	int32 OnRecv(BYTE* buffer, int32 len) sealed override;

	// 완성된 1개 패킷 수신 시 호출되는 순수 가상 함수
	virtual void OnRecvPacket(BYTE* buffer, int32 len) = 0;
};

using PacketSessionRef = std::shared_ptr<PacketSession>;
