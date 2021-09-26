#pragma once
#include <memory>
#include <future>
#include <cassert>
#include <system_error>
#include "Message.h"

class CConnectionOwner;

enum class ESocketState : uint32_t {
	kInit,
	kConnecting,
	kConnected,
	kDisconnected,
	kConnectFailed,
};


class CConnection : public std::enable_shared_from_this<CConnection>
{
	friend class CConnectionOwner;
	friend class CClient;
	friend class CServer;
public:
	struct FImpl;
	CConnection(CConnectionOwner& Owner);

	virtual ~CConnection();

	CConnection(const CConnection&) = delete;
	CConnection(CConnection&&) = delete;
	CConnection& operator=(const CConnection&) = delete;
	CConnection& operator=(CConnection&&) = delete;

	void ConnectToRemote();

	void ConnectFailed();

	const char* GetNetworkName();

	ESocketState GetSocketState();

	void Disconnect();

	uint64_t Send(const FMessageData& MessageData);

	uint64_t Send(FMessageData&& MessageData);

protected:

	virtual void OnErrorCode(std::error_code& ErrorCode);

	virtual void ReadHeader();

	virtual void ReadBody();

	virtual void WriteHeader();

	virtual void WriteBody();


private:
	void* GetSocket();

protected:
	std::unique_ptr<FImpl> Impl;
};