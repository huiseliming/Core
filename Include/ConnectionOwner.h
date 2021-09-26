#pragma once
#include <functional>
#include <future>

#include "Queue.h"
#include "Message.h"

class CConnection;

namespace asio {
	class io_context;
}

class CConnectionOwner
{
public:
	struct FImpl;

	CConnectionOwner(uint32_t ThreadNumber = 0);
	CConnectionOwner(const CConnectionOwner&) = delete;
	CConnectionOwner(CConnectionOwner&& Other) = delete;
	CConnectionOwner& operator=(const CConnectionOwner&) = delete;
	CConnectionOwner& operator=(CConnectionOwner&& Other) = delete;

	virtual ~CConnectionOwner();

	virtual asio::io_context& GetIoContext();
	virtual TQueue<FMessage, EQueueMode::MPSC>& GetRecvQueue();
	virtual void IncreaseConnectionCounter();
	virtual void DecreaseConnectionCounter();

	virtual void OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData) = 0;
	virtual void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr) = 0;
	virtual void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr) = 0;

	void PushTask(std::function<void()>&& Task);;

	void ProcessTask();

	void ProcessMessage();

	void ProcessEvent();

	bool RunInIoContext();

	bool RunInUIThread();

protected:
	std::unique_ptr<FImpl> Impl;

};


class CClient : public CConnectionOwner
{
public:
	CClient(uint32_t ThreadNumber)
		: CConnectionOwner(ThreadNumber)
	{
	}


	void OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData);;
	void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr) {};
	void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr) {};
	std::future<std::shared_ptr<CConnection>> ConnectToServer(std::string Address, uint16_t Port);
};

class CServer : public CConnectionOwner
{
public:
	//using Super = CConnectionOwner;
	//struct FImpl : public Super::FImpl
	//{
	//	using Super = Super::FImpl;
	//	FImpl()
	//		: Super()
	//	{

	//	}
	//};

	CServer(uint32_t ThreadNumber = 0)
		: CConnectionOwner(ThreadNumber)
	{
	}

	void Run(uint16_t Port);

	void WaitForClientConnection();

	void Stop();

	virtual void OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData) override;;
	virtual void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr) override;

	virtual void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr) override;

	uint32_t ConnectedConnection{ 0 };

	std::vector<std::shared_ptr<CConnection>> WaitCleanConnections;
	std::unordered_map<std::string, std::shared_ptr<CConnection>> Connections;

};
