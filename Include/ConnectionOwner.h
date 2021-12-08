#pragma once
#include <functional>
#include <future>
#include "CoreApi.h"
#include "Queue.h"
#include "Connection.h"

class SConnection;

namespace asio {
	class io_context;
}

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

class CORE_API FConnectionOwner
{
	friend class SConnection;
public:
	struct FImpl;

	FConnectionOwner(uint32_t ThreadNumber = 0);
	FConnectionOwner(const FConnectionOwner&) = delete;
	FConnectionOwner(FConnectionOwner&& Other) = delete;
	FConnectionOwner& operator=(const FConnectionOwner&) = delete;
	FConnectionOwner& operator=(FConnectionOwner&& Other) = delete;
	virtual ~FConnectionOwner();

	asio::io_context& GetIoContext();
	TQueue<FDataOwner, EQueueMode::MPSC>& GetRecvQueue();
	void IncreaseConnectionCounter();
	void DecreaseConnectionCounter();
	void PushTask(std::function<void()> Task);;
	void ProcessTask();
	uint32_t ProcessMessage();
	uint32_t ProcessEvent();

	std::unordered_map<std::string, std::shared_ptr<SConnection>>& GetConnectionMap();

protected:
	virtual void OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data);
	virtual void OnConnected(std::shared_ptr<SConnection> ConnectionPtr);
	virtual void OnDisconnected(std::shared_ptr<SConnection> ConnectionPtr);

protected:
	bool RunInOwnerThread();
	bool RunInIoContext();

	std::unique_ptr<FImpl> Impl;

};

class CORE_API FClient : public FConnectionOwner
{
	using Super = FConnectionOwner;
public:
	FClient(uint32_t ThreadNumber = 1);

	std::future<std::shared_ptr<SConnection>> ConnectToServer(std::string Address, uint16_t Port, std::function<void(std::shared_ptr<SConnection>)> ConnectCallback = {});
	//void OnConnectionConnected(std::shared_ptr<SConnection> ConnectionPtr)
	//{
	//	Super::OnConnectionConnected(ConnectionPtr);
	//	//do something
	//};
	//void OnConnectionDisconnected(std::shared_ptr<SConnection> ConnectionPtr)
	//{
	//	//do something
	//	Super::OnConnectionDisconnected(ConnectionPtr);
	//};
protected:
};

class CORE_API FServer : public FConnectionOwner
{
	using Super = FConnectionOwner;
public:
	FServer(uint32_t ThreadNumber = 0);
	virtual ~FServer();

	void Run(uint16_t Port);
	void WaitForClientConnection();
	void Stop();
protected:
	//void OnConnectionConnected(std::shared_ptr<SConnection> ConnectionPtr) 
	//{
	//	Super::OnConnectionConnected(ConnectionPtr);
	//	//do something
	//};
	//void OnConnectionDisconnected(std::shared_ptr<SConnection> ConnectionPtr) 
	//{
	//	//do something
	//	Super::OnConnectionDisconnected(ConnectionPtr);
	//};


	bool CheckRunCallOnce();
#ifndef NDEBUG
	bool RunCallOnceFlag{ false };
#endif // !NDEBUG
};

#pragma warning(pop)
