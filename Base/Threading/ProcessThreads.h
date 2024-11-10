#pragma once
#include <Base/YY.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#endif

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
#ifdef _WIN32
            using ThreadId = DWORD;
            using ThreadHandle = HANDLE;
            constexpr ThreadHandle NullThreadHandle = NULL;

            inline ThreadId __YYAPI GetCurrentThreadId()
            {
                return ::GetCurrentThreadId();
            }
#else
            using ThreadId = pid_t;
            using ThreadHandle = pthread_t;
            constexpr ThreadHandle NullThreadHandle = 0;

            inline ThreadId __YYAPI GetCurrentThreadId()
            {
                return static_cast<ThreadId>(syscall(SYS_gettid));
            }
#endif
        }
    }
} // namespace YY::Base::Threading

namespace YY
{
    using namespace YY::Base::Threading;
}
