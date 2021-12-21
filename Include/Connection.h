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

enum class ESocketState : uint32_t {
	ESS_None,
	ESS_Connected,
	ESS_RequestDisconnect,
	ESS_Disconnected,
};

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

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

	static std::string MakeNetworkName(const asio::ip::tcp::socket& Socket);

protected:
	virtual void OnRecvData(std::shared_ptr<SConnection> ConnectionPtr, std::vector<uint8_t>& Data);

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