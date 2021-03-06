#include "Connection.h"
#include <asio.hpp>
#include "Global.h"
#include "ConnectionOwner.h"
#include <deque>

struct SConnection::FImpl {
	FImpl(IConnectionOwner& Owner, std::shared_ptr<FConnectionDelegate> ConnectionDelegate)
		: Owner(Owner)
		, IoContext(Owner.GetIoContext())
		, IoContextWriteStrand(IoContext)
		, Socket(IoContext)
		, RecvFrom(Owner.GetRecvQueue())
	{
	}
	IConnectionOwner& Owner;
	asio::io_context& IoContext;
	asio::io_context::strand IoContextWriteStrand;
	asio::ip::tcp::socket Socket;
	std::string NetworkName;
	std::atomic<uint64_t> SequenceNumber{ 1 };

#ifdef SENT_TO_USE_TQUEUE
	// free lock queue for send
	TQueue<FMessageData, EQueueMode::MPSC> SendTo;
#else
	std::deque<FMessageData> SendTo;
#endif // SENT_TO_USE_TQUEUE
	std::atomic<bool> IsWriting{ false };
	// free lock queue for recv
	TQueue<FMessage, EQueueMode::MPSC>& RecvFrom;
	// buffer for read
	FMessageData MessageTemporaryRead;
	// connection state
	std::atomic<ESocketState> State{ ESocketState::Init };


	std::shared_ptr<FConnectionDelegate> ConnectionDelegate;
};

SConnection::SConnection(IConnectionOwner& Owner, std::shared_ptr<FConnectionDelegate> ConnectionDelegate)
	: Impl(new FImpl(Owner, ConnectionDelegate))
{
	Impl->Owner.IncreaseConnectionCounter();
}

SConnection::~SConnection()
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

const char* SConnection::GetNetworkName() { return Impl->NetworkName.c_str(); }

ESocketState SConnection::GetSocketState() { return Impl->State; }

void SConnection::Disconnect()
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

uint64_t SConnection::Send(const FMessageData& MessageData)
{
	uint64_t SequenceNumber = Impl->SequenceNumber++;
	const_cast<FMessageData&>(MessageData).SetSequenceNumber(SequenceNumber);
#ifdef SENT_TO_USE_TQUEUE
	Impl->SendTo.Enqueue(MessageData);
	bool Expected = false;
	if (Impl->IsWriting.compare_exchange_strong(Expected, true))
		Impl->IoContextWriteStrand.post([this, Self = shared_from_this()]{ WriteHeader(); });
#else
	Impl->IoContextWriteStrand.post(
		[this, Self = shared_from_this(), MessageData = MessageData] {
			Impl->SendTo.emplace_back(std::move(MessageData));
			if (Impl->SendTo.size() <= 1)
			{
				WriteHeader();
			}
		});
#endif // SENT_TO_USE_TQUEUE
	return SequenceNumber;
}

uint64_t SConnection::Send(FMessageData&& MessageData)
{
	uint64_t SequenceNumber = Impl->SequenceNumber++;
	const_cast<FMessageData&>(MessageData).SetSequenceNumber(SequenceNumber);
#ifdef SENT_TO_USE_TQUEUE
	Impl->SendTo.Enqueue(std::move(MessageData));
	bool Expected = false;
	if (Impl->IsWriting.compare_exchange_strong(Expected, true)) 
		Impl->IoContextWriteStrand.post([this, Self = shared_from_this()] {WriteHeader();});
#else
	Impl->IoContextWriteStrand.post(
		[this, Self = shared_from_this(), MessageData = std::move(MessageData)]{
			Impl->SendTo.emplace_back(std::move(MessageData));
			if (Impl->SendTo.size() <= 1)
			{
				WriteHeader();
			}
		});
#endif // SENT_TO_USE_TQUEUE
	return SequenceNumber;
}

void SConnection::OnErrorCode(const std::error_code& ErrorCode)
{
	assert(Impl->Owner.RunInIoContext());
	if (asio::error::eof == ErrorCode)
	{
		// remote is shutdown if eof 
	}
	GLogger->Log(ELogLevel::kWarning, "<{:s}> Socket Error : {:s}\n", Impl->NetworkName, ErrorCode.message());
	ESocketState ExpectedSocketState = ESocketState::Connected;
	if (Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::Disconnected))
		Impl->Owner.PushTask([Self = shared_from_this()]{ Self->Impl->Owner.OnConnectionDisconnected(Self); });
}

void SConnection::ReadHeader()
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

void SConnection::ReadBody()
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

void SConnection::WriteHeader()
{
	auto Self(this->shared_from_this());
#ifdef SENT_TO_USE_TQUEUE
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.Peek()->GetHeader(), Impl->SendTo.Peek()->GetHeaderSize()),
#else
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.front().GetHeader(), Impl->SendTo.front().GetHeaderSize()),
#endif // SENT_TO_USE_TQUEUE
		Impl->IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
#ifdef SENT_TO_USE_TQUEUE
					if (Impl->SendTo.Peek()->GetBodySize() > 0)
#else
					if (Impl->SendTo.front().GetBodySize() > 0)
#endif // SENT_TO_USE_TQUEUE
					{
						WriteBody();
					}
					else
					{
#ifdef SENT_TO_USE_TQUEUE
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
#else
						Impl->SendTo.pop_front();
						if (!Impl->SendTo.empty())
						{
							WriteHeader();
						}
#endif // SENT_TO_USE_TQUEUE
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

#ifdef SENT_TO_USE_TQUEUE
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.Peek()->GetBody<void>(), Impl->SendTo.Peek()->GetBodySize()),
#else
	asio::async_write(Impl->Socket, asio::buffer(Impl->SendTo.front().GetBody<void>(), Impl->SendTo.front().GetBodySize()),
#endif // SENT_TO_USE_TQUEUE
		Impl->IoContextWriteStrand.wrap([this, Self](std::error_code ErrorCode, std::size_t Length)
			{
				if (!ErrorCode)
				{
#ifdef SENT_TO_USE_TQUEUE
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
#else
					Impl->SendTo.pop_front();
					if (!Impl->SendTo.empty())
					{
						WriteHeader();
					}
#endif // SENT_TO_USE_TQUEUE
				}
				else
				{
					OnErrorCode(ErrorCode);
				}
			}));
}

void SConnection::ConnectToServer()
{
	ESocketState ExpectedSocketState = ESocketState::Connecting;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::Connected));
	ConnectToRemote();
}

void SConnection::ConnectToClient()
{
	ESocketState ExpectedSocketState = ESocketState::Init;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::Connected));
	ConnectToRemote();
}

void SConnection::ConnectToRemote()
{
	Impl->NetworkName = std::format("{:s}:{:d}", Impl->Socket.remote_endpoint().address().to_string(), Impl->Socket.remote_endpoint().port());
	Impl->Owner.PushTask([Self = shared_from_this()]{ Self->Impl->Owner.OnConnectionConnected(Self); });
	ReadHeader();
}

void SConnection::ConnectFailed()
{
	ESocketState ExpectedSocketState = ESocketState::Connecting;
	assert(Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::ConnectFailed));
}

bool SConnection::SetConnectingSocketState()
{
	ESocketState ExpectedSocketState = ESocketState::Init;
	return Impl->State.compare_exchange_strong(ExpectedSocketState, ESocketState::Connecting);
}

void* SConnection::GetSocket(){ return &(Impl->Socket); }
