#include "Connection.h"
#include "ConnectionOwner.h"
#include "Logger.h"

SConnection::SConnection(asio::ip::tcp::socket InSocket, FConnectionOwner& InOwner, FSingleThreadIoContext& InSingleThreadIoContext)
	: Socket(std::move(InSocket))
	, Owner(InOwner)
	, SingleThreadIoContext(InSingleThreadIoContext)
	, RecvFrom(Owner.RecvFrom)
{
	Owner.IncreaseConnectionNumber(this);
}

SConnection::~SConnection()
{
	std::promise<void> Promise;
	std::future<void> Future = Promise.get_future();
	asio::dispatch(
		SingleThreadIoContext.IoContext,
		[&]() mutable {
			if (SocketState == ESocketState::ESS_Connected)
			{
				std::error_code ErrorCode;
				Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
				if (ErrorCode) OnErrorCode(ErrorCode);
			}
			Promise.set_value();
		});
	Future.get();
	Owner.DecreaseConnectionNumber(this);
}

const char* SConnection::GetNetworkName() { return NetworkName.c_str(); }

ESocketState SConnection::GetSocketState() { return SocketState; }

void SConnection::Disconnect()
{
	// 1. Call shutdown() to indicate that you will not write any more data to the socket.
	// 2. Continue to(async - ) read from the socket until you get either an error or the connection is closed.
	// 3. Now close() the socket(in the async read handler).
	auto Self(this->shared_from_this());
	asio::dispatch(
		SingleThreadIoContext.IoContext,
		[this, Self]() mutable {
			if (SocketState == ESocketState::ESS_Connected)
			{
				std::error_code ErrorCode;
				Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
				if (ErrorCode) OnErrorCode(ErrorCode);
				if (Socket.is_open()) Socket.close(ErrorCode);
				if (ErrorCode) OnErrorCode(ErrorCode);
			}
		});
}

void SConnection::Send(const std::vector<uint8_t>& Data)
{
	SingleThreadIoContext.IoContext.post(
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
	SingleThreadIoContext.IoContext.post(
		[this, Self = shared_from_this(), Data = std::move(Data)]{
			SendTo.emplace_back(std::move(Data));
			if (SendTo.size() <= 1)
			{
				WriteHeader();
			}
		});
}

std::string SConnection::MakeNetworkName(const asio::ip::tcp::socket& Socket)
{
	return fmt::format("{:s}:{:d}", Socket.remote_endpoint().address().to_string(), Socket.remote_endpoint().port());
}

void SConnection::OnRecvData(std::vector<uint8_t>& Data) 
{
	uint8_t* BodyPtr = Data.data() + sizeof(uint32_t);
	uint32_t BodySize = *(uint32_t*)Data.data();
	std::string FormatString;
	FormatString.reserve(BodySize * 2 );
	for (size_t i = 0; i < BodySize; i++)
	{
		FormatString.append(fmt::format("{:02x}", *(BodyPtr + i)));
	}
	GLog(ELL_Debug, "<{:s}> Recv [{}]", GetNetworkName(), FormatString);
}

void SConnection::OnErrorCode(const std::error_code& ErrorCode)
{
	if (asio::error::eof == ErrorCode)
	{
		// remote is shutdown if eof 
	}
	else
	{
		GLog(ELL_Warning, "<{:s}> Socket Error : {:s}\n", NetworkName, DefaultCodePageToUTF8(ErrorCode.message()));
	}
	if (SocketState == ESocketState::ESS_Connected)
	{
		Socket.close();
		SocketState = ESocketState::ESS_Disconnected;
		Owner.PushTask([Self = shared_from_this()]{ Self->Owner.OnDisconnected(Self); });
	}
}

void SConnection::ReadHeader()
{
	auto Self(this->shared_from_this());
	DataTemporaryRead.resize(sizeof(uint32_t));
	asio::async_read(
		Socket, 
		asio::buffer(
			DataTemporaryRead.data(), 
			sizeof(uint32_t)),
		[this, Self](std::error_code ErrorCode, std::size_t Length)
		{
			if (!ErrorCode)
			{
				uint32_t BodySize = *(uint32_t*)DataTemporaryRead.data();
				if (BodySize > 0)
				{
					DataTemporaryRead.resize(sizeof(uint32_t) + BodySize);
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
			DataTemporaryRead.data() + sizeof(uint32_t),
			*(uint32_t*)DataTemporaryRead.data()),
		[this, Self](std::error_code ErrorCode, std::size_t length)
		{
			if (!ErrorCode)
			{
				RecvFrom.Enqueue(FDataOwner(Self, std::move(DataTemporaryRead)));
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
			SendTo.front().data(), 
			sizeof(uint32_t)),
		SingleThreadIoContext.IoContext.wrap(
			[this, Self](std::error_code ErrorCode, std::size_t Length){
				if (!ErrorCode)
				{
					uint32_t BodySize = *(uint32_t*)SendTo.front().data();
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
			SendTo.front().data() + sizeof(uint32_t), 
			*(uint32_t*)SendTo.front().data()),
		SingleThreadIoContext.IoContext.wrap(
			[this, Self](std::error_code ErrorCode, std::size_t Length){
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

void SConnection::ConnectToRemote()
{
	NetworkName = MakeNetworkName(Socket);
	SocketState = ESocketState::ESS_Connected;
	Owner.PushTask([Self = shared_from_this()]{ Self->Owner.OnConnected(Self); });
	ReadHeader();
}

