#pragma once
#include <deque>
#include <memory>
#include <future>
#include <cassert>
#include <system_error>
#include <asio.hpp>
#include "NetworkProtocol.h"
#include "CoreApi.h"
#include "Queue.h"

class FConnectionOwner;
struct FSingleThreadIoContext;

//namespace asio
//{
//	class any_io_executor;
//
//	template <typename Protocol, typename Executor = any_io_executor>
//	class basic_stream_socket;
//
//	template <typename Protocol, typename Executor = any_io_executor>
//	class basic_socket_acceptor;
//
//	class io_context;
//
//	namespace ip 
//	{
//		class tcp
//		{
//			typedef basic_stream_socket<tcp> socket;
//
//			typedef basic_socket_acceptor<tcp> acceptor;
//		};
//	}
//}

enum class ESocketState : uint32_t {
	ESS_None,
	ESS_Connected,
	ESS_RequestDisconnect,
	ESS_Disconnected,
};

enum class EChannelState : uint8_t
{
	ECS_None,
	ECS_Init,
	ECS_ReadWrite,
	ECS_RequestFinished,
	ECS_Finished,
	ECS_RequestTerminated,
	ECS_Terminated,
};

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

struct CORE_API FChannel : std::enable_shared_from_this<FChannel>
{
	virtual ~FChannel() = default;
	virtual void Terminated() { if (OnTerminated) OnTerminated(); }
	virtual void Finished() { if (OnFinished) OnFinished(); }
	virtual bool IsMatchedData(std::shared_ptr<SConnection> Connection, const std::vector<uint8_t>& Data) = 0;
	virtual void OnSendData(std::shared_ptr<SConnection> Connection) = 0;
	virtual void OnRecvData(std::shared_ptr<SConnection> Connection, const std::vector<uint8_t>& Data) = 0;

	std::function<void()> OnTerminated;
	std::function<void()> OnFinished;

	EChannelState ChannelState{ EChannelState::ECS_Init };
};

class CORE_API SConnection : public std::enable_shared_from_this<SConnection>
{
	friend class FConnectionOwner;
public:
	SConnection(asio::ip::tcp::socket InSocket, FConnectionOwner& InOwner, FSingleThreadIoContext& InSingleThreadIoContext);
	SConnection(const SConnection&) = delete;
	SConnection(SConnection&&) = delete;
	SConnection& operator=(const SConnection&) = delete;
	SConnection& operator=(SConnection&&) = delete;
	virtual ~SConnection();

	const char* GetNetworkName();
	ESocketState GetSocketState();

	void Disconnect();
	void Send(const std::vector<uint8_t>& MessageData);
	void Send(std::vector<uint8_t>&& MessageData);
	void AddChannel(std::shared_ptr<FChannel> Channel);

protected:
	bool ChannelFilter(std::vector<uint8_t>& Data);

	static std::string MakeNetworkName(const asio::ip::tcp::socket& Socket);

	std::list<std::shared_ptr<FChannel>> ChannelList;

protected:
	// Call In OwenerThread 
	virtual void OnRecvData(std::vector<uint8_t>& Data);
	virtual void OnConnected();
	virtual void OnDisconnected();

protected:
	virtual void OnErrorCode(const std::error_code& ErrorCode);
	virtual void ReadHeader();
	virtual void ReadBody();
	virtual void WriteHeader();
	virtual void WriteBody();

private:
	void ConnectToRemote();

protected:

	// Construction Sequence
	// 1.Socket
	// 2.Owner
	// 3.SingleThreadIoContext
	asio::ip::tcp::socket Socket;
	FConnectionOwner& Owner;
	FSingleThreadIoContext& SingleThreadIoContext;
	// 	fmt::format("{:s}:{:d}", Socket.remote_endpoint().address().to_string(), Socket.remote_endpoint().port());
	std::string NetworkName;

	std::deque<std::vector<uint8_t>> SendTo;
	// free lock queue for recv
	TQueue<FDataOwner, EQueueMode::MPSC>& RecvFrom;
	// buffer for read
	std::vector<uint8_t> DataTemporaryRead;
	// connection state
	ESocketState SocketState{ ESocketState::ESS_None };
};

#pragma warning(pop)