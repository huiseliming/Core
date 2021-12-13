#include "ConnectionOwner.h"
#include "asio.hpp"
#include "Global.h"
#include "Connection.h"

struct FConnectionOwner::FImpl {
	FImpl()
		: IoContext()
		, Acceptor(IoContext)
		, IdleWorkerPtr(new asio::io_context::work(IoContext))
	{
	}
	asio::io_context IoContext;
	asio::ip::tcp::acceptor Acceptor;
	std::vector<std::thread> IoContextThreads;
	std::unique_ptr<asio::io_context::work> IdleWorkerPtr;

	TQueue<FDataOwner, EQueueMode::MPSC> RecvFrom;
	std::atomic<uint32_t> ConnectionCounter{ 0 };
	TQueue<std::function<void()>, EQueueMode::MPSC> Tasks;

	uint32_t ConnectedConnection{ 0 };
	std::vector<std::shared_ptr<SConnection>> WaitCleanConnections;
	std::unordered_map<std::string, std::shared_ptr<SConnection>> ConnectionMap;

#ifndef NDEBUG
	// Check default OnConnectionConnected and OnConnectionDisconnected must be coexist
	bool DefaultConnectedAndDisconnectedUsageFlag{ false };
	void SetDefaultConnectedAndDisconnectedUsage() { DefaultConnectedAndDisconnectedUsageFlag = true; }
	bool CheckDefaultConnectedAndDisconnectedUsage() { return DefaultConnectedAndDisconnectedUsageFlag;  }

	std::thread::id OwnerThreadId;
	std::vector<std::thread::id> IoContextThreadIds;
	bool RunInOwnerThread()
	{
		return OwnerThreadId == std::this_thread::get_id();
	}
	bool RunInIoContext()
	{
		for (size_t i = 0; i < IoContextThreadIds.size(); i++)
		{
			if (IoContextThreadIds[i] == std::this_thread::get_id())
				return true;
		}
		return false;
	}
#else
	void SetConnectedAndDisconnectedUsageChecked() {  }
	bool IsConnectedAndDisconnectedUsageChecked() { return true; }
	bool RunInOwnerThread() { return true; }
	bool RunInIoContext() { return true; }
#endif // NDEBUG
};

FConnectionOwner::FConnectionOwner(uint32_t ThreadNumber)
	: Impl(new FImpl())
{
#ifndef NDEBUG
	Impl->OwnerThreadId = std::this_thread::get_id();
#endif // NDEBUG
	if (ThreadNumber == 0)
		ThreadNumber = std::thread::hardware_concurrency();
	GLogger->Log(ELogLevel::kDebug, "IoContext Work Thread Count {:d}", ThreadNumber);
	for (uint32_t i = 0; i < ThreadNumber; i++) {
		Impl->IoContextThreads.emplace_back(std::thread([&] {
#ifndef NDEBUG
			static std::mutex Mutex;
			{
				std::lock_guard<std::mutex> Lock(Mutex);
				Impl->IoContextThreadIds.push_back(std::this_thread::get_id());
			}
#endif // NDEBUG
			Impl->IoContext.run();
			}));
	}
}

FConnectionOwner::~FConnectionOwner()
{
	for (auto Connection : GetConnectionMap())
	{
		Connection.second->Disconnect();
	}
	while (Impl->ConnectionCounter != 0){
		ProcessEvent();
		std::this_thread::yield();
	}
	Impl->IdleWorkerPtr.reset();
	for (uint32_t i = 0; i < Impl->IoContextThreadIds.size(); i++)
	{
		if (Impl->IoContextThreads[i].joinable())
			Impl->IoContextThreads[i].join();
	}
}

asio::io_context& FConnectionOwner::GetIoContext() { return Impl->IoContext; }

TQueue<FDataOwner, EQueueMode::MPSC>& FConnectionOwner::GetRecvQueue() { return Impl->RecvFrom; }

void FConnectionOwner::IncreaseConnectionCounter() {
	GLogger->Log(ELogLevel::kDebug, "Current Live Connection Count {:d}", ++(Impl->ConnectionCounter));
}

void FConnectionOwner::DecreaseConnectionCounter() {
	GLogger->Log(ELogLevel::kDebug, "Current Live Connection Count {:d}", --(Impl->ConnectionCounter));
}

void FConnectionOwner::PushTask(std::function<void()> Task) { Impl->Tasks.Enqueue(std::move(Task)); }

void FConnectionOwner::ProcessTask()
{
	std::function<void()> Task;
	while (Impl->Tasks.Dequeue(Task))
		Task();
}

uint32_t FConnectionOwner::ProcessMessage()
{
	FDataOwner DataOwner;
	uint32_t ProcessMessageCounter = 0;
	while (Impl->RecvFrom.Dequeue(DataOwner)){
		ProcessMessageCounter++;
		OnRecvData(DataOwner.Owner, DataOwner.Data);
	}
	return ProcessMessageCounter;
}

uint32_t FConnectionOwner::ProcessEvent()
{
	assert(Impl->RunInOwnerThread());
	ProcessTask();
	return ProcessMessage();
}

void FConnectionOwner::OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data)
{
	std::string BinaryString = fmt::format("<{:s}> Recv : ", ConnectionPtr->GetNetworkName());
	uint8_t* BodyPtr = reinterpret_cast<uint8_t*>(ConnectionPtr->GetNetworkProtocol()->GetBodyPtr(Data.data()));
	uint32_t BodySize = ConnectionPtr->GetNetworkProtocol()->GetBodySize(Data.data());
	BinaryString.reserve(BinaryString.size() + BodySize * 4);
	for (size_t i = 0; i < BodySize; i++)
	{
		if (i == 0) BinaryString.append("\n");
		BinaryString.append(fmt::format("{:#02x} ", *(BodyPtr + i)));
	}
	GLogger->Log(ELogLevel::kInfo, BinaryString);
}

void FConnectionOwner::OnConnected(std::shared_ptr<SConnection> ConnectionPtr)
{
	assert(Impl->RunInOwnerThread());
	auto ExistConnection = Impl->ConnectionMap.find(ConnectionPtr->GetNetworkName());
	if (ExistConnection != std::end(Impl->ConnectionMap)) {
		GLogger->Log(ELogLevel::kWarning, "Existed Connection<{:s}>", ConnectionPtr->GetNetworkName());
		ExistConnection->second->Disconnect();
		Impl->WaitCleanConnections.push_back(ExistConnection->second);
		ExistConnection->second = ConnectionPtr;
	}
	else
	{
		Impl->ConnectionMap.insert(std::make_pair<>(ConnectionPtr->GetNetworkName(), ConnectionPtr));
	}
	GLogger->Log(ELogLevel::kInfo, "Connection<{}> Connected", ConnectionPtr->GetNetworkName());
	GLogger->Log(ELogLevel::kDebug, "Current Connected Connection Count {:d}", ++(Impl->ConnectedConnection));
}

void FConnectionOwner::OnDisconnected(std::shared_ptr<SConnection> ConnectionPtr)
{
	assert(Impl->RunInOwnerThread());
	for (auto it = Impl->WaitCleanConnections.begin(); it != Impl->WaitCleanConnections.end(); it++)
	{
		if (ConnectionPtr == *it)
		{
			Impl->WaitCleanConnections.erase(it);
			goto PrintToLogger;
		}
	}
	assert(Impl->ConnectionMap.erase(ConnectionPtr->GetNetworkName()) == 1);
PrintToLogger:
	GLogger->Log(ELogLevel::kInfo, "Connection<{}> Disconnected", ConnectionPtr->GetNetworkName());
	GLogger->Log(ELogLevel::kDebug, "Current Connected Connection Count {:d} ", --(Impl->ConnectedConnection));
}

std::unordered_map<std::string, std::shared_ptr<SConnection>>& FConnectionOwner::GetConnectionMap()
{
	Impl->RunInOwnerThread();
	return Impl->ConnectionMap;
}

bool FConnectionOwner::RunInOwnerThread()
{
	return Impl->RunInOwnerThread();
}

bool FConnectionOwner::RunInIoContext()
{
	return Impl->RunInIoContext();
}

FClient::FClient(uint32_t ThreadNumber)
	: FConnectionOwner(ThreadNumber)
{
}

std::future<std::shared_ptr<SConnection>> FClient::ConnectToServer(std::string Address, uint16_t Port, std::function<void(std::shared_ptr<SConnection>)> ConnectCallback)
{
	assert(Impl->RunInOwnerThread());
	std::shared_ptr<SConnection> Connection;
	std::promise<std::shared_ptr<SConnection>> Promise;
	std::future<std::shared_ptr<SConnection>> ConnectionFuture = Promise.get_future();
	try
	{
		//assert(Connection->SetConnectingSocketState());
		asio::ip::tcp::resolver Resolver(Impl->IoContext);
		auto Endpoints = Resolver.resolve(Address, std::to_string(Port));
		std::shared_ptr<asio::ip::tcp::socket> Socket(new asio::ip::tcp::socket(GetIoContext()));
		asio::async_connect(*Socket, Endpoints, [this, Socket,  Connection, Promise = std::move(Promise), ConnectCallback = std::move(ConnectCallback)](std::error_code ErrorCode, asio::ip::tcp::endpoint endpoint) mutable {
			if (!ErrorCode)
			{
				Connection = std::make_shared<SConnection>(std::move(*Socket), *this);
			}
			PushTask([Connection, ConnectCallback = std::move(ConnectCallback)]{ ConnectCallback(Connection); });
			if (Connection)
				Connection->ConnectToServer();
			Promise.set_value(Connection);
		});
	}
	catch (const std::exception& Exception)
	{
		GLogger->Log(ELogLevel::kWarning, "Exception: {}\n", Exception.what());
		Promise.set_value(std::shared_ptr<SConnection>());
	}
	return std::move(ConnectionFuture);
}

FServer::FServer(uint32_t ThreadNumber)
	: FConnectionOwner(ThreadNumber)
{
}

FServer::~FServer() {
	Stop();
}

void FServer::Run(uint16_t Port)
{
	if (!CheckRunCallOnce()) return;
	GLogger->Log(ELogLevel::kInfo, "Server Run");
	auto Endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), Port);
	Impl->Acceptor.open(Endpoint.protocol());
	Impl->Acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	Impl->Acceptor.bind(Endpoint);
	Impl->Acceptor.listen();
	WaitForClientConnection();
}

void FServer::WaitForClientConnection()
{
	Impl->Acceptor.async_accept(
		[this](std::error_code ErrorCode, asio::ip::tcp::socket Socket)
		{
			if (!ErrorCode)
			{
				std::shared_ptr<SConnection> NewConnection = std::make_shared<SConnection>(std::move(Socket), *this);
				NewConnection->ConnectToClient();
				WaitForClientConnection();
			}
			else
			{
				GLogger->Log(ELogLevel::kWarning, "Accept Connection Error: ", ErrorCode.message());
			}
		}
	);
}

void FServer::Stop()
{
	Impl->Acceptor.close();
	GLogger->Log(ELogLevel::kInfo, "Server Stop");
}

bool FServer::CheckRunCallOnce()
{
#ifndef NDEBUG
	if (RunCallOnceFlag)
	{
		GLogger->Log(ELogLevel::kInfo, "CServer::Run just call once, please renew a CServer");
		return false;
	}
	RunCallOnceFlag = true;
#endif // !NDEBUG
	return true;
}
