#include "Connection.h"
#include <asio.hpp>
#include "ConnectionOwner.h"
#include "Global.h"

struct CConnection::FImpl {
	FImpl(CConnectionOwner& Owner)
		: Owner(Owner)
		, IoContext(Owner.GetIoContext())
		, IoContextWriteStrand(IoContext)
		, Socket(IoContext)
		, RecvFrom(Owner.GetRecvQueue())
	{
	}
	CConnectionOwner& Owner;
	asio::io_context& IoContext;
	asio::io_context::strand IoContextWriteStrand;
	asio::ip::tcp::socket Socket;
	std::string NetworkName;
	std::atomic<uint64_t> SequenceNumber{ 1 };

	// free lock queue for send
	TQueue<FMessageData, EQueueMode::MPSC> SendTo;
	std::atomic<bool> IsWriting{ false };
	// free lock queue for recv
	TQueue<FMessage, EQueueMode::MPSC>& RecvFrom;
	// buffer for read
	FMessageData MessageTemporaryRead;
	// connection state
	std::atomic<ESocketState> State{ ESocketState::kInit };
};

CConnection::CConnection(CConnectionOwner& Owner)
	: Impl(new FImpl(Owner))
{
	Impl->Owner.IncreaseConnectionCounter();
}

CConnection::~CConnection()
{
	std::promise<void> Promise;
	std::future<void> Future = Promise.get_future();
	asio::dispatch(Impl->IoContext, [&]() mutable {
		if (Impl->Socket.is_open())
		{
			std::error_code ErrorCode;
			Impl->Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
			if (ErrorCode) OnErrorCode(ErrorCode);
			Impl->Socket.close();
		}
		Promise.set_value();
		});
	Future.get();
	Impl->Owner.DecreaseConnectionCounter();
}

const char* CConnection::GetNetworkName() { return Impl->NetworkName.c_str(); }

ESocketState CConnection::GetSocketState() { return Impl->State; }

void CConnection::Disconnect()
{
	auto Self(this->shared_from_this());
	asio::dispatch(Impl->IoContext, [this, Self]() mutable {
		if (Impl->Socket.is_open())
		{
			std::error_code ErrorCode;
			Impl->Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ErrorCode);
			if (ErrorCode) OnErrorCode(ErrorCode);
			Impl->Socket.close();
		}
		});
}

uint64_t CConnection::Send(const FMessageData& MessageData)
{
	uint64_t SequenceNumber = Impl->SequenceNumber++;
	const_cast<FMessageData&>(MessageData).SetSequenceNumber(SequenceNumber);
	Impl->SendTo.Enqueue(MessageData);
	bool Expected = false;
	if (Impl->IsWriting.compare_exchange_strong(Expected, true))
		Impl->IoContextWriteStrand.dispatch([this, Self = shared_from_this()]{ WriteHeader(); });
	return SequenceNumber;
}

uint64_t CConnection::Send(FMessageData&& MessageData)
{
	uint64_t SequenceNumber = Impl->SequenceNumber++;
	const_cast<FMessageData&>(MessageData).SetSequenceNumber(SequenceNumber);
	Impl->SendTo.Enqueue(std::move(MessageData));
	bool Expected = false;
	if (Impl->IsWriting.compare_exchange_strong(Expected, true)) 
		Impl->IoContextWriteStrand.dispatch([this, Self = shared_from_this()] {WriteHeader();});
	return SequenceNumber;
}

void CConnection::OnErrorCode(const std::error_code& ErrorCode)
{
	assert(Impl->Owner.RunInIoContext());
	if (asio::error::eof == ErrorCode)
	{
		// remote is shutdown if eof 
	}
	GLogger->Log(ELogLevel::kWarning, "<{:s}> Socket Error : {:s}\n", Impl->NetworkName, ErrorCode.message());
	ESocketState ExpectedSocketState = ESocketState::kConnected;
	if (Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::kDisconnected))
		Impl->Owner.PushTask([Self = shared_from_this()]{ Self->Impl->Owner.OnConnectionDisconnected(Self); });
}

void CConnection::ReadHeader()
{
	auto Self(this->shared_from_this());
	asio::async_read(Impl->Socket, asio::buffer(Impl->MessageTemporaryRead.GetHeader(), Impl->MessageTemporaryRead.GetHeaderSize()),
		[this, Self](std::error_code ErrorCode, std::size_t length)
		{
			if (!ErrorCode)
			{
				if (Impl->MessageTemporaryRead.GetBodySize() > 0)
				{
					Impl->MessageTemporaryRead.UpdateBodySize();
					ReadBody();
				}
				else
				{
					Impl->RecvFrom.Enqueue(FMessage{ Self, std::move(Impl->MessageTemporaryRead) });
					ReadHeader();
				}
			}
			else
			{
				OnErrorCode(ErrorCode);
			}
		});
}

void CConnection::ReadBody()
{
	auto Self(this->shared_from_this());
	asio::async_read(Impl->Socket, asio::buffer(Impl->MessageTemporaryRead.GetBody<void>(), Impl->MessageTemporaryRead.GetBodySize()),
		[this, Self](std::error_code ErrorCode, std::size_t length)
		{
			if (!ErrorCode)
			{
				Impl->RecvFrom.Enqueue(FMessage{ Self, std::move(Impl->MessageTemporaryRead) });
				ReadHeader();
			}
			else
			{
				OnErrorCode(ErrorCode);
			}
		});
}

void CConnection::WriteHeader()
{
	auto Self(this->shared_from_this());
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.Peek()->GetHeader(), Impl->SendTo.Peek()->GetHeaderSize()),
		Impl->IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
					if (Impl->SendTo.Peek()->GetBodySize() > 0)
					{
						WriteBody();
					}
					else
					{
						Impl->SendTo.Pop();
						if (Impl->SendTo.Peek() != nullptr)
						{
							WriteHeader();
						}
						else
						{
							bool Expected = true;
							bool IsNeedWriteHeader = Impl->IsWriting.compare_exchange_strong(Expected, false);
							assert(IsNeedWriteHeader);
						}
					}
				}
				else
				{
					OnErrorCode(ErrorCode);
				}
			}));
}

void CConnection::WriteBody()
{
	auto Self(this->shared_from_this());
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.Peek()->GetBody<void>(), Impl->SendTo.Peek()->GetBodySize()),
		Impl->IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
					Impl->SendTo.Pop();
					if (Impl->SendTo.Peek() != nullptr)
					{
						WriteHeader();
					}
					else
					{
						bool Expected = true;
						bool IsNeedWriteHeader = Impl->IsWriting.compare_exchange_strong(Expected, false);
						assert(IsNeedWriteHeader);
					}
				}
				else
				{
					OnErrorCode(ErrorCode);
				}
			}));
}

void CConnection::ConnectToServer()
{
	ESocketState ExpectedSocketState = ESocketState::kConnecting;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnected));
	ConnectToRemote();
}

void CConnection::ConnectToClient()
{
	ESocketState ExpectedSocketState = ESocketState::kInit;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnected));
	ConnectToRemote();
}

void CConnection::ConnectToRemote()
{
	Impl->NetworkName = std::format("{:s}:{:d}", Impl->Socket.remote_endpoint().address().to_string(), Impl->Socket.remote_endpoint().port());
	Impl->Owner.PushTask([Self = shared_from_this()]{ Self->Impl->Owner.OnConnectionConnected(Self); });
	ReadHeader();
}

void CConnection::ConnectFailed()
{
	ESocketState ExpectedSocketState = ESocketState::kConnecting;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnectFailed));
}

bool CConnection::SetConnectingSocketState()
{
	ESocketState ExpectedSocketState = ESocketState::kInit;
	return Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::kConnecting);
}

void* CConnection::GetSocket(){ return &(Impl->Socket); }
