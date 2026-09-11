#include "Network/PacketSession.h"

int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processedLen = 0;

	while (true)
	{
		int32 dataSize = len - processedLen;
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *reinterpret_cast<PacketHeader*>(&buffer[processedLen]);
		if (header.size < sizeof(PacketHeader))
			return -1; // 비정상 패킷 크기 -> 즉시 연결 해제

		if (dataSize < header.size)
			break; // 아직 온전한 패킷 1개가 다 오지 않음

		OnRecvPacket(&buffer[processedLen], header.size);

		processedLen += header.size;
	}

	return processedLen;
}
