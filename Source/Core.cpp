#include "Core.h"
#include "Global.h"
#include "Reflect.h"
#include <ctime>
#include <chrono>

void CoreInitialize()
{
	GMetaTable->Initialize();
}

void CoreUninitialize()
{

}

std::string TranslateHResult(HRESULT Hr)
{
	char* pMsgBuf = nullptr;
	//获取格式化错误
	const DWORD nMsgLen = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, Hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
	);
	//长度小于0说明未获取到格式化的错误
	if (nMsgLen <= 0)
		return "Unidentified error code";
	// 拷贝字符串
	std::string errorMsg = pMsgBuf;
	// 这个字符串是Windows的系统内存，归还系统
	LocalFree(pMsgBuf);
	return errorMsg;
}

std::string FormatSystemTime(const time_t& tt)
{
#ifdef _WIN32
	tm NowTm = { 0 };
	tm* NowTmPtr = &NowTm;
	localtime_s(NowTmPtr, &tt);
#else
	tm* NowTmPtr = localtime(&tt);
#endif // DEBUG
	return fmt::format("{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
		NowTmPtr->tm_year + 1900, NowTmPtr->tm_mon + 1, NowTmPtr->tm_mday,
		NowTmPtr->tm_hour, NowTmPtr->tm_min, NowTmPtr->tm_sec);
}

std::string GetCurrentSystemTime()
{
	return FormatSystemTime(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

std::string DefaultCodePageToUTF8(const std::string& DefaultCodePage)
{
#ifdef _WIN32
	int Len = MultiByteToWideChar(CP_ACP, 0, DefaultCodePage.c_str(), -1, nullptr, 0);
	wchar_t* WString = new wchar_t[Len + 1];
	memset(WString, 0, Len + 1);
	MultiByteToWideChar(CP_ACP, 0, DefaultCodePage.c_str(), -1, WString, Len);

	Len = WideCharToMultiByte(CP_UTF8, 0, WString, -1, nullptr, 0, nullptr, nullptr);
	std::string Ret;
	Ret.resize(Len + 1);
	WideCharToMultiByte(CP_UTF8, 0, WString, -1, (LPSTR)Ret.c_str(), Len, nullptr, nullptr);
	delete[] WString;
	return Ret;
#else
	return DefaultCodePage;
#endif // DEBUG
}

