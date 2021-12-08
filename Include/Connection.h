#pragma once
#include <memory>
#include <future>
#include <cassert>
#include <system_error>
#include "Message.h"
#include "CoreApi.h"

// define macro SENT_TO_USE_TQUEUE not better performance because Impl->IoContextWriteStrand is lock 
// SENT_TO_USE_TQUEUE Minimal probability will not activate the write event because Impl->IsWriting.compare_exchange_strong
// #define SENT_TO_USE_TQUEUE


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
	friend class CClient;
	friend class CServer;
public:
	struct FImpl;
	SConnection(FConnectionOwner& Owner);
	SConnection(const SConnection&) = delete;
	SConnection(SConnection&&) = delete;
	SConnection& operator=(const SConnection&) = delete;
	SConnection& operator=(SConnection&&) = delete;
	virtual ~SConnection();

	const char* GetNetworkName();
	ESocketState GetSocketState();
	void Disconnect();
	uint64_t Send(const FMessageData& MessageData);
	uint64_t Send(FMessageData&& MessageData);

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
	std::unique_ptr<FImpl> Impl;
};

#pragma warning(pop)