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


class IConnectionOwner;

enum class ESocketState : uint32_t {
	Init,
	Connecting,
	Connected,
	Disconnected,
	ConnectFailed,
};

// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)

template<typename ARet, typename... AArgs>
struct TDelegate
{
	bool IsBound() 
	{
		if (Delegate) return false;
		return true;
	}
	ARet Execute(AArgs&& ... Args)
	{
		return Delegate(std::forward<AArgs...>(Args));
	}
	std::function<ARet(AArgs...)> Delegate;
};

template<typename ARet, typename... AArgs>
struct TMulticastDelegate
{
	bool IsBound()
	{
		if (Delegates.empty()) return false;
		return true;
	}

	void Broadcast(AArgs&&... Args)
	{
		for (auto& Delegate : Delegates)
			Delegate(std::forward<AArgs...>(Args));
	}
	std::vector<std::function<ARet(AArgs...)>> Delegates;
};

struct FConnectionDelegate
{
	//TDelegate<bool(std::vector<uint8_t>&)> OnRecvDataPreProcessing;
	//TDelegate<bool(std::vector<uint8_t>&)> OnRecvDataProcessing;
	//TDelegate<void(std::vector<uint8_t>&)> OnRecvDataPostProcessing;

	//TDelegate<void(std::vector<uint8_t>&)> OnSendDataPreProcessing;
	//TDelegate<void(std::vector<uint8_t>&)> OnSendDataProcessing;
	//TDelegate<void(std::vector<uint8_t>&)> OnSendDataPostProcessing;

	//TDelegate<void(std::shared_ptr<SConnection>)> OnConnected;
	//TDelegate<void(std::shared_ptr<SConnection>)> OnDisconnected;
	//TDelegate<void(std::shared_ptr<SConnection>)> OnConnectFailed;
};

struct IIOTarget
{
	virtual void RecvData() = 0;
	virtual void SendData() = 0;
};

class CORE_API SConnection : public std::enable_shared_from_this<SConnection>
{
	friend class IConnectionOwner;
	friend class CClient;
	friend class CServer;
public:
	struct FImpl;
	SConnection(IConnectionOwner& Owner, std::shared_ptr<FConnectionDelegate> ConnectionDelegate);
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