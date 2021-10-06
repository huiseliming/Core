#pragma once
#include <thread>
#include <fstream>
#include <functional>
#include <future>
#include <cassert>
#include "Queue.h"
#include "CoreApi.h"
#include "Core.h"

//namespace ELogLevel
//{
	enum class ELogLevel : uint32_t
	{
		kTrace = 1 << 0,
		kDebug = 1 << 1,
		kInfo = 1 << 2,
		kWarning = 1 << 3,
		kError = 1 << 4,
		kFatal = 1 << 5,
		kDisplay = 1 << 6,
		kLevelBitMask = kTrace | kDebug | kInfo | kWarning | kError | kFatal | kDisplay,
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
		Other.LogLevel = ELogLevel::kTrace;
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
		Other.LogLevel = ELogLevel::kTrace;
		Message = std::move(Other.Message);
		return *this;
	}

	time_t Timestamp;
	std::thread::id ThreadId;
	ELogLevel LogLevel;
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

	void Log(ELogLevel LogLevel, const std::string& Message);

	void Log(ELogLevel LogLevel, std::string&& Message);

	template<typename ... TArgs>
	void Log(ELogLevel LogLevel, const std::string_view Message, TArgs&& ... Args)
	{
		Log(LogLevel, std::format(Message, std::forward<TArgs>(Args)...));
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
	//	Log(LogLevel, std::format(std::forward<std::string>(Message), std::forward<TArgs>(Args)...));
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

const char* ToString(ELogLevel LogLevel);

extern CORE_API std::unique_ptr<CLogger> GLogger;

#define LOG(X, ...) GLogger->Log(ELogLevel::kDisplay, X, ##__VA_ARGS__)