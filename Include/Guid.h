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
	FGuid();
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
		return Other.Data == Data;
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