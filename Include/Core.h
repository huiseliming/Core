#pragma once

#include <string>
#include "Global.h"
#include "PlatformImplement.h"
#ifdef  _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#ifdef __REFLECTOR__ // reflector using clang, c++23 std::format not impl
#ifndef STD_FORMAT_EMPTY_DECL
namespace std {
	template <class... _Types>
	string format(const string_view _Fmt, const _Types&... _Args) {
		return {};
	}
}
#define STD_FORMAT_EMPTY_DECL
#endif
#else
#include <format>
#endif



void CoreInitialize();

void CoreUninitialize();

std::string TranslateHResult(HRESULT Hr);
#endif //  _WIN32

std::string FormatSystemTime(const time_t& tt);

std::string GetCurrentSystemTime();



























