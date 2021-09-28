#pragma once

#include <string>
#include "Global.h"
#include "PlatformImplement.h"
#ifdef  _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>



void CoreInitialize();

void CoreUninitialize();

std::string TranslateHResult(HRESULT Hr);
#endif //  _WIN32

std::string FormatSystemTime(const time_t& tt);

std::string GetCurrentSystemTime();





























