#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081
int32_t TestClientCounter = 32;
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
        uint8_t* BodyPtr = reinterpret_cast<uint8_t*>(ConnectionPtr->GetNetworkProtocol()->GetBodyPtr(Data.data()));
        uint32_t BodySize = ConnectionPtr->GetNetworkProtocol()->GetBodySize(Data.data());
        std::string CopyBinary((char*)BodyPtr, BodySize);
        std::vector<uint8_t> SendData;
        ConnectionPtr->Send(Data);
    }
};

int main()
{
    CoreInitialize();
    {
        std::thread T1([] {
            FTestEchoServer TestServer;
            TestServer.Run(TEST_PORT);
            uint32_t WaitEventCounter = TestSendCounter * TestClientCounter;
            while (WaitEventCounter > 0)
            {
                WaitEventCounter -= TestServer.ProcessEvent();
            }
            });
        std::thread T2([] {
            std::vector<std::unique_ptr<FClient>> Clients;
            std::vector<std::future<std::shared_ptr<SConnection>>> ClientFutureConnections;
            std::vector<std::shared_ptr<SConnection>> ClientConnections;
            for (int32_t i = 0; i < TestClientCounter; i++)
            {
                Clients.emplace_back(std::make_unique<FClient>(1));
                ClientFutureConnections.push_back(Clients[i]->ConnectToServer("127.0.0.1", TEST_PORT));
            }
            while (!ClientFutureConnections.empty())
            {
                for (auto it = ClientFutureConnections.begin(); it != ClientFutureConnections.end(); )
                {
                    if (it->_Is_ready())
                    {
                        ClientConnections.push_back(it->get());
                        it = ClientFutureConnections.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
            }

            uint32_t WaitEventCounter = TestSendCounter * TestClientCounter;
            while (--TestSendCounter >= 0)
            {
                for (size_t i = 0; i < ClientConnections.size(); i++)
                {
                    INetworkProtocol* NetworkProtocol = ClientConnections[i]->GetNetworkProtocol();
                    ClientConnections[i]->Send(NetworkProtocol->CreateDataFromString(std::format("ClientIndex : {:d}, TestSendCounter : {:d}", i, TestSendCounter)));
                }
            }
            while (WaitEventCounter > 0) {
                for (auto it = Clients.begin(); it != Clients.end(); it++)
                {
                    WaitEventCounter -= (*it)->ProcessEvent();
                }
            }
            ClientConnections.clear();
            Clients.clear();
            });
        T1.join();
        T2.join();
    }
    CoreUninitialize();
}




