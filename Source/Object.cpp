#include "Object.h"

std::unordered_map<std::thread::id, FThreadContext> GThreadContexts;

FThreadContext& GetCurrentThreadContext()
{
    return GThreadContexts[std::this_thread::get_id()];
}




