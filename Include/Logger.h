#pragma once
#include <thread>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include "Queue.h"

namespace ELogLevel
{
	enum Type : uint32_t
	{
		kTrace = 1 << 0,
		kDebug = 1 << 1,
		kInfo = 1 << 2,
		kWarning = 1 << 3,
		kError = 1 << 4,
		kFatal = 1 << 5,
		kLevelBitMask = kTrace | kDebug | kInfo | kWarning | kError | kFatal,
	};
}


struct FLogMessage
{
	time_t Timestamp;
	std::thread::id ThreadId;
	ELogLevel::Type LogLevel;
	std::string Message;
};

class CLogger
{
public:
	CLogger();

	~CLogger();

	void Log(ELogLevel::Type LogLevel, const std::string_view Message);

	void Log(ELogLevel::Type LogLevel, std::string&& Message);

	template<typename ... TArgs>
	void Log(ELogLevel::Type LogLevel, const std::string_view Message, TArgs&& ... Args)
	{
		Log(LogLevel, std::format(Message, std::forward<TArgs>(Args)...));
	}

	template<typename ... TArgs>
	void Log(ELogLevel::Type LogLevel, std::string&& Message, TArgs&& ... Args)
	{
		Log(LogLevel, std::format(std::forward<std::string>(Message), std::forward<TArgs>(Args)...));
	}

protected:
	void Run();

	void Loop();

	std::atomic<bool> Running;
	std::thread LogThread;

	std::ofstream LogFile;
	std::vector<std::function<void(const FLogMessage&)>> LogActions;

	TQueue<std::function<void()>> TaskQueue;
	TQueue<FLogMessage> LogMessageQueue;

};

const char* ToString(ELogLevel::Type LogLevel);