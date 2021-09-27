#include "Logger.h"

#include <cassert>
#include <iostream>
#include "Core.h"

std::unique_ptr<CLogger> GLogger;

const char* ToString(ELogLevel::Type LogLevel)
{
	LogLevel = ELogLevel::Type(static_cast<uint32_t>(LogLevel) & static_cast<uint32_t>(ELogLevel::kLevelBitMask));
	unsigned long Index = 0;
	assert(LogLevel != 0);
	FPlatformImplement::BitScanReverseImpl(&Index, LogLevel);
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
	Running = false;
	LogThread.join();
}

void CLogger::Log(ELogLevel::Type LogLevel, const std::string& Message)
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
		.Message = std::move(Message)
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
	Running = true;
	while (Running) {
		Loop();
	}
	Loop();
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
		if (*LogMessage.Message.rbegin() == '\n') // remove last \n or \r\n
		{
			if (*(LogMessage.Message.rbegin()++) == '\r')
				LogMessage.Message.resize(LogMessage.Message.size() - 2);
			else
				LogMessage.Message.resize(LogMessage.Message.size() - 1);
		}
		std::string FormatLog =
			std::format("[{:s}] [{:#010x}] [{:<7s}] {:s}\n",
				GetCurrentSystemTime(LogMessage.Timestamp),
				*reinterpret_cast<_Thrd_id_t*>(&LogMessage.ThreadId),
				ToString(LogMessage.LogLevel),
				LogMessage.Message);
		std::cout << FormatLog;
		for (size_t i = 0; i < LogActions.size(); i++)
		{
			LogActions[i](LogMessage);
		}
	}
	std::this_thread::yield();
}
