#include "pch.h"
#include "TaskRunnerImpl.h"

#include <Base/Sync/Interlocked.h>
#include <Base/Sync/Sync.h>
#include <Base/ErrorCode.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    thread_local WeakPtr<TaskRunner> g_pTaskRunnerWeak;

    uint32_t __YYAPI GenerateNewTaskRunnerId()
    {
        static uint32_t s_TaskRunnerId = 0;
        return Sync::Increment(&s_TaskRunnerId);
    }
} // namespace YY::Base::Threading
