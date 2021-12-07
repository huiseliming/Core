#pragma once
#include <functional>
#include <future>
#include <memory>
#include "CoreApi.h"
#include "Queue.h"
#include "Message.h"


namespace asio {
	class io_context;
}

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

class CORE_API FTcpServer
{
	struct FImpl;
protected:
	FTcpServer(uint32_t InNumberOfWorkers = 0, uint32_t InMaxNumOfConnectionsPerIoWorker = 256);
public:
	~FTcpServer();

	std::unique_ptr<FImpl> Impl;
};

class CORE_API FTcpClientManager
{
	struct FImpl;
public:
	static FTcpClientManager& Instance();
protected:
	FTcpClientManager();
public:
	~FTcpClientManager();

	std::unique_ptr<FImpl> Impl;
};


class CORE_API IConnectionOwner
{
public:
	//virtual asio::io_context& GetIoContext() = 0;
	//virtual void OnConnected(std::shared_ptr<SConnection> ConnectionPtr) = 0;
	//virtual void OnDisconnected(std::shared_ptr<SConnection> ConnectionPtr) = 0;
	//virtual void OnData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data) = 0;

	friend class SConnection;
public:
	struct FImpl;

	IConnectionOwner(uint32_t ThreadNumber = 0);
	IConnectionOwner(const IConnectionOwner&) = delete;
	IConnectionOwner(IConnectionOwner&& Other) = delete;
	IConnectionOwner& operator=(const IConnectionOwner&) = delete;
	IConnectionOwner& operator=(IConnectionOwner&& Other) = delete;
	virtual ~IConnectionOwner();

	asio::io_context& GetIoContext();


	TQueue<FMessage, EQueueMode::MPSC>& GetRecvQueue();
	void IncreaseConnectionCounter();
	void DecreaseConnectionCounter();

	void PushTask(std::function<void()>&& Task);
	void ProcessTask();
	uint32_t ProcessMessage();
	uint32_t ProcessEvent();

	std::unordered_map<std::string, std::shared_ptr<SConnection>>& GetConnectionMap();

protected:
	virtual void OnMessage(std::shared_ptr<SConnection> ConnectionPtr, FMessageData& MessageData);
	virtual void OnConnectionConnected(std::shared_ptr<SConnection> ConnectionPtr);
	virtual void OnConnectionDisconnected(std::shared_ptr<SConnection> ConnectionPtr);

protected:
	bool RunInOwnerThread();
	bool RunInIoContext();

	std::unique_ptr<FImpl> Impl;

};


class CORE_API CClient : public IConnectionOwner
{
	using Super = IConnectionOwner;
public:
	CClient(uint32_t ThreadNumber);

	std::future<std::shared_ptr<SConnection>> ConnectToServer(std::string Address, uint16_t Port);
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

class CORE_API CServer : public IConnectionOwner
{
	using Super = IConnectionOwner;
public:
	CServer(uint32_t ThreadNumber = 0);
	virtual ~CServer();

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
