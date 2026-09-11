#pragma once
#include "ServerEngine.h"
#include <string>
#include <atomic>

/*--------------------
    ServerSession
---------------------*/
class ServerSession : public PacketSession
{
public:
	ServerSession();
	virtual ~ServerSession() = default;

	void SetPlayerName(const std::string& name) { _playerName = name; }
	const std::string& GetPlayerName() const { return _playerName; }

	void SetLoggedIn(bool loggedIn) { _isLoggedIn.store(loggedIn); }
	bool IsLoggedIn() const { return _isLoggedIn.load(); }

	void SetCurrentRoomId(int32 roomId) { _currentRoomId.store(roomId); }
	int32 GetCurrentRoomId() const { return _currentRoomId.load(); }

	std::shared_ptr<ServerSession> GetServerSessionRef()
	{
		return std::static_pointer_cast<ServerSession>(shared_from_this());
	}

protected:
	void OnConnected() override;
	void OnRecvPacket(BYTE* buffer, int32 len) override;
	void OnDisconnected() override;

private:
	std::string _playerName;
	std::atomic<bool> _isLoggedIn{ false };
	std::atomic<int32> _currentRoomId{ 0 };
};

using ServerSessionRef = std::shared_ptr<ServerSession>;
