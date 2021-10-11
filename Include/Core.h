#pragma once

#include <string>
#include "Global.h"
#include "PlatformImplement.h"
#ifdef  _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>





CORE_API void CoreInitialize();

CORE_API void CoreUninitialize();

std::string TranslateHResult(HRESULT Hr);
#endif //  _WIN32

CORE_API std::string FormatSystemTime(const time_t& tt);

CORE_API std::string GetCurrentSystemTime();



























