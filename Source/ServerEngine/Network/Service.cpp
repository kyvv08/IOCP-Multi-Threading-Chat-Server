#include "Network/Service.h"
#include "Network/Session.h"
#include "Network/Listener.h"
#include "Network/SocketUtils.h"

/*-----------------
    Service
------------------*/
Service::Service(ServiceType type, NetAddress address, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount)
	: _serviceType(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

void Service::Stop()
{
	WriteLockGuard guard(_lock);
	for (const auto& session : _sessions)
	{
		session->Disconnect(L"Service Stop");
	}
	_sessions.clear();
}

SessionRef Service::CreateSession()
{
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());
	return session;
}

void Service::AddSession(SessionRef session)
{
	WriteLockGuard guard(_lock);
	_sessionCount.fetch_add(1);
	_sessions.insert(session);
}

void Service::ReleaseSession(SessionRef session)
{
	WriteLockGuard guard(_lock);
	_sessionCount.fetch_sub(1);
	_sessions.erase(session);
}

void Service::Broadcast(SendBufferRef sendBuffer)
{
	ReadLockGuard guard(_lock);
	for (const auto& session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

std::vector<SessionRef> Service::GetSessionsSnapshot()
{
	ReadLockGuard guard(_lock);
	return std::vector<SessionRef>(_sessions.begin(), _sessions.end());
}

/*--------------------
    ServerService
---------------------*/
ServerService::ServerService(NetAddress address, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (_sessionFactory == nullptr)
		return false;

	_listener = std::make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = std::static_pointer_cast<ServerService>(shared_from_this());
	if (false == _listener->StartAccept(service))
		return false;

	return true;
}

void ServerService::Stop()
{
	if (_listener != nullptr)
	{
		_listener->CloseSocket();
		_listener = nullptr;
	}

	Service::Stop();
}

/*--------------------
    ClientService
---------------------*/
ClientService::ClientService(NetAddress targetAddress, std::shared_ptr<IocpCore> core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
	if (_sessionFactory == nullptr)
		return false;

	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; ++i)
	{
		SessionRef session = CreateSession();
		session->SetNetAddress(_netAddress);
		if (false == session->Connect())
			return false;
	}

	return true;
}

void ClientService::Stop()
{
	Service::Stop();
}
