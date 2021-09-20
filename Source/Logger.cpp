#include "Logger.h"

#include <cassert>
#include <iostream>

#include "PlatformImplement.h"

const char* ToString(ELogLevel::Type LogLevel)
{
	LogLevel = ELogLevel::Type(static_cast<uint32_t>(LogLevel) & static_cast<uint32_t>(ELogLevel::kLevelBitMask));
	unsigned long Index = 0;
	assert(LogLevel != 0);
	FPlatformImplement::BitScanReverse(&Index, LogLevel);
	switch (1 << Index)
	{
	case ELogLevel::kDebug:
		return "Debug";
	case ELogLevel::kInfo:
		return "Info";
	case ELogLevel::kWarning:
		return "Warning";
	case ELogLevel::kError:
		return "Error";
	case ELogLevel::kFatal:
		return "Fatal";
	default:
		return "None";
	}
	return "Level";
}

CLogger::CLogger()
	: Running(false)
	, LogThread(&CLogger::Run, this)
{
	
}

CLogger::~CLogger()
{
	//Running = false;
	LogThread.join();
}

void CLogger::Log(ELogLevel::Type LogLevel, const std::string_view Message)
{
	//assert(Running == true);
	LogMessageQueue.Enqueue(FLogMessage{
		.Timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
		.ThreadId = std::this_thread::get_id(),
		.LogLevel = LogLevel,
		.Message = Message.data()
		});
}

void CLogger::Log(ELogLevel::Type LogLevel, std::string&& Message)
{
	//assert(Running == true);
	LogMessageQueue.Enqueue(FLogMessage{
		.Timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
		.ThreadId = std::this_thread::get_id(),
		.LogLevel = LogLevel,
		.Message = std::forward<std::string>(Message)
		});
}

void CLogger::Run()
{
	time_t Now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef _WIN32
	tm NowTm = {0};
	tm* NowTmPtr = &NowTm;
	localtime_s(NowTmPtr, &Now);
#else
	tm* NowTmPtr = localtime(&Now);
#endif // DEBUG
	std::string SystemTime =
		std::format("{:d}{:02d}{:02d}_{:02d}{:02d}{:02d}",
			NowTmPtr->tm_year + 1900, NowTmPtr->tm_mon, NowTmPtr->tm_mday,
			NowTmPtr->tm_hour, NowTmPtr->tm_min, NowTmPtr->tm_sec);
	LogFile.open(std::format("{:s}.log", SystemTime), std::ios::app);
	if (!LogFile.is_open()) {
		std::cout << std::format("[{:s}.log] FILE CANT OPEN!", SystemTime) << std::endl;
		return;
	}
	Running = true;
	while (Running) {
		Loop();
	}
	Loop();
	LogFile.close();
}

void CLogger::Loop() {
	std::function<void()> Task;
	FLogMessage LogMessage;
	while (TaskQueue.Dequeue(Task))
	{
		Task();
	}
	while (LogMessageQueue.Dequeue(LogMessage))
	{
		std::string FormatLog =
			std::format("[{:s}] [{:#010x}] [{:<7s}] {:s}\n",
				FPlatformImplement::GetCurrentSystemTime(LogMessage.Timestamp),
				*reinterpret_cast<_Thrd_id_t*>(&LogMessage.ThreadId),
				ToString(LogMessage.LogLevel),
				LogMessage.Message);
		LogFile.write(FormatLog.data(), FormatLog.size());
		std::cout << FormatLog;
		for (size_t i = 0; i < LogActions.size(); i++)
		{
			LogActions[i](LogMessage);
		}
	}
	std::this_thread::yield();
}
