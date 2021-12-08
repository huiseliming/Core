#include "NetworkProtocol.h"

struct FDefaultProtocol : public INetworkProtocol
{
	virtual uint32_t GetHeaderSize() override
	{
		return sizeof(uint32_t);
	}
	virtual uint32_t GetBodySize(uint8_t* DataPtr) override
	{
		return *(uint32_t*)DataPtr;
	}
	virtual uint8_t* GetHeaderPtr(uint8_t* DataPtr)override
	{
		return DataPtr;
	}
	virtual uint8_t* GetBodyPtr(uint8_t* DataPtr) override
	{
		return DataPtr + GetHeaderSize();
	}
	void SpawnHeaderFromBody(uint8_t* DataPtr, uint8_t DataSize) override
	{
		*(uint32_t*)DataPtr = DataSize - GetHeaderSize();
	}
	virtual std::vector<uint8_t> CreateDataFromString(std::string Str)
	{ 
		std::vector<uint8_t> Data;
		Data.resize(GetHeaderSize() + Str.size());
		*(uint32_t*)GetHeaderPtr(Data.data()) = static_cast<uint32_t>(Str.size());
		std::memcpy(Data.data() + GetHeaderSize(), Str.data(), Str.size());
		return Data;
	};
};
static FDefaultProtocol StaticDefaultProtocol;
INetworkProtocol* INetworkProtocol::DefaultProtocol = &StaticDefaultProtocol;

