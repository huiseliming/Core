#pragma once
#include <memory>
#include "Guid.h"
#include "Logger.h"
#include "CoreExport.h"

#define CORE_API CORE_EXPORT

extern CORE_API std::unique_ptr<CLogger> GLogger;


#define GLOG_DEBUG(X, ...)   GLogger->Log(ELogLevel::kDebug,   X, ##__VA_ARGS__)
#define GLOGD(X, ...) GLOG_DEBUG(X, ##__VA_ARGS__)

#define GLOG_INFO(X, ...)    GLogger->Log(ELogLevel::kInfo,    X, ##__VA_ARGS__)
#define GLOGI(X, ...) GLOG_INFO(X, ##__VA_ARGS__)

#define GLOG_WARNING(X, ...) GLogger->Log(ELogLevel::kWarning, X, ##__VA_ARGS__)
#define GLOGW(X, ...) GLOG_WARNING(X, ##__VA_ARGS__)

#define GLOG_ERROR(X, ...)   GLogger->Log(ELogLevel::kError,   X, ##__VA_ARGS__)
#define GLOGE(X, ...) GLOG_ERROR(X, ##__VA_ARGS__)

#define GLOG_FATAL(X, ...)   GLogger->Log(ELogLevel::kFatal,   X, ##__VA_ARGS__)
#define GLOGF(X, ...) GLOG_FATAL(X, ##__VA_ARGS__)






