#pragma once
#include "Common/Types.h"
#include "Network/IocpEvent.h"
#include "Network/NetAddress.h"
#include "Buffer/RingBuffer.h"
#include "Buffer/SendBuffer.h"
#include "Lock/SpinLock.h"
#include <queue>

static std::atomic<uint64> ID = 0;

class Service;

/*----------------
    Session
-----------------*/
class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum { RECV_BUFFER_SIZE = 0x10000 }; // 64KB

public:
	Session();
	virtual ~Session();

	// 외부 노출 인터페이스
	void Send(SendBufferRef sendBuffer);
	void Send(const std::vector<SendBufferRef>& sendBuffers);
	bool Connect();
	void Disconnect(const std::wstring& cause);

	bool IsConnected() const { return _connected.load(); }
	SessionRef GetSessionRef() { return std::static_pointer_cast<Session>(shared_from_this()); }

	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress GetNetAddress() const { return _netAddress; }
	SOCKET GetSocket() const { return _socket; }
	void SetService(std::shared_ptr<Service> service) { _service = service; }
	std::shared_ptr<Service> GetService() const { return _service.lock(); }

	uint64 GetSessionId() const { return _sessionId; }
	void SetSessionId(uint64 id) { _sessionId = id; }

	uint64 GetLastHeartbeatTick() const { return _lastHeartbeatTick.load(); }
	void UpdateHeartbeat(uint64 tick) { _lastHeartbeatTick.store(tick); }

public:
	// IocpObject 인터페이스 구현
	HANDLE GetHandle() override { return reinterpret_cast<HANDLE>(_socket); }
	void Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	// I/O 등록 및 완료 처리
	bool RegisterConnect();
	bool RegisterDisconnect();
	void RegisterRecv();
	void RegisterSend();

	void ProcessConnect();
	void ProcessDisconnect();
	void ProcessRecv(int32 numOfBytes);
	void ProcessSend(int32 numOfBytes);

	void HandleError(int32 errorCode);

protected:
	// 비즈니스 로직 이벤트 훅
	virtual void OnConnected() {}
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 /*len*/) {}
	virtual void OnDisconnected() {}

private:
	std::weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	NetAddress _netAddress;
	std::atomic<bool> _connected{ false };

	uint64 _sessionId = 0;
	std::atomic<uint64> _lastHeartbeatTick{ 0 };

	// 수신 링 버퍼
	RingBuffer _recvBuffer;

	// 송신 큐 및 스핀락
	SpinLock _sendLock;
	std::queue<SendBufferRef> _sendQueue;
	std::atomic<bool> _isSending{ false };

	// 비동기 IOCP 이벤트 객체
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;
};
