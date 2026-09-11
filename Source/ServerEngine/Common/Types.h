#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <iostream>
#include <thread>

// Windows & Winsock
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

// 기본 정수 타입 단축 정의
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using BYTE = uint8_t;

// 스마트 포인터 별칭
using SessionRef = std::shared_ptr<class Session>;
using PacketSessionRef = std::shared_ptr<class PacketSession>;
using ListenerRef = std::shared_ptr<class Listener>;
using ServiceRef = std::shared_ptr<class Service>;
using ServerServiceRef = std::shared_ptr<class ServerService>;
using ClientServiceRef = std::shared_ptr<class ClientService>;
using SendBufferRef = std::shared_ptr<class SendBuffer>;
using SendBufferChunkRef = std::shared_ptr<class SendBufferChunk>;
using JobRef = std::shared_ptr<class Job>;
using JobQueueRef = std::shared_ptr<class JobQueue>;

// TLS 스레드 ID
extern thread_local uint32 LThreadId;
