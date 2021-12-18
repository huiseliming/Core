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

enum class ESocketState : uint32_t {
	kInit,
	kConnecting,
	kConnected,
	kDisconnected,
	kConnectFailed,
};

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

class CORE_API SConnection : public std::enable_shared_from_this<SConnection>
{
	friend class FConnectionOwner;
	friend class FClient;
	friend class FServer;
public:
	SConnection(asio::ip::tcp::socket Socket, FConnectionOwner& Owner, INetworkProtocol* NetworkProtocol = INetworkProtocol::DefaultProtocol);
	SConnection(const SConnection&) = delete;
	SConnection(SConnection&&) = delete;
	SConnection& operator=(const SConnection&) = delete;
	SConnection& operator=(SConnection&&) = delete;
	virtual ~SConnection();

	INetworkProtocol* GetNetworkProtocol();
	const char* GetNetworkName();
	ESocketState GetSocketState();

	void Disconnect();
	void Send(const std::vector<uint8_t>& MessageData);
	void Send(std::vector<uint8_t>&& MessageData);

protected:
	virtual void OnErrorCode(const std::error_code& ErrorCode);
	virtual void ReadHeader();
	virtual void ReadBody();
	virtual void WriteHeader();
	virtual void WriteBody();


private:
	void ConnectToServer();
	void ConnectToClient();
	void ConnectToRemote();
	void ConnectFailed();
	bool SetConnectingSocketState();
	void* GetSocket();

protected:
	FConnectionOwner& Owner;
	asio::io_context& IoContext;
	asio::io_context::strand IoContextWriteStrand;
	asio::ip::tcp::socket Socket;
	std::string NetworkName;

	INetworkProtocol* NetworkProtocol;

	std::deque<std::vector<uint8_t>> SendTo;
	// free lock queue for recv
	TQueue<FDataOwner, EQueueMode::MPSC>& RecvFrom;
	// buffer for read
	std::vector<uint8_t> DataTemporaryRead;
	// connection state
	std::atomic<ESocketState> State{ ESocketState::kInit };
};

#pragma warning(pop)