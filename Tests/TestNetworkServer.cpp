#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081
int32_t TestClientCounter = 512;
int32_t TestSendCounter = 128;

class CTestEchoServer : public CServer 
{
    using Super = CServer;
public:
    CTestEchoServer(uint32_t ThreadNumber = 0)
        : CServer(ThreadNumber)
    {

    }

    void OnMessage(std::shared_ptr<SConnection> ConnectionPtr, FMessageData& MessageData) override
    {
        Super::OnMessage(ConnectionPtr, MessageData);
        std::string CopyBinary(MessageData.GetBody<char>(), MessageData.GetBodySize());
        ConnectionPtr->Send(CopyBinary);
    }
};

int main()
{
    CoreInitialize();
    {
        CTestEchoServer TestServer;
        TestServer.Run(TEST_PORT);
        uint32_t WaitEventCounter = TestSendCounter * TestClientCounter;
        while (WaitEventCounter > 0)
        {
            WaitEventCounter -= TestServer.ProcessEvent();
        }
    }
    CoreUninitialize();
}

