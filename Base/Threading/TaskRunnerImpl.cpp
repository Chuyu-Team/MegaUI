#include "pch.h"
#include "TaskRunnerImpl.h"

#include <Base/Sync/Interlocked.h>
#include <Base/Sync/Sync.h>
#include <MegaUI/Base/ErrorCode.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            thread_local SequencedTaskRunner* g_pTaskRunner = nullptr;

            uint32_t __YYAPI GenerateNewTaskRunnerId()
            {
                static uint32_t s_TaskRunnerId = 0;
                return Sync::Increment(&s_TaskRunnerId);
            }
        } // namespace Threading
    }     // namespace Base
} // namespace YY
