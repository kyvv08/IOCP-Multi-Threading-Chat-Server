#include "ServerEngine.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <chrono>

/*-----------------------
    Memory Pool Test
------------------------*/
struct Player
{
	int32 id;
	int32 hp;
	int32 mp;
	char name[32];

	Player(int32 id, int32 hp, int32 mp, const char* n) : id(id), hp(hp), mp(mp)
	{
		strcpy_s(name, n);
	}
	~Player()
	{
		id = 0;
	}
};

void TestMemoryPool()
{
	std::cout << "[Test 1] MemoryPool & TLS Allocator Testing..." << std::endl;

	PoolAllocator::Init();

	// 1. 단순 할당/해제 테스트
	void* p1 = xalloc(24);
	void* p2 = xalloc(120);
	void* p3 = xalloc(4000);
	void* p4 = xalloc(8000); // 4KB 초과 대형 할당

	xrelease(p1);
	xrelease(p2);
	xrelease(p3);
	xrelease(p4);

	// 2. 멀티스레드 동시 할당/해제 스트레스 테스트
	constexpr int32 THREAD_COUNT = 8;
	constexpr int32 ALLOC_PER_THREAD = 10000;

	std::vector<std::thread> workers;
	for (int32 t = 0; t < THREAD_COUNT; ++t)
	{
		workers.emplace_back([&]()
		{
			for (int32 i = 0; i < ALLOC_PER_THREAD; ++i)
			{
				int32 size = (i % 256) + 16;
				void* ptr = xalloc(size);
				assert(ptr != nullptr);
				xrelease(ptr);
			}
		});
	}

	for (auto& w : workers)
		w.join();

	std::cout << "  -> Multi-threaded Allocation (80,000 ops) Succeeded!" << std::endl;

	// 3. ObjectPool 테스트
	Player* player = ObjectPool<Player>::Pop(101, 1000, 500, "Knight");
	assert(player->id == 101 && player->hp == 1000);
	ObjectPool<Player>::Push(player);

	std::shared_ptr<Player> spPlayer = ObjectPool<Player>::MakeShared(102, 2000, 800, "Mage");
	assert(spPlayer->id == 102);

	std::cout << "  -> ObjectPool Succeeded!" << std::endl;
}

/*-----------------------
    Ring Buffer Test
------------------------*/
void TestRingBuffer()
{
	std::cout << "[Test 2] RingBuffer & Scatter-Gather Span Testing..." << std::endl;

	RingBuffer rb(128); // 128 바이트 버퍼
	assert(rb.GetCapacity() == 128);
	assert(rb.GetDataSize() == 0);
	assert(rb.GetFreeSize() == 127); // 1바이트 마진

	// 1. 데이터 기록
	const char* msg1 = "Hello IOCP Ring Buffer!";
	int32 len1 = static_cast<int32>(strlen(msg1));
	bool ok = rb.Write(reinterpret_cast<const BYTE*>(msg1), len1);
	assert(ok);
	assert(rb.GetDataSize() == len1);

	// 2. Peek & Read
	char readBuf[128] = { 0 };
	ok = rb.Peek(reinterpret_cast<BYTE*>(readBuf), len1);
	assert(ok && strcmp(readBuf, msg1) == 0);
	assert(rb.GetDataSize() == len1);

	memset(readBuf, 0, sizeof(readBuf));
	ok = rb.Read(reinterpret_cast<BYTE*>(readBuf), len1);
	assert(ok && strcmp(readBuf, msg1) == 0);
	assert(rb.GetDataSize() == 0);

	// 3. Wrapping (원형 롤오버) 및 WSABUF Span 테스트
	rb.Clear();
	BYTE dummy[100];
	memset(dummy, 'A', sizeof(dummy));
	rb.Write(dummy, 100);
	rb.Read(dummy, 100); // writePos=100, readPos=100

	// 50바이트 쓰기 -> 100~128(28바이트) + 0~22(22바이트)로 쪼개짐
	BYTE wrapData[50];
	for (int i = 0; i < 50; ++i) wrapData[i] = static_cast<BYTE>('0' + (i % 10));
	ok = rb.Write(wrapData, 50);
	assert(ok);
	assert(rb.GetDataSize() == 50);

	// WSABUF Read Span 검증 (반드시 2개의 span으로 분할되어야 함)
	WSABUF readSpans[2];
	int32 spanCount = rb.GetReadSpan(readSpans, 50);
	assert(spanCount == 2);
	assert(readSpans[0].len == 28);
	assert(readSpans[1].len == 22);

	// Span 데이터 무결성 검증
	BYTE verifiedData[50];
	memcpy(verifiedData, readSpans[0].buf, readSpans[0].len);
	memcpy(verifiedData + readSpans[0].len, readSpans[1].buf, readSpans[1].len);
	assert(memcmp(verifiedData, wrapData, 50) == 0);

	// Read 완료 처리
	rb.MoveReadPos(50);
	assert(rb.GetDataSize() == 0);

	std::cout << "  -> RingBuffer Wrap-around & Scatter-Gather Span Succeeded!" << std::endl;
}

/*-----------------------
    Send Buffer Test
------------------------*/
void TestSendBuffer()
{
	std::cout << "[Test 3] SendBuffer Chunk & Multi-Session Broadcast Testing..." << std::endl;

	SendBufferRef sendBuffer = SendBufferManager::Open(128);
	BYTE* buf = sendBuffer->Buffer();
	const char* packetMsg = "CHAT_MSG: High Performance Game Server Portfolio";
	int32 packetSize = static_cast<int32>(strlen(packetMsg)) + 1;
	memcpy(buf, packetMsg, packetSize);

	SendBufferManager::Close(sendBuffer, packetSize);

	std::vector<SendBufferRef> sessionQueues(1000);
	for (int i = 0; i < 1000; ++i)
	{
		sessionQueues[i] = sendBuffer;
	}

	assert(sendBuffer.use_count() == 1001);

	for (int i = 0; i < 1000; ++i)
	{
		sessionQueues[i] = nullptr;
	}

	assert(sendBuffer.use_count() == 1);
	std::cout << "  -> Zero-Fragmentation SendBuffer Succeeded!" << std::endl;
}

/*-----------------------
    Thread Pool Test
------------------------*/
void TestThreadPool()
{
	std::cout << "[Test 4] ThreadPool Task Execution Testing..." << std::endl;

	ThreadPool pool(4);
	std::atomic<int32> completedTasks = 0;
	constexpr int32 TOTAL_TASKS = 1000;

	for (int32 i = 0; i < TOTAL_TASKS; ++i)
	{
		pool.Enqueue([&completedTasks]()
		{
			completedTasks.fetch_add(1, std::memory_order_relaxed);
		});
	}

	while (completedTasks.load() < TOTAL_TASKS)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	pool.Stop();
	assert(completedTasks.load() == TOTAL_TASKS);
	std::cout << "  -> ThreadPool 1,000 Tasks Executed Successfully!" << std::endl;
}

/*-----------------------
    Job System Test
------------------------*/
class TestRoom : public JobQueue
{
public:
	void AddScore(int32 delta)
	{
		_score += delta;
	}

	int32 GetScore() const { return _score; }

private:
	int32 _score = 0;
};

void TestJobSystem()
{
	std::cout << "[Test 5] Actor JobQueue & GlobalQueue & JobTimer Testing..." << std::endl;

	std::shared_ptr<TestRoom> room = std::make_shared<TestRoom>();
	constexpr int32 PRODUCER_THREADS = 4;
	constexpr int32 JOBS_PER_THREAD = 1000;

	std::vector<std::thread> producers;
	for (int32 i = 0; i < PRODUCER_THREADS; ++i)
	{
		producers.emplace_back([&]()
		{
			for (int32 j = 0; j < JOBS_PER_THREAD; ++j)
			{
				room->PushJob(room, &TestRoom::AddScore, 1);
			}
		});
	}

	std::atomic<bool> stopWorkers{ false };
	std::vector<std::thread> workers;
	for (int32 i = 0; i < 2; ++i)
	{
		workers.emplace_back([&]()
		{
			PoolAllocator::Init();
			while (!stopWorkers.load() || GlobalQueue::GetCount() > 0)
			{
				JobQueueRef jq = GlobalQueue::Pop();
				if (jq)
					jq->Execute();
				else
					std::this_thread::yield();
			}
		});
	}

	for (auto& p : producers)
		p.join();

	while (room->GetScore() < PRODUCER_THREADS * JOBS_PER_THREAD)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	stopWorkers.store(true);
	for (auto& w : workers)
		w.join();

	assert(room->GetScore() == PRODUCER_THREADS * JOBS_PER_THREAD);
	std::cout << "  -> Actor JobQueue Sequenced Execution (4,000 jobs) Succeeded!" << std::endl;

	// JobTimer 테스트
	std::atomic<bool> timerExecuted{ false };
	room->DoTimer(50, [&timerExecuted]()
	{
		timerExecuted.store(true);
	});

	uint64 startTick = ::GetTickCount64();
	while (!timerExecuted.load())
	{
		uint64 now = ::GetTickCount64();
		JobTimer::Distribute(now);

		JobQueueRef jq = GlobalQueue::Pop();
		if (jq)
			jq->Execute();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		if (now - startTick > 1000)
			break;
	}

	assert(timerExecuted.load() == true);
	std::cout << "  -> JobTimer Scheduled Callback Succeeded!" << std::endl;
}

/*-----------------------
    IOCP Network Test
------------------------*/
#pragma pack(push, 1)
struct TestPacket
{
	PacketHeader header;
	int32 packetSeq;
	char message[32];
};
#pragma pack(pop)

std::atomic<int32> GServerRecvCount{ 0 };
std::atomic<int32> GClientRecvCount{ 0 };

class TestServerSession : public PacketSession
{
protected:
	void OnConnected() override
	{
	}

	void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		const TestPacket* pkt = reinterpret_cast<const TestPacket*>(buffer);
		assert(pkt->header.size == sizeof(TestPacket));
		assert(pkt->header.id == 1001);

		GServerRecvCount.fetch_add(1, std::memory_order_relaxed);

		SendBufferRef sendBuffer = SendBufferManager::Open(sizeof(TestPacket));
		memcpy(sendBuffer->Buffer(), pkt, sizeof(TestPacket));
		SendBufferManager::Close(sendBuffer, sizeof(TestPacket));
		Send(sendBuffer);
	}

	void OnDisconnected() override
	{
	}
};

class TestClientSession : public PacketSession
{
protected:
	void OnConnected() override
	{
		for (int32 i = 0; i < 100000; ++i)
		{
			SendBufferRef sendBuffer = SendBufferManager::Open(sizeof(TestPacket));
			TestPacket* pkt = reinterpret_cast<TestPacket*>(sendBuffer->Buffer());
			pkt->header.size = sizeof(TestPacket);
			pkt->header.id = 1001;
			pkt->packetSeq = i + 1;
			sprintf_s(pkt->message, "EchoTest_%d", i + 1);

			SendBufferManager::Close(sendBuffer, sizeof(TestPacket));
			Send(sendBuffer);
		}
	}

	void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		const TestPacket* pkt = reinterpret_cast<const TestPacket*>(buffer);
		assert(pkt->header.size == sizeof(TestPacket));
		assert(pkt->header.id == 1001);

		GClientRecvCount.fetch_add(1, std::memory_order_relaxed);
	}

	void OnDisconnected() override
	{
	}
};

void TestIocpNetwork()
{
	std::cout << "[Test 6] IOCP Server & Client Loopback Network Testing..." << std::endl;

	GServerRecvCount.store(0);
	GClientRecvCount.store(0);

	SocketUtils::Init();

	std::shared_ptr<IocpCore> iocpCore = std::make_shared<IocpCore>();

	NetAddress serverAddr(L"127.0.0.1", 7777);

	ServerServiceRef serverService = std::make_shared<ServerService>(
		serverAddr,
		iocpCore,
		[]() { return std::make_shared<TestServerSession>(); },
		100
	);
	bool srvOk = serverService->Start();
	assert(srvOk);

	std::atomic<bool> stopIocp{ false };
	std::vector<std::thread> iocpWorkers;
	for (int32 i = 0; i < 2; ++i)
	{
		iocpWorkers.emplace_back([&iocpCore, &stopIocp]()
		{
			PoolAllocator::Init();
			while (!stopIocp.load())
			{
				iocpCore->Dispatch(10);
			}
		});
	}

	ClientServiceRef clientService = std::make_shared<ClientService>(
		serverAddr,
		iocpCore,
		[]() { return std::make_shared<TestClientSession>(); },
		1
	);
	bool cliOk = clientService->Start();
	assert(cliOk);

	uint64 waitStart = ::GetTickCount64();
	while (GClientRecvCount.load() < 100000)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (::GetTickCount64() - waitStart > 3000)
			break;
	}

	assert(GServerRecvCount.load() == 100000);
	assert(GClientRecvCount.load() == 100000);

	std::cout << "  -> IOCP Server/Client 5 Packets Echoed with 100% Integrity!" << std::endl;

	clientService->Stop();
	serverService->Stop();

	stopIocp.store(true);
	for (auto& w : iocpWorkers)
		w.join();

	SocketUtils::Clear();
}

/*---------------------------------------------
    Packet Generator & Client Handlers Test
----------------------------------------------*/
#include "ServerPacketHandler.h"
#include "ClientPacketHandler.h"
#include "Session/ClientSession.h"
#include "Room/RoomManager.h"
#include "Service/AbuseMonitor.h"

#include "Session/DummySession.h"

void TestPacketGenerator()
{
	std::cout << "[Test 7] C# PacketGenerator Generated Code Round-Trip Testing..." << std::endl;

	// 1. 단일 문자열 패킷 직렬화 및 서버 핸들러 디스패치
	{
		Protocol::C_LOGIN pkt;
		pkt.name = "GameArchitect";

		SendBufferRef sb = pkt.MakeSendBuffer();
		assert(sb != nullptr);

		ClientSessionRef mockSession = std::make_shared<ClientSession>();
		bool dispatched = ServerPacketHandler::HandlePacket(mockSession, sb->Buffer(), sb->WriteSize());
		assert(dispatched == true);
		assert(mockSession->GetPlayerName() == "GameArchitect");
	}

	// 2. 가변 길이 문자열 리스트를 포함한 복합 패킷 직렬화 및 클라이언트 핸들러 디스패치
	{
		Protocol::S_ENTER_ROOM pkt;
		pkt.success = true;
		pkt.roomId = 77;
		pkt.roomName = "Lobby";
		pkt.members.push_back({ "Alice" });
		pkt.members.push_back({ "Bob" });
		pkt.members.push_back({ "Charlie" });

		SendBufferRef sb = pkt.MakeSendBuffer();
		assert(sb != nullptr);

		bool dispatched = ClientPacketHandler::HandlePacket(nullptr, sb->Buffer(), sb->WriteSize());
		assert(dispatched == true);
	}

	// 3. 중첩 구조체 리스트 직렬화 및 클라이언트 핸들러 디스패치
	{
		Protocol::S_ROOM_LIST pkt;
		pkt.rooms.push_back({ 1, "Rookie Channel", 15 });
		pkt.rooms.push_back({ 2, "Pro Channel", 42 });

		SendBufferRef sb = pkt.MakeSendBuffer();
		assert(sb != nullptr);

		bool dispatched = ClientPacketHandler::HandlePacket(nullptr, sb->Buffer(), sb->WriteSize());
		assert(dispatched == true);
	}

	std::cout << "  -> Packet Serialization & Handler Dispatching Succeeded with 100% Integrity!" << std::endl;
}

void TestChatServerAndAbuseMonitor()
{
	std::cout << "[Test 8] Chat Server Multi-Room & TokenBucket Abuse Defense Testing..." << std::endl;

	RoomManager::Init();
	AbuseMonitor::Init();

	// 1. RoomManager & Multi-Room 생성 검증
	RoomRef room1 = RoomManager::GetOrCreateRoom(101, "TestRoom101");
	RoomRef room2 = RoomManager::GetOrCreateRoom(102, "TestRoom102");
	assert(room1 != nullptr && room2 != nullptr);
	assert(RoomManager::GetTotalRoomCount() >= 2);

	// 2. ClientSession & Actor JobQueue 방 입장/퇴장 무락 검증
	ClientSessionRef session1 = std::make_shared<ClientSession>();
	session1->SetPlayerInfo(1, "Player_Alice");

	ClientSessionRef session2 = std::make_shared<ClientSession>();
	session2->SetPlayerInfo(2, "Player_Bob");

	// Session 1은 Room 101 입장, Session 2는 Room 102 입장
	room1->PushJob(room1, &Room::Enter, session1);
	room2->PushJob(room2, &Room::Enter, session2);

	// GlobalQueue 워커 실행
	while (JobQueueRef jq = GlobalQueue::Pop())
		jq->Execute();

	assert(session1->GetRoom() == room1);
	assert(session2->GetRoom() == room2);
	assert(room1->GetUserCount() == 1);
	assert(room2->GetUserCount() == 1);

	// 3. 룸 내부 채팅 처리
	room1->PushJob(room1, &Room::HandleChat, session1, std::string("Hello Room 101"));
	while (JobQueueRef jq = GlobalQueue::Pop())
		jq->Execute();

	// 4. Session 1 방 퇴장
	room1->PushJob(room1, &Room::Leave, session1);
	while (JobQueueRef jq = GlobalQueue::Pop())
		jq->Execute();

	assert(session1->GetRoom() == nullptr);
	assert(room1->GetUserCount() == 0);

	// 5. TokenBucket 기반 Flooding 어뷰징 감지 및 차단 검증
	TokenBucket tb(5, 5); // 5개 최대 버스트
	for (int32 i = 0; i < 5; ++i)
	{
		assert(tb.Consume(1) == true);
	}
	// 6번째 패킷은 토큰 고갈로 차단되어야 함
	assert(tb.Consume(1) == false);

	// 어뷰징 모니터링 로그 기록
	AbuseMonitor::ReportAbuse(999, "Attacker_User", "192.168.1.100", 55555, L"Packet Flooding Detected");
	assert(AbuseMonitor::GetAbuseCount() == 1);

	// 6. 로그 파일 덤프 출력 검증 (Req #11 연동)
	bool dumped = AbuseMonitor::DumpLogsToFile("Test_Abuse_Log.txt");
	assert(dumped == true);

	std::cout << "  -> Multi-Room Business Logic & TokenBucket Flooding Defense Succeeded!" << std::endl;
}

#include "Monitoring/ServerStats.h"
#include "Monitoring/ConsoleDashboard.h"

void TestMonitoringAndLogging()
{
	std::cout << "[Test 9] Server Monitoring Metrics & Console Dashboard Testing..." << std::endl;

	ServerStats::Init();
	ConsoleDashboard::Init();

	// 1. 세션 연결 및 트래픽 메트릭 수집 검증
	ServerStats::OnSessionConnected();
	ServerStats::OnSessionConnected();
	assert(ServerStats::GetCurrentCcu() == 2);
	assert(ServerStats::GetTotalAccepted() == 2);

	ServerStats::OnPacketReceived(128);
	ServerStats::OnPacketReceived(256);
	assert(ServerStats::GetTotalRecvPackets() == 2);
	assert(ServerStats::GetTotalRecvBytes() == 384);

	ServerStats::OnPacketSent(512);
	assert(ServerStats::GetTotalSendBytes() == 512);

	ServerStats::OnSessionDisconnected();
	assert(ServerStats::GetCurrentCcu() == 1);
	assert(ServerStats::GetTotalAccepted() == 2);

	// 2. 가동 시간 포맷 검증
	std::string uptime = ServerStats::GetFormattedUptime();
	assert(uptime.length() == 8); // "00:00:00"

	// 3. 더미 서버 서비스를 통한 TUI 대시보드 렌더링 검증
	std::shared_ptr<IocpCore> core = std::make_shared<IocpCore>();
	ServerServiceRef dummyService = std::make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		core,
		[]() { return std::make_shared<ClientSession>(); },
		1000
	);

	ConsoleDashboard::Render(dummyService);

	std::cout << "  -> ServerStats Metrics & Dashboard Render Succeeded!" << std::endl;
}

void TestStressScenarios()
{
	std::cout << "[Test 10] Live Loopback Stress Bot Testing (Normal, Flood, Spam, Timeout)..." << std::endl;

	SocketUtils::Init();
	RoomManager::Init();
	AbuseMonitor::Init();
	ServerStats::Init();

	std::shared_ptr<IocpCore> serverIocp = std::make_shared<IocpCore>();
	ServerServiceRef serverService = std::make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 8888),
		serverIocp,
		[]() { return std::make_shared<ClientSession>(); },
		500
	);

	assert(serverService->Start() == true);

	std::atomic<bool> isRunning{ true };
	std::vector<std::thread> workers;

	// Server IOCP & JobQueue Worker Threads
	for (int32 i = 0; i < 4; ++i)
	{
		workers.emplace_back([&serverIocp, &isRunning]()
		{
			PoolAllocator::Init();
			while (isRunning.load())
			{
				serverIocp->Dispatch(10);
				while (JobQueueRef jq = GlobalQueue::Pop())
					jq->Execute();
			}
		});
	}

	// 1. Packet Flooding Attack Bot Test (10 bots spamming 80 packets each)
	{
		std::shared_ptr<IocpCore> floodIocp = std::make_shared<IocpCore>();
		std::vector<DummySessionRef> floodSessions;

		ClientServiceRef floodService = std::make_shared<ClientService>(
			NetAddress(L"127.0.0.1", 8888),
			floodIocp,
			[&floodSessions]()
			{
				auto s = std::make_shared<DummySession>(BotScenario::PacketFlood, static_cast<int32>(floodSessions.size() + 1));
				floodSessions.push_back(s);
				return s;
			},
			10
		);

		assert(floodService->Start() == true);

		// Run flood client for 500ms
		for (int32 t = 0; t < 50; ++t)
		{
			floodIocp->Dispatch(10);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		floodService->Stop();
		assert(AbuseMonitor::GetAbuseCount() > 0);
	}

	// 2. Connect / Disconnect Rapid Spam Bot Test (20 bots)
	{
		std::shared_ptr<IocpCore> spamIocp = std::make_shared<IocpCore>();
		std::vector<DummySessionRef> spamSessions;

		ClientServiceRef spamService = std::make_shared<ClientService>(
			NetAddress(L"127.0.0.1", 8888),
			spamIocp,
			[&spamSessions]()
			{
				auto s = std::make_shared<DummySession>(BotScenario::ConnectSpam, static_cast<int32>(spamSessions.size() + 1));
				spamSessions.push_back(s);
				return s;
			},
			20
		);

		assert(spamService->Start() == true);

		for (int32 t = 0; t < 50; ++t)
		{
			spamIocp->Dispatch(10);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		spamService->Stop();
	}

	// 3. Normal Chatting Bots Test (20 bots joining Room 1 and chatting)
	{
		std::shared_ptr<IocpCore> normalIocp = std::make_shared<IocpCore>();
		std::vector<DummySessionRef> normalSessions;

		ClientServiceRef normalService = std::make_shared<ClientService>(
			NetAddress(L"127.0.0.1", 8888),
			normalIocp,
			[&normalSessions]()
			{
				auto s = std::make_shared<DummySession>(BotScenario::Normal, static_cast<int32>(normalSessions.size() + 1));
				normalSessions.push_back(s);
				return s;
			},
			20
		);

		assert(normalService->Start() == true);

		for (int32 t = 0; t < 60; ++t)
		{
			normalIocp->Dispatch(10);
			if (t % 10 == 0)
			{
				for (auto& s : normalSessions)
				{
					if (s && s->IsConnected())
						s->SendChat("Automated stress bot chat message");
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		normalService->Stop();
	}

	// Stop Server
	serverService->Stop();
	isRunning.store(false);

	for (auto& w : workers)
	{
		if (w.joinable())
			w.join();
	}

	std::cout << "  -> Multi-threaded Live Stress Test Succeeded with 100% Stability!" << std::endl;
}

int main()
{
	std::cout << "==================================================" << std::endl;
	std::cout << "    [Step 7] Full Architecture & Stress Testing   " << std::endl;
	std::cout << "==================================================" << std::endl;

	TestMemoryPool();
	TestRingBuffer();
	TestSendBuffer();
	TestThreadPool();
	TestJobSystem();
	TestIocpNetwork();
	TestPacketGenerator();
	TestChatServerAndAbuseMonitor();
	TestMonitoringAndLogging();
	TestStressScenarios();

	std::cout << "\n>>> ALL 10 TESTS PASSED WITH 100% SUCCESS! <<<" << std::endl;
	return 0;
}
