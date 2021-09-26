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
    std::vector<std::unique_ptr<CClient>> Clients;
    std::vector<std::future<std::shared_ptr<CConnection>>> ClientFutureConnections;
    std::vector<std::shared_ptr<CConnection>> ClientConnections;
    for (size_t i = 0; i < 32; i++)
    {
        Clients.emplace_back(std::make_unique<CClient>(1));
        ClientFutureConnections.push_back(Clients[i]->ConnectToServer("127.0.0.1", TEST_PORT));
    }
    std::thread WorkThread;
    std::once_flag OnceFlag;
    while (true)
    {
        //GLOGD("{:d}", ClientFutureConnections.size());
        if (ClientFutureConnections.empty()) {
            std::call_once(OnceFlag,
                [&] {
                    WorkThread = std::move(std::thread([&] {
                        uint32_t i = 0;
                        for (auto it = ClientConnections.begin(); it != ClientConnections.end(); it++)
                        {
                            (*it)->Send(FMessageData(std::format("send message: {:d}", i++)));
                            //(*it)->Disconnect();
                        }
                    }));
                    //Server.Stop();
                }
            );
        }
        for (auto it = ClientFutureConnections.begin(); it != ClientFutureConnections.end(); )
        {
            if(it->_Is_ready())
            {
                ClientConnections.push_back(it->get());
                it = ClientFutureConnections.erase(it);
            }
            else
            {
                it++;
            }
        }
        for (auto it = Clients.begin(); it != Clients.end(); it++)
        {
            (*it)->ProcessEvent();
        }
        Server.ProcessEvent();
    }

     CoreUninitialize();
}


