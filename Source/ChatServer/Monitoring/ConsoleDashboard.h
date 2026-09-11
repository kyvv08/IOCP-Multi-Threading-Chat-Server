#pragma once
#include "ServerEngine.h"

/*------------------------
    ConsoleDashboard
-------------------------*/
class ConsoleDashboard
{
public:
	static void Init();
	static void Render(ServerServiceRef service);

private:
	static void EnableAnsiEscapeCodes();
};
