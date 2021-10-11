#pragma once
#include <string>
#include "CoreApi.h"

struct CORE_API FPlatformImplement
{
	static void* InterlockedExchangePtr(void** Dest, void* Exchange);
	static void MemoryBarrier();

	static unsigned char BitScanForwardImpl(unsigned long* Index, unsigned long Mask);
	static unsigned char BitScanReverseImpl(unsigned long* Index, unsigned long Mask);

};
