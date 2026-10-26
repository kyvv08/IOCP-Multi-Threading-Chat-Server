#pragma once
#include "Common/Types.h"
#include <windows.h>
#include <DbgHelp.h>
#include <string>

#pragma comment(lib, "DbgHelp.lib")

/*--------------------
    CrashDump
---------------------*/
class CrashDump
{
public:
	// 프로세스 시작 시 초기화 및 UnhandledExceptionFilter 등록
	static void Init(const std::wstring& dumpFolder = L"Dumps");

	// SEH 예외 필터 핸들러
	static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);

	// 수동 또는 자동 미니덤프 생성 함수
	static bool CreateDump(EXCEPTION_POINTERS* exceptionPointers = nullptr, const std::wstring& customTag = L"");

private:
	static std::wstring GetCurrentDateTimeString();

private:
	inline static std::wstring _dumpFolder = L"Dumps";
	inline static bool _initialized = false;
};
