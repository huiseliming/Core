#include "ConnectionOwner.h"
#include "asio.hpp"
#include "Global.h"
#include "Connection.h"

struct CConnectionOwner::FImpl {
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

	TQueue<FMessage, EQueueMode::MPSC> RecvFrom;
	std::atomic<uint32_t> ConnectionCounter{ 0 };
	TQueue<std::function<void()>, EQueueMode::MPSC> Tasks;

	uint32_t ConnectedConnection{ 0 };
	std::vector<std::shared_ptr<CConnection>> WaitCleanConnections;
	std::unordered_map<std::string, std::shared_ptr<CConnection>> ConnectionMap;

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

CConnectionOwner::CConnectionOwner(uint32_t ThreadNumber)
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

CConnectionOwner::~CConnectionOwner()
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

asio::io_context& CConnectionOwner::GetIoContext() { return Impl->IoContext; }

TQueue<FMessage, EQueueMode::MPSC>& CConnectionOwner::GetRecvQueue() { return Impl->RecvFrom; }

void CConnectionOwner::IncreaseConnectionCounter() {
	GLogger->Log(ELogLevel::kDebug, "Current Live Connection Count {:d}", ++(Impl->ConnectionCounter));
}

void CConnectionOwner::DecreaseConnectionCounter() {
	GLogger->Log(ELogLevel::kDebug, "Current Live Connection Count {:d}", --(Impl->ConnectionCounter));
}

void CConnectionOwner::PushTask(std::function<void()>&& Task) { Impl->Tasks.Enqueue(std::move(Task)); }

void CConnectionOwner::ProcessTask()
{
	std::function<void()> Task;
	while (Impl->Tasks.Dequeue(Task))
		Task();
}

uint32_t CConnectionOwner::ProcessMessage()
{
	FMessage Message;
	uint32_t ProcessMessageCounter = 0;
	while (Impl->RecvFrom.Dequeue(Message)){
		ProcessMessageCounter++;
		OnMessage(Message.MessageOwner, Message.MessageData);
	}
	return ProcessMessageCounter;
}

uint32_t CConnectionOwner::ProcessEvent()
{
	assert(Impl->RunInOwnerThread());
	ProcessTask();
	return ProcessMessage();
}

void CConnectionOwner::OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData)
{
	std::string BinaryString = std::format("<{:s}> Recv : ", ConnectionPtr->GetNetworkName());
	BinaryString.reserve(BinaryString.size() + MessageData.GetBodySize() * 4);
	for (size_t i = 0; i < MessageData.GetBodySize(); i++)
	{
		if (i == 0) BinaryString.append("\n");
		BinaryString.append(std::format("{:#02x} ", *(MessageData.GetBody<uint8_t>() + i)));
	}
	GLogger->Log(ELogLevel::kInfo, BinaryString);
}

void CConnectionOwner::OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr)
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

void CConnectionOwner::OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr)
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

std::unordered_map<std::string, std::shared_ptr<CConnection>>& CConnectionOwner::GetConnectionMap()
{
	Impl->RunInOwnerThread();
	return Impl->ConnectionMap;
}

bool CConnectionOwner::RunInOwnerThread()
{
	return Impl->RunInOwnerThread();
}

bool CConnectionOwner::RunInIoContext()
{
	return Impl->RunInIoContext();
}

CClient::CClient(uint32_t ThreadNumber)
	: CConnectionOwner(ThreadNumber)
{
}

std::future<std::shared_ptr<CConnection>> CClient::ConnectToServer(std::string Address, uint16_t Port)
{
	assert(Impl->RunInOwnerThread());
	std::shared_ptr<CConnection> Connection = std::make_shared<CConnection>(*this);
	std::promise<std::shared_ptr<CConnection>> Promise;
	std::future<std::shared_ptr<CConnection>> ConnectionFuture = Promise.get_future();
	try
	{
		assert(Connection->SetConnectingSocketState());
		asio::ip::tcp::resolver Resolver(Impl->IoContext);
		auto Endpoints = Resolver.resolve(Address, std::to_string(Port));
		asio::async_connect(*(asio::ip::tcp::socket*)Connection->GetSocket(), Endpoints,
			[this, Connection, Promise = std::move(Promise)](std::error_code ErrorCode, asio::ip::tcp::endpoint endpoint) mutable
		{
			if (!ErrorCode)
			{
				Connection->ConnectToServer();
			}
			else
			{
				Connection->ConnectFailed();
			}
			Promise.set_value(Connection);
		});
	}
	catch (const std::exception& Exception)
	{
		GLogger->Log(ELogLevel::kWarning, "Exception: {}\n", Exception.what());
		Promise.set_value(std::shared_ptr<CConnection>());
	}
	return std::move(ConnectionFuture);
}

CServer::CServer(uint32_t ThreadNumber)
	: CConnectionOwner(ThreadNumber)
{
}

CServer::~CServer() {
	Stop();
}

void CServer::Run(uint16_t Port)
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

void CServer::WaitForClientConnection()
{
	Impl->Acceptor.async_accept(
		[this](std::error_code ErrorCode, asio::ip::tcp::socket Socket)
		{
			if (!ErrorCode)
			{
				std::shared_ptr<CConnection> NewConnection = std::make_shared<CConnection>(*this);
				*(asio::ip::tcp::socket*)NewConnection->GetSocket() = std::move(Socket);
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

void CServer::Stop()
{
	Impl->Acceptor.close();
	GLogger->Log(ELogLevel::kInfo, "Server Stop");
}

bool CServer::CheckRunCallOnce()
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
