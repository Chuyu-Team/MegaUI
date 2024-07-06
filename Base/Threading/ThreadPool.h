#pragma once

#if defined(_WIN32)
#include <Base/Threading/ThreadPoolForWindows.h>
#else
#include <Base/Threading/ThreadPoolForLinux.h>
#endif
