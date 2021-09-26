#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081

int main()
{
    CoreInitialize();
    CServer Server;
    Server.Run(TEST_PORT);
    while (true)
    {
        Server.ProcessEvent();
    }

    CoreUninitialize();
}


