#pragma once
#include <memory>
#include "Logger.h"
#include "CoreExport.h"

#define CORE_API CORE_EXPORT

extern CORE_API std::unique_ptr<CLogger> GLogger;
