#include "Connection.h"
#include "ConnectionOwner.h"

SConnection::SConnection(asio::ip::tcp::socket Socket, FConnectionOwner& Owner, INetworkProtocol* InNetworkProtocol)
	: Socket(std::move(Socket))
	, Owner(Owner)
	, NetworkProtocol(InNetworkProtocol)
	, IoContext(Owner.GetIoContext())
	, IoContextWriteStrand(IoContext)
	, RecvFrom(Owner.GetRecvQueue())
{
	Owner.IncreaseConnectionCounter();
}

SConnection::~SConnection()
{
	std::promise<void> Promise;
	std::future<void> Future = Promise.get_future();
	asio::dispatch(IoContext, [&]() mutable {
		if (Socket.is_open())
		{
			std::error_code ErrorCode;
			Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
			if (ErrorCode) OnErrorCode(ErrorCode);
			Socket.close();
		}
		Promise.set_value();
		});
	Future.get();
	Owner.DecreaseConnectionCounter();
}

INetworkProtocol* SConnection::GetNetworkProtocol() { return NetworkProtocol; }

const char* SConnection::GetNetworkName() { return NetworkName.c_str(); }

ESocketState SConnection::GetSocketState() { return State; }

void SConnection::Disconnect()
{
	auto Self(this->shared_from_this());
	asio::dispatch(IoContext, [this, Self]() mutable {
		if (Socket.is_open())
		{
			std::error_code ErrorCode;
			Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
			if (ErrorCode) OnErrorCode(ErrorCode);
			Socket.close();
		}
		});
}

void SConnection::Send(const std::vector<uint8_t>& Data)
{
	IoContextWriteStrand.post(
		[this, Self = shared_from_this(), Data] {
			SendTo.emplace_back(std::move(Data));
			if (SendTo.size() <= 1)
			{
				WriteHeader();
			}
		});
}

void SConnection::Send(std::vector<uint8_t>&& Data)
{
	IoContextWriteStrand.post(
		[this, Self = shared_from_this(), Data = std::move(Data)]{
			SendTo.emplace_back(std::move(Data));
			if (SendTo.size() <= 1)
			{
				WriteHeader();
			}
		});
}

void SConnection::OnErrorCode(const std::error_code& ErrorCode)
{
	assert(Owner.RunInIoContext());
	if (asio::error::eof == ErrorCode)
	{
		// remote is shutdown if eof 
	}
	GLogger->Log(ELogLevel::kWarning, "<{:s}> Socket Error : {:s}\n", NetworkName, ErrorCode.message());
	ESocketState ExpectedSocketState = ESocketState::kConnected;
	if (State.compare_exchange_strong(ExpectedSocketState, ESocketState::kDisconnected))
		Owner.PushTask([Self = shared_from_this()]{ Self->Owner.OnDisconnected(Self); });
}

void SConnection::ReadHeader()
{
	auto Self(this->shared_from_this());
	DataTemporaryRead.resize(NetworkProtocol->GetRecvHeaderSize());
	asio::async_read(
		Socket, 
		asio::buffer(
			DataTemporaryRead.data(), 
			NetworkProtocol->GetRecvHeaderSize()),
		[this, Self](std::error_code ErrorCode, std::size_t length)
		{
			if (!ErrorCode)
			{
				uint32_t BodySize = NetworkProtocol->GetRecvBodySize(DataTemporaryRead.data());
				if (BodySize > 0)
				{
					DataTemporaryRead.resize(NetworkProtocol->GetRecvHeaderSize() + BodySize);
					ReadBody();
				}
				else
				{
					RecvFrom.Enqueue(FDataOwner{ Self, std::move(DataTemporaryRead) });
					ReadHeader();
				}
			}
			else
			{
				OnErrorCode(ErrorCode);
			}
		});
}

void SConnection::ReadBody()
{
	auto Self(this->shared_from_this());
	asio::async_read(
		Socket, 
		asio::buffer(
			NetworkProtocol->GetRecvBodyPtr(DataTemporaryRead.data()), 
			NetworkProtocol->GetRecvBodySize(DataTemporaryRead.data())),
		[this, Self](std::error_code ErrorCode, std::size_t length)
		{
			if (!ErrorCode)
			{
				RecvFrom.Enqueue(FDataOwner{ Self, std::move(DataTemporaryRead) });
				ReadHeader();
			}
			else
			{
				OnErrorCode(ErrorCode);
			}
		});
}

void SConnection::WriteHeader()
{
	auto Self(this->shared_from_this());
	asio::async_write(
		Socket, 
		asio::buffer(
			NetworkProtocol->GetSendHeaderPtr(SendTo.front().data()), 
			NetworkProtocol->GetSendHeaderSize()),
		IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
					uint32_t BodySize = NetworkProtocol->GetSendBodySize(SendTo.front().data());
					if (BodySize > 0)
					{
						WriteBody();
					}
					else
					{
						SendTo.pop_front();
						if (!SendTo.empty())
						{
							WriteHeader();
						}
					}
				}
				else
				{
					OnErrorCode(ErrorCode);
				}
			}));
}

void SConnection::WriteBody()
{
	auto Self(this->shared_from_this());
	asio::async_write(
		Socket, 
		asio::buffer(
			NetworkProtocol->GetSendBodyPtr(SendTo.front().data()), 
			NetworkProtocol->GetSendBodySize(SendTo.front().data())),
		IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
					SendTo.pop_front();
					if (!SendTo.empty())
					{
						WriteHeader();
					}
				}
				else
				{
					OnErrorCode(ErrorCode);
				}
			}));
}

void SConnection::ConnectToServer()
{
	ESocketState ExpectedSocketState = ESocketState::kInit;
	assert(State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnected));
	ConnectToRemote();
}

void SConnection::ConnectToClient()
{
	ESocketState ExpectedSocketState = ESocketState::kInit;
	assert(State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnected));
	ConnectToRemote();
}

void SConnection::ConnectToRemote()
{
	NetworkName = fmt::format("{:s}:{:d}", Socket.remote_endpoint().address().to_string(), Socket.remote_endpoint().port());
	Owner.PushTask([Self = shared_from_this()]{ Self->Owner.OnConnected(Self); });
	ReadHeader();
}

void SConnection::ConnectFailed()
{
	ESocketState ExpectedSocketState = ESocketState::kConnecting;
	assert(State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnectFailed));
}

bool SConnection::SetConnectingSocketState()
{
	ESocketState ExpectedSocketState = ESocketState::kInit;
	return State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnecting);
}

void* SConnection::GetSocket(){ return &(Socket); }
