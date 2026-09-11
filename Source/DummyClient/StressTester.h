#pragma once
#include "ServerEngine.h"
#include "Session/DummySession.h"
#include <string>
#include <vector>

/*-------------------
    StressTester
--------------------*/
class StressTester
{
public:
	static void RunScenario(BotScenario scenario, int32 botCount, int32 durationSec, const std::wstring& ip = L"127.0.0.1", uint16 port = 7777);

private:
	static std::string GetScenarioName(BotScenario scenario);
	static void RenderProgress(BotScenario scenario, int32 botCount, int32 elapsedSec, int32 totalSec);
};
