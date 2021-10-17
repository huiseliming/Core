#pragma once
#include <string>
#include "CoreApi.h"

struct CORE_API FPlatform
{
	static void* InterlockedCompareExchangePointerImpl(void* volatile* Dest, void* Exchange, void* Comparand);
	static void* InterlockedExchangePointerImpl(void** Dest, void* Exchange);
	static void MemoryBarrierImpl();

	static unsigned char BitScanForwardImpl(unsigned long* Index, unsigned long Mask);
	static unsigned char BitScanReverseImpl(unsigned long* Index, unsigned long Mask);
};
