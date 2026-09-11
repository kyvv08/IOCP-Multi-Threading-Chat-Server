#pragma once
#include "ServerEngine.h"
#include <string>
#include <memory>

class Room;

/*--------------------
    ClientSession
---------------------*/
class ClientSession : public PacketSession
{
public:
	ClientSession();
	virtual ~ClientSession() = default;

	void SetPlayerInfo(uint64 id, const std::string& name)
	{
		_playerId = id;
		_playerName = name;
		_authenticated = true;
	}

	uint64 GetPlayerId() const { return _playerId; }
	const std::string& GetPlayerName() const { return _playerName; }
	bool IsAuthenticated() const { return _authenticated; }

	void SetRoom(std::shared_ptr<Room> room) { _room = room; }
	std::shared_ptr<Room> GetRoom() const { return _room.lock(); }

public:
	std::shared_ptr<ClientSession> GetClientSessionRef()
	{
		return std::static_pointer_cast<ClientSession>(shared_from_this());
	}

protected:
	void OnConnected() override;
	void OnRecvPacket(BYTE* buffer, int32 len) override;
	void OnDisconnected() override;

private:
	uint64 _playerId = 0;
	std::string _playerName;
	bool _authenticated = false;
	std::weak_ptr<Room> _room;

	// 세션별 패킷 플러딩 감지용 토큰 버킷 (최대 40 버스트, 초당 20 충전)
	TokenBucket _rateLimiter{ 40, 20 };
};

using ClientSessionRef = std::shared_ptr<ClientSession>;
