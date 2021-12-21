#pragma once
#include <functional>
#include <future>
#include "Queue.h"
#include "Connection.h"

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

struct FSingleThreadIoContext
{
	FSingleThreadIoContext(int32_t InIndex)
		: IoContext()
		, IdleWorker(new asio::io_context::work(IoContext))
		, Index(InIndex)
	{
		Thread = std::move(
			std::thread(
				[&] {
					IoContext.run();
				}));
	}
	FSingleThreadIoContext(const FSingleThreadIoContext&) = delete;
	FSingleThreadIoContext(FSingleThreadIoContext&&) = delete;
	FSingleThreadIoContext& operator=(const FSingleThreadIoContext&) = delete;
	FSingleThreadIoContext& operator=(FSingleThreadIoContext&&) = delete;
	~FSingleThreadIoContext()
	{
		IdleWorker.reset();
		Thread.join();
	}

	void IncreaseSocketNum() { SocketNum++; }
	void DecreaseSocketNum() { SocketNum--; }

	asio::io_context IoContext;
	std::thread Thread;
	std::unique_ptr<asio::io_context::work> IdleWorker;
	std::atomic<int32_t> SocketNum{ 0 };
	uint32_t Index;
};


class CORE_API FConnectionOwner
{
	friend class SConnection;
public:

	FConnectionOwner(uint32_t ThreadNumber = 0);
	FConnectionOwner(const FConnectionOwner&) = delete;
	FConnectionOwner(FConnectionOwner&& Other) = delete;
	FConnectionOwner& operator=(const FConnectionOwner&) = delete;
	FConnectionOwner& operator=(FConnectionOwner&& Other) = delete;
	virtual ~FConnectionOwner();

	void Listen(std::string IP, uint32_t Port);
	bool ConnectToServer(std::string IP, uint32_t Port, std::function<void(std::shared_ptr<SConnection>)> ResultCallback);
	void WaitForClientConnection();

	void ProcessTask();
	uint32_t ProcessMessage();
	uint32_t ProcessTaskAndMessage();

protected:
	virtual FSingleThreadIoContext& RequestIoContext();

	void IncreaseConnectionNumber(SConnection* Connection);
	void DecreaseConnectionNumber(SConnection* Connection);
	void PushTask(std::function<void()> Task);;

	std::unordered_map<std::string, std::shared_ptr<SConnection>>& GetConnectionMap();

protected:
	virtual std::shared_ptr<SConnection> CreateConnection(asio::ip::tcp::socket& Socket, FSingleThreadIoContext& SingleThreadIoContext);
	virtual void OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data);
	virtual void OnConnected(std::shared_ptr<SConnection> ConnectionPtr);
	virtual void OnDisconnected(std::shared_ptr<SConnection> ConnectionPtr);

protected:

	std::unique_ptr<FSingleThreadIoContext> AcceptorIoContext;
	asio::ip::tcp::acceptor Acceptor;
	
	std::vector<std::unique_ptr<FSingleThreadIoContext>> IoContexts;

	TQueue<FDataOwner, EQueueMode::MPSC> RecvFrom;
	TQueue<std::function<void()>, EQueueMode::MPSC> Tasks;

	// The ConnectionOwner own SConnection number
	std::atomic<uint32_t> ConnectionNumber{ 0 };
	// The number of connections that have called OnConnected
	uint32_t ConnectedConnectionNumber{ 0 };

	std::vector<std::shared_ptr<SConnection>> WaitCleanConnections;
	std::unordered_map<std::string, std::shared_ptr<SConnection>> ConnectionMap;

};






#pragma warning(pop)
