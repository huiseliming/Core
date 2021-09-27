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
	friend class CConnection;
public:
	struct FImpl;

	CConnectionOwner(uint32_t ThreadNumber = 0);
	CConnectionOwner(const CConnectionOwner&) = delete;
	CConnectionOwner(CConnectionOwner&& Other) = delete;
	CConnectionOwner& operator=(const CConnectionOwner&) = delete;
	CConnectionOwner& operator=(CConnectionOwner&& Other) = delete;
	virtual ~CConnectionOwner();

	asio::io_context& GetIoContext();
	TQueue<FMessage, EQueueMode::MPSC>& GetRecvQueue();
	void IncreaseConnectionCounter();
	void DecreaseConnectionCounter();
	void PushTask(std::function<void()>&& Task);;
	void ProcessTask();
	uint32_t ProcessMessage();
	uint32_t ProcessEvent();

	std::unordered_map<std::string, std::shared_ptr<CConnection>>& GetConnectionMap();

protected:
	virtual void OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData);
	virtual void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr);
	virtual void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr);

protected:
	bool RunInOwnerThread();
	bool RunInIoContext();

	std::unique_ptr<FImpl> Impl;

};


class CClient : public CConnectionOwner
{
	using Super = CConnectionOwner;
public:
	CClient(uint32_t ThreadNumber);

	std::future<std::shared_ptr<CConnection>> ConnectToServer(std::string Address, uint16_t Port);
	//void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr)
	//{
	//	Super::OnConnectionConnected(ConnectionPtr);
	//	//do something
	//};
	//void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr)
	//{
	//	//do something
	//	Super::OnConnectionDisconnected(ConnectionPtr);
	//};
protected:
};

class CServer : public CConnectionOwner
{
	using Super = CConnectionOwner;
public:
	CServer(uint32_t ThreadNumber = 0);
	virtual ~CServer();

	void Run(uint16_t Port);
	void WaitForClientConnection();
	void Stop();
protected:
	//void OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr) 
	//{
	//	Super::OnConnectionConnected(ConnectionPtr);
	//	//do something
	//};
	//void OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr) 
	//{
	//	//do something
	//	Super::OnConnectionDisconnected(ConnectionPtr);
	//};


	bool CheckRunCallOnce();
#ifndef NDEBUG
	bool RunCallOnceFlag{ false };
#endif // !NDEBUG
};
