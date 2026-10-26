#include "Common/CrashDump.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

void CrashDump::Init(const std::wstring& dumpFolder)
{
	if (_initialized)
		return;

	_dumpFolder = dumpFolder;
	::CreateDirectoryW(_dumpFolder.c_str(), NULL);

	::SetUnhandledExceptionFilter(ExceptionFilter);
	_initialized = true;

	std::wcout << L"[INFO] CrashDump Initialized. Dump Directory: " << _dumpFolder << std::endl;
}

LONG WINAPI CrashDump::ExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
	std::wcerr << L"\n\033[1;31m========================================================================================\033[0m\n";
	std::wcerr << L"\033[1;31m                     FATAL ERROR: UNHANDLED EXCEPTION / ASSERT_CRASH                    \033[0m\n";
	std::wcerr << L"\033[1;31m========================================================================================\033[0m\n";

	if (exceptionPointers && exceptionPointers->ExceptionRecord)
	{
		std::wcerr << L" * Exception Code    : 0x" << std::hex << exceptionPointers->ExceptionRecord->ExceptionCode << std::dec << L"\n";
		std::wcerr << L" * Exception Address : " << exceptionPointers->ExceptionRecord->ExceptionAddress << L"\n";
	}

	CreateDump(exceptionPointers, L"Crash");

	std::wcerr << L"\033[1;31m========================================================================================\033[0m\n\n";

	return EXCEPTION_EXECUTE_HANDLER;
}

bool CrashDump::CreateDump(EXCEPTION_POINTERS* exceptionPointers, const std::wstring& customTag)
{
	::CreateDirectoryW(_dumpFolder.c_str(), NULL);

	std::wstring tag = customTag.empty() ? L"Dump" : customTag;
	std::wstring fileName = _dumpFolder + L"/" + tag + L"_" + GetCurrentDateTimeString() + L".dmp";

	HANDLE hFile = ::CreateFileW(
		fileName.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[ERROR] Failed to create crash dump file: " << fileName << L" (Error: " << ::GetLastError() << L")\n";
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	if (exceptionPointers)
	{
		dumpInfo.ThreadId = ::GetCurrentThreadId();
		dumpInfo.ExceptionPointers = exceptionPointers;
		dumpInfo.ClientPointers = FALSE;
	}

	// 콜스택, 모듈, 스레드, 간접 참조 메모리까지 포함하는 미니덤프 플래그
	MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
		MiniDumpWithDataSegs |
		MiniDumpWithIndirectlyReferencedMemory |
		MiniDumpWithProcessThreadData |
		MiniDumpWithHandleData |
		MiniDumpWithUnloadedModules
	);

	BOOL success = ::MiniDumpWriteDump(
		::GetCurrentProcess(),
		::GetCurrentProcessId(),
		hFile,
		dumpType,
		exceptionPointers ? &dumpInfo : NULL,
		NULL,
		NULL
	);

	::CloseHandle(hFile);

	if (success)
	{
		std::wcerr << L" \033[1;32m[CRASH DUMP SAVED] Successfully generated crash minidump:\033[0m\n"
			<< L"  -> \033[1;37m" << fileName << L"\033[0m\n"
			<< L"     (Open with Visual Studio or WinDbg to inspect callstack & memory)\n";
		return true;
	}
	else
	{
		std::wcerr << L"[ERROR] MiniDumpWriteDump failed (Error: " << ::GetLastError() << L")\n";
		return false;
	}
}

std::wstring CrashDump::GetCurrentDateTimeString()
{
	auto now = std::chrono::system_clock::now();
	std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

	std::tm buf;
	localtime_s(&buf, &in_time_t);

	std::wstringstream ss;
	ss << std::put_time(&buf, L"%Y%m%d_%H%M%S");
	return ss.str();
}
