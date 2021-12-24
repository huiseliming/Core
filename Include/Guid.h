#pragma once
#include <cassert>
#include <memory>
#include <string>
#include "CoreExport.h"

#ifdef _WIN32
#include <combaseapi.h>
#else
#include <uuid/uuid.h>
#endif

struct CORE_EXPORT FGuid
{
protected:
	FGuid();
public:
	FGuid(uint8_t InBytes[16])
	{
		memcpy(&Data, &InBytes[0], 16);
	}
	FGuid(const FGuid& Other) {
		Data = Other.Data;
	}
	FGuid(FGuid&& Other) {
		Data = Other.Data;
		Other.Data = {};
	}
	FGuid& operator=(const FGuid& Other) {
		Data = Other.Data;
		return *this;
	}
	FGuid operator=(FGuid&& Other) {
		assert(std::addressof(Other) != this);
		Data = Other.Data;
		Other.Data = {};
		return *this;
	}
	bool operator==(const FGuid& Other)
	{
		if (Other.Data.Data1 != Data.Data2) return false;
		if (Other.Data.Data2 != Data.Data2) return false;
		if (Other.Data.Data3 != Data.Data3) return false;
		for (size_t i = 0; i < 8; i++)
			if (Other.Data.Data4[i] == Data.Data4[i]) 
				return false;
		return true;
	}
	bool operator!=(const FGuid& Other)
	{
		return !(*this == Other);
	}

	static FGuid GenerateGuid();

	std::string ToString();

#ifdef _WIN32
	GUID Data;
#else

#endif // DEBUG
};