#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081
int32_t TestClientCounter = 512;
int32_t TestSendCounter = 128;

class FTestEchoServer : public FServer 
{
    using Super = FServer;
public:
    FTestEchoServer(uint32_t ThreadNumber = 0)
        : FServer(ThreadNumber)
    {

    }

    void OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data) override
    {
        Super::OnRecvData(ConnectionPtr, Data);
        //uint8_t* BodyPtr = reinterpret_cast<uint8_t*>(ConnectionPtr->GetNetworkProtocol()->GetBodyPtr(Data.data()));
        //uint32_t BodySize = ConnectionPtr->GetNetworkProtocol()->GetBodySize(Data.data());
        //std::string CopyBinary(BodyPtr, BodySize);
        ConnectionPtr->Send(Data);
    }
};

int main()
{
    CoreInitialize();
    {
        FTestEchoServer TestServer;
        TestServer.Run(TEST_PORT);
        uint32_t WaitEventCounter = TestSendCounter * TestClientCounter;
        while (WaitEventCounter > 0)
        {
            WaitEventCounter -= TestServer.ProcessEvent();
        }
    }
    CoreUninitialize();
}

