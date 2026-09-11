#pragma once
#include "Common/Types.h"
#include "Network/NetAddress.h"
#include "Network/IocpCore.h"
#include "Network/Listener.h"
#include "Lock/SpinLock.h"
#include <unordered_set>
#include <functional>

enum class ServiceType : uint8
{
	Server,
	Client
};

using SessionFactory = std::function<SessionRef()>;

/*-----------------
    Service
------------------*/
class Service : public std::enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1000);
	virtual ~Service();

	virtual bool Start() = 0;
	virtual void Stop();

	void SetSessionFactory(SessionFactory func) { _sessionFactory = func; }

	SessionRef CreateSession();
	void AddSession(SessionRef session);
	void ReleaseSession(SessionRef session);
	int32 GetCurrentSessionCount() const { return _sessionCount.load(); }
	int32 GetMaxSessionCount() const { return _maxSessionCount; }

	std::shared_ptr<IocpCore> GetIocpCore() const { return _iocpCore; }
	NetAddress GetNetAddress() const { return _netAddress; }
	ServiceType GetServiceType() const { return _serviceType; }

	void Broadcast(SendBufferRef sendBuffer);
	std::vector<SessionRef> GetSessionsSnapshot();

protected:
	ReadWriteLock _lock;
	ServiceType _serviceType;
	NetAddress _netAddress;
	std::shared_ptr<IocpCore> _iocpCore;

	std::unordered_set<SessionRef> _sessions;
	std::atomic<int32> _sessionCount = 0;
	int32 _maxSessionCount = 0;
	SessionFactory _sessionFactory;
};

using ServiceRef = std::shared_ptr<Service>;

/*--------------------
    ServerService
---------------------*/
class ServerService : public Service
{
public:
	ServerService(NetAddress address, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1000);
	virtual ~ServerService() = default;

	bool Start() override;
	void Stop() override;

private:
	std::shared_ptr<Listener> _listener = nullptr;
};

using ServerServiceRef = std::shared_ptr<ServerService>;

/*--------------------
    ClientService
---------------------*/
class ClientService : public Service
{
public:
	ClientService(NetAddress targetAddress, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ClientService() = default;

	bool Start() override;
	void Stop() override;
};

using ClientServiceRef = std::shared_ptr<ClientService>;
