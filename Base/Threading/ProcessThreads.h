#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
#ifdef _WIN32
            using ::GetCurrentThreadId;
#else
            uint32_t
            __YYAPI
            GetCurrentThreadId();
#endif
        }
    } // namespace Base
} // namespace YY
