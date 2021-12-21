#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081
int32_t TestSendCounter = 10;

class FTestEchoServer : public FConnectionOwner
{
    using Super = FConnectionOwner;
public:
    FTestEchoServer(uint32_t ThreadNumber = 0)
        : FConnectionOwner(ThreadNumber)
    {

    }

    virtual void OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data) override
    {
        FConnectionOwner::OnRecvData(ConnectionPtr, Data);
        if (0 == strcmp("Hello" , (const char*)(Data.data() + sizeof(uint32_t))))
        {
            ConnectionPtr->Send({ 0x0C, 0x00, 0x00, 0x00, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '\0' });
        }
    }
    int32_t RecvCounter{ 0 };
};

int main(int argc, const char* argv[])
{
    bool RunServerThread = false;
    bool RunClientThread = false;
    if (argc == 1)
    {
        RunServerThread = true;
        RunClientThread = true;
    }
    for (size_t i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-S"))
            RunServerThread = true;
        if (strcmp(argv[i], "-C"))
            RunClientThread = true;
    }
    CoreInitialize();
    {
        bool StopEchoServer = false;
        std::thread T1([&] {
            FTestEchoServer TestServer(1);
            TestServer.Listen("", TEST_PORT);
            while (!StopEchoServer)
            {
                TestServer.ProcessTaskAndMessage();
                std::this_thread::yield();
            }
            });
        std::thread T2(
            [&] {
                {
                    FConnectionOwner Client(1);
                    std::shared_ptr<SConnection> ClientConnection;
                    std::atomic<int32_t> WaitConnectionConnectedCount = 0;
                    WaitConnectionConnectedCount++;
                    Client.ConnectToServer(
                        "127.0.0.1",
                        TEST_PORT,
                        [&](std::shared_ptr<SConnection> Connection) {
                            ClientConnection = Connection;
                            WaitConnectionConnectedCount--;
                        });
                    while (WaitConnectionConnectedCount != 0)
                    {
                        Client.ProcessTaskAndMessage();
                        std::this_thread::yield();
                    }
                    uint32_t WaitEventCounter = TestSendCounter;
                    while (--TestSendCounter >= 0)
                    {
                        ClientConnection->Send({ 0x06, 0x00, 0x00, 0x00, 'H','e','l' ,'l' ,'o', '\0' });
                    }
                    while (WaitEventCounter > 0) {
                        WaitEventCounter -= Client.ProcessTaskAndMessage();
                        std::this_thread::yield();
                    }
                    ClientConnection.reset();
                }
                StopEchoServer = true;
            });
        T1.join();
        T2.join();
    }
    CoreUninitialize();
    system("pause");
}




