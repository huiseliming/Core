#pragma once
#include <memory>
#include <vector>
#include <string>

class SConnection;

struct FDataOwner 
{
	std::shared_ptr<SConnection> Owner;
	std::vector<uint8_t> Data;
};

struct INetworkProtocol
{
	static INetworkProtocol* DefaultProtocol;
	virtual uint32_t GetHeaderSize() = 0;
	virtual uint32_t GetBodySize(uint8_t* DataPtr) = 0;
	virtual uint8_t* GetHeaderPtr(uint8_t* DataPtr) = 0;
	virtual uint8_t* GetBodyPtr(uint8_t* DataPtr) = 0;
	virtual void SpawnHeaderFromBody(uint8_t* DataPtr, uint8_t DataSize) = 0;
	virtual std::vector<uint8_t> CreateDataFromString(std::string Str) { return {}; };

	virtual uint32_t GetRecvHeaderSize() { return GetHeaderSize(); };
	virtual uint32_t GetRecvBodySize(uint8_t* DataPtr) { return GetBodySize(DataPtr); };
	virtual void* GetRecvHeaderPtr(uint8_t* DataPtr) { return GetHeaderPtr(DataPtr); };
	virtual void* GetRecvBodyPtr(uint8_t* DataPtr) { return GetBodyPtr(DataPtr); };

	virtual uint32_t GetSendHeaderSize() { return GetHeaderSize(); };
	virtual uint32_t GetSendBodySize(uint8_t* DataPtr) { return GetBodySize(DataPtr); };
	virtual void* GetSendHeaderPtr(uint8_t* DataPtr) { return GetHeaderPtr(DataPtr); };
	virtual void* GetSendBodyPtr(uint8_t* DataPtr) { return GetBodyPtr(DataPtr); };
};
