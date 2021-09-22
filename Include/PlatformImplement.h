#pragma once
#include <string>

namespace FPlatformImplement
{
	void* InterlockedExchangePtr(void** Dest, void* Exchange);
	void MemoryBarrier();

	unsigned char BitScanForwardImpl(unsigned long* Index, unsigned long Mask);
	unsigned char BitScanReverseImpl(unsigned long* Index, unsigned long Mask);

}
