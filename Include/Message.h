#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cassert>

class CConnection;

struct FMessageData
{
	struct FHeader
	{
	private:
		FHeader()
		{
			assert(!"DONT NEW STRUCT FMessageData::FHeader");
		}
	public:
		uint32_t BodySize{ 0 };
		uint32_t DataDesc{ 0 };
		uint64_t SequenceNumber{ 0 };
		uint8_t Reserve[16];
	};
public:

	FMessageData(const std::vector<uint8_t>& Body = {})
		:Data(sizeof(FHeader), 0)
	{
		SetBody(Body);
	};

	FMessageData(const std::string& Text)
		:Data(sizeof(FHeader), 0)
	{
		SetBody(Text);
	};

	FMessageData(const FMessageData&) = default;
	FMessageData(FMessageData&& Other)
	{
		this->Data = std::move(Other.Data);
		Other.SetBody(std::vector<uint8_t>());
	}

	FMessageData& operator=(const FMessageData&) = default;
	FMessageData& operator=(FMessageData&& Other)
	{
		assert(std::addressof(Other) != this);
		this->Data = std::move(Other.Data);
		Other.SetBody(std::vector<uint8_t>());
		return *this;
	}

	uint32_t GetBodySize() { return reinterpret_cast<FHeader*>(Data.data())->BodySize; }
	void SetBodySize(uint32_t NewBodySize)
	{
		Data.resize(sizeof(FHeader) + NewBodySize);
		reinterpret_cast<FHeader*>(Data.data())->BodySize = NewBodySize;
	}

	uint32_t GetDataDesc() { return reinterpret_cast<FHeader*>(Data.data())->DataDesc; }
	void SetDataDesc(uint32_t NewDataDesc)
	{
		reinterpret_cast<FHeader*>(Data.data())->DataDesc = NewDataDesc;
	}

	uint64_t GetSequenceNumber() { return reinterpret_cast<FHeader*>(Data.data())->SequenceNumber; }
	void SetSequenceNumber(uint64_t NewSequenceNumber)
	{
		reinterpret_cast<FHeader*>(Data.data())->SequenceNumber = NewSequenceNumber;
	}

	constexpr uint32_t GetHeaderSize() { return sizeof(FHeader); }

	FHeader* GetHeader() { return reinterpret_cast<FHeader*>(Data.data()); }

	uint32_t GetBodyBufferSize() { return static_cast<uint32_t>(Data.size() - sizeof(FHeader)); }

	template<typename T>
	T* GetBody() { return reinterpret_cast<T*>(Data.data() + sizeof(FHeader)); }

	void SetBody(const std::vector<uint8_t>& NewBody) {
		Data.resize(sizeof(FHeader) + NewBody.size());
		memcpy(GetBody<void>(), NewBody.data(), NewBody.size());
		SetBodySize(static_cast<int32_t>(NewBody.size()));
		return;
	}

	void SetBody(const std::string& NewBody) {
		Data.resize(sizeof(FHeader) + NewBody.size());
		memcpy(GetBody<void>(), NewBody.data(), NewBody.size());
		SetBodySize(static_cast<int32_t>(NewBody.size()));
		return;
	}

	template<typename T>
	void SetBody(T& NewBody) {
		Data.resize(sizeof(FHeader) + sizeof(T));
		memcpy(GetBody<void>(), &NewBody, sizeof(T));
		SetBodySize(sizeof(T));
	}

	std::vector<uint8_t> FullData() { return Data; }
	void UpdateBodySize()
	{
		Data.resize(sizeof(FHeader) + GetBodySize());
	}

	int32_t GetSize() { return (int32_t)Data.size(); }

private:
	void ReserveSize(uint32_t NewReserveSize)
	{
		Data.reserve(NewReserveSize);
	}

	std::vector<uint8_t> Data;
};

struct FMessage {
	std::shared_ptr<CConnection> MessageOwner;
	FMessageData MessageData;
};