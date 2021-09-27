#include "Logger.h"
#include "Network.h"
#include "Core.h"
#include <cassert>

#define TEST_PORT 1081
int32_t TestClientCounter = 512;
int32_t TestSendCounter = 128;

int main()
{
    CoreInitialize();
    std::vector<std::unique_ptr<CClient>> Clients;
    std::vector<std::future<std::shared_ptr<CConnection>>> ClientFutureConnections;
    std::vector<std::shared_ptr<CConnection>> ClientConnections;
    for (size_t i = 0; i < TestClientCounter; i++)
    {
        Clients.emplace_back(std::make_unique<CClient>(1));
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
            ClientConnections[i]->Send(std::format("ClientIndex : {:d}, TestSendCounter : {:d}", i, TestSendCounter));
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
    CoreUninitialize();
}


