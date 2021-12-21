#include "ConnectionOwner.h"
#include "asio.hpp"
#include "Connection.h"
#include "Logger.h"

FConnectionOwner::FConnectionOwner(uint32_t ThreadNumber)
	: AcceptorIoContext(new FSingleThreadIoContext(0))
	, Acceptor(AcceptorIoContext->IoContext)
{
	if (ThreadNumber == 0)
		ThreadNumber = std::thread::hardware_concurrency();
	GLog(ELL_Debug, "IoContext Work Thread Count {:d}", ThreadNumber);
	for (int32_t i = 0; i < ThreadNumber; i++) 
		IoContexts.emplace_back(std::make_unique<FSingleThreadIoContext>(i));
}

FConnectionOwner::~FConnectionOwner()
{
	if (Acceptor.is_open())
	{
		Acceptor.close();
	}
	for (auto Connection : ConnectionMap)
	{
		Connection.second->Disconnect();
	}
	while (ConnectionNumber != 0){
		ProcessTaskAndMessage();
		std::this_thread::yield();
	}
	AcceptorIoContext.reset();
	IoContexts.clear();
}

void FConnectionOwner::Listen(std::string IP, uint32_t Port)
{
	GLog(ELL_Info, "Server Run");
	asio::ip::tcp::endpoint Endpoint;
	if (!IP.empty()) Endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(IP.c_str()), Port);
	else  Endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), Port);
	Acceptor.open(Endpoint.protocol());
	Acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	Acceptor.bind(Endpoint);
	Acceptor.listen();
	WaitForClientConnection();
}

bool FConnectionOwner::ConnectToServer(std::string IP, uint32_t Port, std::function<void(std::shared_ptr<SConnection>)> ResultCallback)
{
	try
	{
		//assert(Connection->SetConnectingSocketState());
		asio::ip::tcp::resolver Resolver(AcceptorIoContext->IoContext);
		auto Endpoints = Resolver.resolve(IP, std::to_string(Port));
		auto& IoContext = RequestIoContext();
		std::shared_ptr<asio::ip::tcp::socket> Socket(new asio::ip::tcp::socket(IoContext.IoContext));
		asio::async_connect(*Socket, Endpoints, [this, &IoContext = IoContext, Socket, ResultCallback = std::move(ResultCallback)](std::error_code ErrorCode, asio::ip::tcp::endpoint endpoint) mutable {
			std::shared_ptr<SConnection> Connection;
			if (!ErrorCode) Connection = CreateConnection(*Socket, IoContext);
			if (Connection) Connection->ConnectToRemote();
			PushTask([Connection, ResultCallback = std::move(ResultCallback)]{ ResultCallback(Connection); });
		});
		return true;
	}
	catch (const std::exception& Exception)
	{
		GLog(ELL_Warning, "Exception: {}\n", Exception.what());
	}
	return false;
}

void FConnectionOwner::WaitForClientConnection()
{
	auto& IoContext = RequestIoContext();
	Acceptor.async_accept(
		IoContext.IoContext,
		[this, &IoContext = IoContext](std::error_code ErrorCode, asio::ip::tcp::socket Socket)
		{
			if (!ErrorCode)
			{
				std::shared_ptr<SConnection> NewConnection = CreateConnection(Socket, IoContext);
				NewConnection->ConnectToRemote();
				WaitForClientConnection();
			}
			else
			{
				GLog(ELL_Warning, "Accept Connection Error: ", ErrorCode.message());
			}
		}
	);
}

void FConnectionOwner::ProcessTask()
{
	std::function<void()> Task;
	while (Tasks.Dequeue(Task))
		Task();
}

uint32_t FConnectionOwner::ProcessMessage()
{
	FDataOwner DataOwner;
	uint32_t ProcessMessageCounter = 0;
	while (RecvFrom.Dequeue(DataOwner)) {
		ProcessMessageCounter++;
		OnRecvData(DataOwner.Owner, DataOwner.Data);
	}
	return ProcessMessageCounter;
}

uint32_t FConnectionOwner::ProcessTaskAndMessage()
{
	ProcessTask();
	return ProcessMessage();
}

FSingleThreadIoContext& FConnectionOwner::RequestIoContext()
{
	uint32_t IoContextIndex = 0;
	for (size_t i = 0; i < IoContexts.size(); i++)
	{
		if (IoContexts[IoContextIndex]->SocketNum > IoContexts[i]->SocketNum)
		{
			IoContextIndex = i;
		}
	}
	return *IoContexts[IoContextIndex];
}

void FConnectionOwner::IncreaseConnectionNumber(SConnection* Connection) {
	Connection->SingleThreadIoContext.IncreaseSocketNum();
	GLog(ELL_Debug, "Current Live Connection Count {:d}", ++ConnectionNumber);
}

void FConnectionOwner::DecreaseConnectionNumber(SConnection* Connection) {
	Connection->SingleThreadIoContext.DecreaseSocketNum();
	GLog(ELL_Debug, "Current Live Connection Count {:d}", --ConnectionNumber);
}

void FConnectionOwner::PushTask(std::function<void()> Task) { Tasks.Enqueue(std::move(Task)); }

// todo : SET THE IOCONTEXT INDEX
std::shared_ptr<SConnection> FConnectionOwner::CreateConnection(asio::ip::tcp::socket& Socket, FSingleThreadIoContext& SingleThreadIoContext)
{
	return std::make_shared<SConnection>(std::move(Socket), *this, SingleThreadIoContext);
}

void FConnectionOwner::OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data)
{
	ConnectionPtr->OnRecvData(ConnectionPtr, Data);
}

void FConnectionOwner::OnConnected(std::shared_ptr<SConnection> ConnectionPtr)
{
	auto ExistConnection = ConnectionMap.find(ConnectionPtr->GetNetworkName());
	if (ExistConnection != std::end(ConnectionMap))
	{
		// remove action in ondisconnected, so save connection to WaitCleanConnections
		GLog(ELL_Warning, "Existed Connection<{:s}>", ConnectionPtr->GetNetworkName());
		ExistConnection->second->Disconnect();
		WaitCleanConnections.push_back(ExistConnection->second);
		ExistConnection->second = ConnectionPtr;
	}
	else
	{
		ConnectionMap.insert(std::make_pair<>(ConnectionPtr->GetNetworkName(), ConnectionPtr));
	}
	GLog(ELL_Info, "Connection<{}> Connected", ConnectionPtr->GetNetworkName());
	GLog(ELL_Debug, "Current Connected Connection Count {:d}", ++ConnectedConnectionNumber);
}

void FConnectionOwner::OnDisconnected(std::shared_ptr<SConnection> ConnectionPtr)
{
	bool bRemoveInWaitCleanConnections = false;
	for (auto it = WaitCleanConnections.begin(); it != WaitCleanConnections.end(); it++)
	{
		if (ConnectionPtr == *it)
		{
			WaitCleanConnections.erase(it);
			bRemoveInWaitCleanConnections = true;
			break;
		}
	}
	if (!bRemoveInWaitCleanConnections)
		assert(ConnectionMap.erase(ConnectionPtr->GetNetworkName()) == 1);
	GLog(ELL_Info, "Connection<{}> Disconnected", ConnectionPtr->GetNetworkName());
	GLog(ELL_Debug, "Current Connected Connection Count {:d} ", --ConnectedConnectionNumber);
}

std::unordered_map<std::string, std::shared_ptr<SConnection>>& FConnectionOwner::GetConnectionMap()
{
	return ConnectionMap;
}
