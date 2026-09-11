namespace PacketGenerator;

public static class PacketFormat
{
    // PacketProtocol.h (전체 패킷 정의)
    // {0}: 패킷 ID Enum 목록
    // {1}: 패킷 구조체 정의 및 직렬화/역직렬화 구현
    public static string ProtocolHeaderFormat =
@"#pragma once
#include ""ServerEngine.h""

namespace Protocol
{{
	enum PacketId : uint16
	{{
{0}
	}};

{1}
}}
";

    // PacketHandler.h (서버 / 클라이언트 분기 처리기)
    // {0}: include 파일 이름
    // {1}: 사용자 정의 패킷 핸들러 전방 선언
    // {2}: HandlePacket 분기 스위치 문
    // {3}: 핸들러 클래스 이름
    public static string HandlerHeaderFormat =
@"#pragma once
#include ""PacketProtocol.h""

// 사용자 정의 패킷 핸들러 전방 선언
{1}

class {3}
{{
public:
	static void Init()
	{{
	}}

	static bool HandlePacket(PacketSessionRef session, BYTE* buffer, int32 len)
	{{
		BufferReader reader(buffer, len);
		PacketHeader header;
		if (reader.Read(header) == false)
			return false;

		switch (header.id)
		{{
{2}
		default:
			return false;
		}}

		return true;
	}}
}};
";

    // {0}: 패킷 이름, {1}: 패킷 ID
    public static string PacketIdMember = @"		PKT_{0} = {1},";

    // {0}: 패킷 이름
    // {1}: 멤버 변수 선언
    // {2}: Read 역직렬화 코드
    // {3}: Write 직렬화 코드
    public static string PacketStruct =
@"	struct {0}
	{{
		PacketHeader header;
{1}

		bool Read(BufferReader& reader)
		{{
{2}
			return reader.IsValid();
		}}

		SendBufferRef MakeSendBuffer()
		{{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_{0};
			header.size = 0; // 나중에 갱신
			writer.Write(header);

{3}

			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}}
	}};
";

    // {0}: 패킷 이름
    public static string HandlerDeclaration =
@"bool Handle_{0}(PacketSessionRef session, Protocol::{0}& pkt);";

    // {0}: 패킷 이름
    public static string DispatchCase =
@"		case Protocol::PKT_{0}:
			{{
				Protocol::{0} pkt;
				if (pkt.Read(reader) == false)
					return false;
				return Handle_{0}(session, pkt);
			}}";
}
