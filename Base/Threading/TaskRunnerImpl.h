#pragma once
#include <Base/Threading/TaskRunner.h>
#include <Base/Memory/WeakPtr.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Threading
{
    extern thread_local WeakPtr<TaskRunner> g_pTaskRunnerWeak;

    uint32_t __YYAPI GenerateNewTaskRunnerId();
} // namespace YY::Base::Threading

#pragma pack(pop)
