#pragma once
#include <cassert>
#include <memory>

struct FGuid
{
	FGuid();
	FGuid(const FGuid& Other) {
		Data1 = Other.Data1;
		Data2 = Other.Data2;
		Data3 = Other.Data3;
		Data4 = Other.Data4;
	}
	FGuid(FGuid&& Other) {
		Data1 = Other.Data1;
		Data2 = Other.Data2;
		Data3 = Other.Data3;
		Data4 = Other.Data4;
		Other.Data1 = 0;
		Other.Data2 = 0;
		Other.Data3 = 0;
		Other.Data4 = 0;
	}
	FGuid& operator=(const FGuid& Other) {
		assert(std::addressof(Other) != this);
		Data1 = Other.Data1;
		Data2 = Other.Data2;
		Data3 = Other.Data3;
		Data4 = Other.Data4;
	}
	FGuid operator=(FGuid&& Other) {
		assert(std::addressof(Other) != this);
		Data1 = Other.Data1;
		Data2 = Other.Data2;
		Data3 = Other.Data3;
		Data4 = Other.Data4;
		Other.Data1 = 0;
		Other.Data2 = 0;
		Other.Data3 = 0;
		Other.Data4 = 0;
	}
	bool operator==(const FGuid& Other)
	{
		return
			Other.Data1 == this->Data1 &&
			Other.Data2 == this->Data2 &&
			Other.Data3 == this->Data3 &&
			Other.Data4 == this->Data4;
	}
	bool operator!=(const FGuid& Other)
	{
		return *this == Other;
	}
	uint32_t Data1{ 0 };
	uint16_t Data2{ 0 };
	uint16_t Data3{ 0 };
	uint64_t Data4{ 0 };
};