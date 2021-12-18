#include "Logger.h"

#include <cassert>
#include <iostream>
#include "Core.h"

CLogger* GLogger = GLogInitializer();

const char* ToString(ELogLevel LogLevel)
{
	LogLevel = ELogLevel(static_cast<uint32_t>(LogLevel) & static_cast<uint32_t>(ELogLevel::LevelBitMask));
	unsigned long Index = 0;
	assert(uint32_t(LogLevel) != 0);
	FPlatform::BitScanReverseImpl(&Index, uint32_t(LogLevel));
	switch (ELogLevel(1 << Index))
	{
	case ELogLevel::Trace:
		return "Trace";
	case ELogLevel::Debug:
		return "Debug";
	case ELogLevel::Info:
		return "Info";
	case ELogLevel::Warning:
		return "Warning";
	case ELogLevel::Error:
		return "Error";
	case ELogLevel::Fatal:
		return "Fatal";
	case ELogLevel::Display:
		return "Display";
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

void CLogger::Log(ELogLevel LogLevel, const std::string& Message)
{
	//assert(Running == true);
	LogMessageQueue.Enqueue(FLogMessage{
		std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
		std::this_thread::get_id(),
		LogLevel,
		Message.data()
		});
}

void CLogger::Log(ELogLevel LogLevel, std::string&& Message)
{
	//assert(Running == true);
	LogMessageQueue.Enqueue(FLogMessage{
		std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
		std::this_thread::get_id(),
		LogLevel,
		std::move(Message)
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
			fmt::format("[{:s}] [{:#010x}] [{:<7s}] {:s}\n",
				FormatSystemTime(LogMessage.Timestamp),
				*reinterpret_cast<_Thrd_id_t*>(&LogMessage.ThreadId),
				ToString(LogMessage.LogLevel),
				LogMessage.Message);
		std::cout << FormatLog;
		for (size_t i = 0; i < LogCallbacks.size(); i++)
		{
			(*LogCallbacks[i])(LogMessage);
		}
	}
	std::this_thread::yield();
}


CLogger* GLogInitializer()
{
	static CLogger Logger;
	return &Logger;
}
