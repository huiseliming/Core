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

namespace std{
	template<>
	struct hash<FLogMessage> {
		using result_type = size_t;
		using argument_type = FLogMessage;
		uint64_t operator()(const FLogMessage& InLogMessage) const{
			return std::hash<std::thread::id>()(InLogMessage.ThreadId)^
				   std::size_t                 (InLogMessage.LogLevel)^
				   std::hash<std::string>()    (InLogMessage.Message );
		}
	};

	template<>
	struct equal_to<FLogMessage> {
		typedef FLogMessage first_argument_type;
		typedef FLogMessage second_argument_type;
		typedef bool result_type ;

		uint64_t operator()(const FLogMessage& RightLogMessage, const FLogMessage& LeftLogMessage) const {
			return RightLogMessage.ThreadId == LeftLogMessage.ThreadId &&
				   RightLogMessage.LogLevel == LeftLogMessage.LogLevel &&
				   RightLogMessage.Message  == LeftLogMessage.Message   ;
		}
	};
}

class CLogger
{
public:
	CLogger();

	~CLogger();

	void Log(ELogLevel::Type LogLevel, const std::string& Message);

	void Log(ELogLevel::Type LogLevel, std::string&& Message);

	template<typename ... TArgs>
	void Log(ELogLevel::Type LogLevel, const std::string_view Message, TArgs&& ... Args)
	{
		Log(LogLevel, std::format(Message, std::forward<TArgs>(Args)...));
	}

	//void Log(ELogLevel::Type LogLevel, const std::string_view Message);
	//template<typename ... TArgs>
	//void Log(ELogLevel::Type LogLevel, std::string&& Message, TArgs&& ... Args)
	//{
	//	Log(LogLevel, std::format(std::forward<std::string>(Message), std::forward<TArgs>(Args)...));
	//}

protected:
	void Run();

	void Loop();

	std::atomic<bool> Running;
	std::thread LogThread;

	std::ofstream LogFile;
	std::vector<std::function<void(const FLogMessage&)>> LogActions;

	TQueue<std::function<void()>, EQueueMode::MPSC> TaskQueue;
	TQueue<FLogMessage, EQueueMode::MPSC> LogMessageQueue;

};

const char* ToString(ELogLevel::Type LogLevel);