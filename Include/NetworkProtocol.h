#pragma once
#include <memory>
#include <vector>
#include <string>
#include <cassert>

class SConnection;

struct FDataOwner 
{
	FDataOwner() = default;
	//FDataOwner(std::shared_ptr<SConnection> InOwner, const std::vector<uint8_t>& InData)
	//	: Owner(InOwner)
	//	, Data(InData)
	//{}
	FDataOwner(std::shared_ptr<SConnection> InOwner, std::vector<uint8_t>&& InData)
		: Owner(InOwner)
		, Data(std::move(InData))
	{}
	FDataOwner(const FDataOwner&) = delete;
	FDataOwner(FDataOwner&& Other)
	{
		Owner = std::move(Other.Owner);
		Data = std::move(Other.Data);
	}
	FDataOwner& operator=(const FDataOwner&) = delete;
	FDataOwner& operator=(FDataOwner&& Other)
	{
		assert(std::addressof(Other) != this);
		Owner = std::move(Other.Owner);
		Data = std::move(Other.Data);
		return *this;
	}

	std::shared_ptr<SConnection> Owner;
	std::vector<uint8_t> Data;
};
