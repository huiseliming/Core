#pragma once
#include <string>

namespace FPlatformImplement
{
	void* InterlockedExchangePtr(void** Dest, void* Exchange);
	void MemoryBarrier();

	unsigned char BitScanForward(unsigned long* Index, unsigned long Mask);
	unsigned char BitScanReverse(unsigned long* Index, unsigned long Mask);

	std::string GetCurrentSystemTime(const time_t& tt);

	std::string GetCurrentSystemTime();

}
