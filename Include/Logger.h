#pragma once
#include <thread>
#include <fstream>
#include <functional>
#include <future>
#include <cassert>
#include "Queue.h"
#include "CoreApi.h"
#include "Core.h"
#include "fmt/format.h"


enum class ELogLevel : uint32_t
{
	Trace = 1 << 0,
	Debug = 1 << 1,
	Info = 1 << 2,
	Warning = 1 << 3,
	Error = 1 << 4,
	Fatal = 1 << 5,
	Display = 1 << 6,
	LevelBitMask = Trace | Debug | Info | Warning | Error | Fatal | Display,
};

struct FLogMessage
{
	FLogMessage() = default;
	FLogMessage(time_t Timestamp, std::thread::id ThreadId, ELogLevel LogLevel, const std::string& Message)
		: Timestamp(Timestamp)
		, ThreadId(ThreadId)
		, LogLevel(LogLevel)
		, Message(Message)
	{
	}
	FLogMessage(time_t Timestamp, std::thread::id ThreadId, ELogLevel LogLevel, std::string&& Message)
		: Timestamp(Timestamp)
		, ThreadId(ThreadId)
		, LogLevel(LogLevel)
		, Message(std::move(Message))
	{
	}

	FLogMessage(const FLogMessage&) = default;
	FLogMessage(FLogMessage&& Other)
	{
		Timestamp = Other.Timestamp;
		ThreadId = Other.ThreadId;
		LogLevel = Other.LogLevel;
		Other.Timestamp = { 0 };
		Other.ThreadId = std::thread::id();
		Other.LogLevel = ELogLevel::Trace;
		Message = std::move(Other.Message);
	}

	FLogMessage& operator=(const FLogMessage&) = default;
	FLogMessage& operator=(FLogMessage&& Other)
	{
		assert(std::addressof(Other) != this);
		Timestamp = Other.Timestamp;
		ThreadId = Other.ThreadId;
		LogLevel = Other.LogLevel;
		Other.Timestamp = { 0 };
		Other.ThreadId = std::thread::id();
		Other.LogLevel = ELogLevel::Trace;
		Message = std::move(Other.Message);
		return *this;
	}

	time_t Timestamp;
	std::thread::id ThreadId;
	ELogLevel LogLevel;
	std::string Message;
};

//namespace std{
//	template<>
//	struct hash<FLogMessage> {
//		using result_type = size_t;
//		using argument_type = FLogMessage;
//		uint64_t operator()(const FLogMessage& InLogMessage) const{
//			return std::hash<std::thread::id>()(InLogMessage.ThreadId)^
//				   std::size_t                 (InLogMessage.LogLevel)^
//				   std::hash<std::string>()    (InLogMessage.Message );
//		}
//	};
//
//	template<>
//	struct equal_to<FLogMessage> {
//		typedef FLogMessage first_argument_type;
//		typedef FLogMessage second_argument_type;
//		typedef bool result_type ;
//
//		uint64_t operator()(const FLogMessage& RightLogMessage, const FLogMessage& LeftLogMessage) const {
//			return RightLogMessage.ThreadId == LeftLogMessage.ThreadId &&
//				   RightLogMessage.LogLevel == LeftLogMessage.LogLevel &&
//				   RightLogMessage.Message  == LeftLogMessage.Message   ;
//		}
//	};
//}
// disable warning 4251
#pragma warning(push)
#pragma warning (disable: 4251)
class CORE_API CLogger
{
public:
	CLogger();

	~CLogger();

	void Log(ELogLevel LogLevel, const std::string& Message);

	void Log(ELogLevel LogLevel, std::string&& Message);

	template<typename ... TArgs>
	void Log(ELogLevel LogLevel, const std::string_view Message, TArgs&& ... Args)
	{
		Log(LogLevel, fmt::format(Message, std::forward<TArgs>(Args)...));
	}

	std::future<void> AddLogCallback(std::shared_ptr<std::function<void(const FLogMessage&)>> LogCallback)
	{
		std::shared_ptr<std::promise<void>> Promise = std::make_shared<std::promise<void>>();
		std::future<void> Future = Promise->get_future();
		TaskQueue.Enqueue([&, LogCallback, Promise] {
			LogCallbacks.push_back(LogCallback);
			Promise->set_value();
		});
		return Future;
	}

	std::future<bool> RemoveLogCallback(std::shared_ptr<std::function<void(const FLogMessage&)>> LogCallback)
	{
		std::shared_ptr<std::promise<bool>> Promise = std::make_shared<std::promise<bool>>();
		std::future<bool> Future = Promise->get_future();
		TaskQueue.Enqueue([&, LogCallback, Promise]()  {
			for (auto it = LogCallbacks.begin(); it != LogCallbacks.end(); it++)
			{
				if (*it == LogCallback) {
					LogCallbacks.erase(it);
					Promise->set_value(true);
					return;
				}
			}
			Promise->set_value(false);
		});
		return Future;
	}

	//void Log(ELogLevel LogLevel, const std::string_view Message);
	//template<typename ... TArgs>
	//void Log(ELogLevel LogLevel, std::string&& Message, TArgs&& ... Args)
	//{
	//	Log(LogLevel, fmt::format(std::forward<std::string>(Message), std::forward<TArgs>(Args)...));
	//}

protected:
	void Run();

	void Loop();

	std::atomic<bool> Running;
	std::thread LogThread;

	std::vector<std::shared_ptr<std::function<void(const FLogMessage&)>>> LogCallbacks;

	TQueue<std::function<void()>, EQueueMode::MPSC> TaskQueue;
	TQueue<FLogMessage, EQueueMode::MPSC> LogMessageQueue;

};

// disable warning 4251
#pragma warning(pop)

CORE_API const char* ToString(ELogLevel LogLevel);

extern CORE_API CLogger* GLogger;

CLogger* GLogInitializer();

#define ELL_Trace   ELogLevel::Trace
#define ELL_Debug   ELogLevel::Debug
#define ELL_Info    ELogLevel::Info
#define ELL_Warning ELogLevel::Warning
#define ELL_Error   ELogLevel::Error
#define ELL_Fatal   ELogLevel::Fatal
#define ELL_Display ELogLevel::Display

#define GLog(ELL, X, ...) GLogger->Log(ELL, X, ##__VA_ARGS__)

struct CLogScopeTimeConsume
{


};