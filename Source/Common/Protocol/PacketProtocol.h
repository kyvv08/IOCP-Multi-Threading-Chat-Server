#pragma once
#include "ServerEngine.h"

namespace Protocol
{
	enum PacketId : uint16
	{
		PKT_C_LOGIN = 1001,
		PKT_S_LOGIN = 1002,
		PKT_C_HEARTBEAT = 1003,
		PKT_S_HEARTBEAT = 1004,
		PKT_C_ROOM_LIST = 1005,
		PKT_S_ROOM_LIST = 1006,
		PKT_C_ENTER_ROOM = 1007,
		PKT_S_ENTER_ROOM = 1008,
		PKT_C_LEAVE_ROOM = 1009,
		PKT_S_LEAVE_ROOM = 1010,
		PKT_C_ROOM_CHAT = 1011,
		PKT_S_ROOM_CHAT = 1012,
		PKT_C_GLOBAL_CHAT = 1013,
		PKT_S_GLOBAL_CHAT = 1014,
		PKT_S_NOTICE = 1015,

	};

	struct C_LOGIN
	{
		PacketHeader header;
		std::string name;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(name) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_LOGIN;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(name) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_LOGIN
	{
		PacketHeader header;
		bool success{};
		uint64 playerId{};
		std::string name;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.Read(success) == false) return false;
			if (reader.Read(playerId) == false) return false;
			if (reader.ReadString(name) == false) return false;
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_LOGIN;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(success) == false) return nullptr;
			if (writer.Write(playerId) == false) return nullptr;
			if (writer.WriteString(name) == false) return nullptr;
			if (writer.WriteString(message) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_HEARTBEAT
	{
		PacketHeader header;
		uint64 clientTick{};


		bool Read(BufferReader& reader)
		{
			if (reader.Read(clientTick) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_HEARTBEAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(clientTick) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_HEARTBEAT
	{
		PacketHeader header;
		uint64 serverTick{};


		bool Read(BufferReader& reader)
		{
			if (reader.Read(serverTick) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_HEARTBEAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(serverTick) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_ROOM_LIST
	{
		PacketHeader header;


		bool Read(BufferReader& reader)
		{

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_ROOM_LIST;
			header.size = 0; // 나중에 갱신
			writer.Write(header);



			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_ROOM_LIST
	{
		PacketHeader header;
		struct RoomsInfo
		{
			int32 roomId{};
			std::string roomName{};
			int32 userCount{};
		};
		std::vector<RoomsInfo> rooms;


		bool Read(BufferReader& reader)
		{
			uint16 roomsCount = 0;
			if (reader.Read(roomsCount) == false) return false;
			rooms.resize(roomsCount);
			for (uint16 i = 0; i < roomsCount; ++i)
			{
				if (reader.Read(rooms[i].roomId) == false) return false;
				if (reader.ReadString(rooms[i].roomName) == false) return false;
				if (reader.Read(rooms[i].userCount) == false) return false;
			}

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_ROOM_LIST;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			uint16 roomsCount = static_cast<uint16>(rooms.size());
			if (writer.Write(roomsCount) == false) return nullptr;
			for (const auto& item : rooms)
			{
				if (writer.Write(item.roomId) == false) return nullptr;
				if (writer.WriteString(item.roomName) == false) return nullptr;
				if (writer.Write(item.userCount) == false) return nullptr;
			}


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_ENTER_ROOM
	{
		PacketHeader header;
		int32 roomId{};
		std::string roomName;


		bool Read(BufferReader& reader)
		{
			if (reader.Read(roomId) == false) return false;
			if (reader.ReadString(roomName) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_ENTER_ROOM;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(roomId) == false) return nullptr;
			if (writer.WriteString(roomName) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_ENTER_ROOM
	{
		PacketHeader header;
		struct MembersInfo
		{
			std::string name{};
		};
		std::vector<MembersInfo> members;
		bool success{};
		int32 roomId{};
		std::string roomName;


		bool Read(BufferReader& reader)
		{
			if (reader.Read(success) == false) return false;
			if (reader.Read(roomId) == false) return false;
			if (reader.ReadString(roomName) == false) return false;
			uint16 membersCount = 0;
			if (reader.Read(membersCount) == false) return false;
			members.resize(membersCount);
			for (uint16 i = 0; i < membersCount; ++i)
			{
				if (reader.ReadString(members[i].name) == false) return false;
			}

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_ENTER_ROOM;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(success) == false) return nullptr;
			if (writer.Write(roomId) == false) return nullptr;
			if (writer.WriteString(roomName) == false) return nullptr;
			uint16 membersCount = static_cast<uint16>(members.size());
			if (writer.Write(membersCount) == false) return nullptr;
			for (const auto& item : members)
			{
				if (writer.WriteString(item.name) == false) return nullptr;
			}


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_LEAVE_ROOM
	{
		PacketHeader header;


		bool Read(BufferReader& reader)
		{

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_LEAVE_ROOM;
			header.size = 0; // 나중에 갱신
			writer.Write(header);



			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_LEAVE_ROOM
	{
		PacketHeader header;
		bool success{};


		bool Read(BufferReader& reader)
		{
			if (reader.Read(success) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_LEAVE_ROOM;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.Write(success) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_ROOM_CHAT
	{
		PacketHeader header;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_ROOM_CHAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(message) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_ROOM_CHAT
	{
		PacketHeader header;
		std::string senderName;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(senderName) == false) return false;
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_ROOM_CHAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(senderName) == false) return nullptr;
			if (writer.WriteString(message) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct C_GLOBAL_CHAT
	{
		PacketHeader header;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_C_GLOBAL_CHAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(message) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_GLOBAL_CHAT
	{
		PacketHeader header;
		std::string senderName;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(senderName) == false) return false;
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_GLOBAL_CHAT;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(senderName) == false) return nullptr;
			if (writer.WriteString(message) == false) return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};

	struct S_NOTICE
	{
		PacketHeader header;
		std::string message;


		bool Read(BufferReader& reader)
		{
			if (reader.ReadString(message) == false) return false;

			return reader.IsValid();
		}

		SendBufferRef MakeSendBuffer()
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(4096 * 4);
			BufferWriter writer(sendBuffer->Buffer(), sendBuffer->AllocSize());

			header.id = PKT_S_NOTICE;
			header.size = 0; // 나중에 갱신
			writer.Write(header);

			if (writer.WriteString(message) == false) 
				return nullptr;


			// 헤더의 최종 패킷 크기 업데이트
			header.size = static_cast<uint16>(writer.WriteSize());
			*reinterpret_cast<PacketHeader*>(sendBuffer->Buffer()) = header;

			SendBufferManager::Close(sendBuffer, header.size);
			return sendBuffer;
		}
	};


}
