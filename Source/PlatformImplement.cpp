#include "PlatformImplement.h"
#include <intrin.h>
#include <ctime>
#include <chrono>
#include <format>


namespace FPlatformImplement
{
	void* InterlockedExchangePtr(void** Dest, void* Exchange) {
#ifdef _WIN32
		return ::_InterlockedExchangePointer(Dest, Exchange);
#elif __clang__
		return __sync_lock_test_and_set(Dest, Exchange);
#endif
	}

	void MemoryBarrier() {
#ifdef _WIN32
		_mm_sfence();
#elif __clang__
		__sync_synchronize();
#endif
	}
	unsigned char BitScanForward(unsigned long* Index, unsigned long Mask)
	{
#ifdef _WIN32
		return _BitScanForward(Index, Mask);
#elif __clang__
		return __builtin_clz(Index, Mask);
#endif
	}
	unsigned char BitScanReverse(unsigned long* Index, unsigned long Mask)
	{
#ifdef _WIN32
		return _BitScanReverse(Index, Mask);
#elif __clang__
		return __builtin_clz(Index, Mask);
#endif
	}
	std::string GetCurrentSystemTime(const time_t& tt)
	{
#ifdef _WIN32
		tm NowTm = { 0 };
		tm* NowTmPtr = &NowTm;
		localtime_s(NowTmPtr, &tt);
#else
		tm* NowTmPtr = localtime(&tt);
#endif // DEBUG
		return std::format("{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
			NowTmPtr->tm_year + 1900, NowTmPtr->tm_mon, NowTmPtr->tm_mday,
			NowTmPtr->tm_hour, NowTmPtr->tm_min, NowTmPtr->tm_sec);
	}
	std::string GetCurrentSystemTime()
	{
		return GetCurrentSystemTime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
	}
}

















