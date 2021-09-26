#include "ConnectionOwner.h"
#include <asio.hpp>
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

#ifndef NDEBUG
	std::thread::id UIThreadId;
	std::vector<std::thread::id> IoContextThreadIds;
#endif // NDEBUG
};

CConnectionOwner::CConnectionOwner(uint32_t ThreadNumber)
	: Impl(new FImpl())
{
#ifndef NDEBUG
	Impl->UIThreadId = std::this_thread::get_id();
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
	while (Impl->ConnectionCounter != 0) std::this_thread::yield();
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

void CConnectionOwner::ProcessMessage()
{
	FMessage Message;
	while (Impl->RecvFrom.Dequeue(Message))
		OnMessage(Message.MessageOwner, Message.MessageData);
}

void CConnectionOwner::ProcessEvent()
{
	ProcessTask();
	ProcessMessage();
}

bool CConnectionOwner::RunInUIThread()
#ifndef NDEBUG
{
	return Impl->UIThreadId == std::this_thread::get_id();
}
#else
{
	return true;
}
#endif // NDEBUG

bool CConnectionOwner::RunInIoContext()
#ifndef NDEBUG
{
	for (size_t i = 0; i < Impl->IoContextThreadIds.size(); i++)
	{
		if (Impl->IoContextThreadIds[i] == std::this_thread::get_id())
			return true;
	}
	return false;
}
#else
{
	return true;
}
#endif // NDEBUG


void CClient::OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData)
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

std::future<std::shared_ptr<CConnection>> CClient::ConnectToServer(std::string Address, uint16_t Port)
{
	std::shared_ptr<CConnection> Connection = std::make_shared<CConnection>(*this);
	std::promise<std::shared_ptr<CConnection>> Promise;
	std::future<std::shared_ptr<CConnection>> ConnectionFuture = Promise.get_future();
	try
	{
		//Impl->State = ESocketState::kConnecting;
		asio::ip::tcp::resolver Resolver(Impl->IoContext);
		auto Endpoints = Resolver.resolve(Address, std::to_string(Port));
		asio::async_connect(*(asio::ip::tcp::socket*)Connection->GetSocket(), Endpoints,
			[this, Connection, Promise = std::move(Promise)](std::error_code ErrorCode, asio::ip::tcp::endpoint endpoint) mutable
		{
			if (!ErrorCode)
			{
				Connection->ConnectToRemote();
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

void CServer::Run(uint16_t Port)
{
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
				NewConnection->ConnectToRemote();
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
	PushTask([&] {
		for (auto Connection : Connections)
		{
			Connection.second->Disconnect();
		}
		});
	GLogger->Log(ELogLevel::kInfo, "Server Stop");
}


void CServer::OnMessage(std::shared_ptr<CConnection> ConnectionPtr, FMessageData& MessageData)
{
	std::string BinaryString = std::format("<{:s}> Recv : ",ConnectionPtr->GetNetworkName());
	BinaryString.reserve(BinaryString.size() + MessageData.GetBodySize() * 4);
	for (size_t i = 0; i < MessageData.GetBodySize(); i++)
	{
		if (i == 0) BinaryString.append("\n");
		BinaryString.append(std::format("{:#02x} ", *(MessageData.GetBody<uint8_t>() + i)));
	}
	GLogger->Log(ELogLevel::kInfo, BinaryString);
}

void CServer::OnConnectionConnected(std::shared_ptr<CConnection> ConnectionPtr)
{
	assert(!RunInIoContext());
	auto ExistConnection = Connections.find(ConnectionPtr->GetNetworkName());
	if (ExistConnection != std::end(Connections)) {
		GLogger->Log(ELogLevel::kWarning, "Existed Connection<{:s}>", ConnectionPtr->GetNetworkName());
		ExistConnection->second->Disconnect();
		WaitCleanConnections.push_back(ExistConnection->second);
		ExistConnection->second = ConnectionPtr;
	}
	else
	{
		Connections.insert(std::make_pair<>(ConnectionPtr->GetNetworkName(), ConnectionPtr));
	}
	GLogger->Log(ELogLevel::kInfo, "Connection<{}> Connected", ConnectionPtr->GetNetworkName());
	GLogger->Log(ELogLevel::kDebug, "Current Connected Connection Count {:d}", ++ConnectedConnection);
};

void CServer::OnConnectionDisconnected(std::shared_ptr<CConnection> ConnectionPtr)
{
	assert(!RunInIoContext());
	for (auto it = WaitCleanConnections.begin(); it != WaitCleanConnections.end(); it++)
	{
		if (ConnectionPtr == *it)
		{
			WaitCleanConnections.erase(it);
			return;
		}
	}
	Connections.erase(ConnectionPtr->GetNetworkName());
	GLogger->Log(ELogLevel::kInfo, "Connection<{}> Disconnected", ConnectionPtr->GetNetworkName());
	GLogger->Log(ELogLevel::kDebug, "Current Connected Connection Count {:d} ", --ConnectedConnection);
};
