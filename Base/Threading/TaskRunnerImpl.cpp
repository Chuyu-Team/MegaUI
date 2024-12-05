#include "TaskRunnerImpl.h"

#include <Base/Sync/Interlocked.h>
#include <Base/Sync/Sync.h>
#include <Base/ErrorCode.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            thread_local WeakPtr<TaskRunner> g_pTaskRunnerWeak;

            uint32_t __YYAPI GenerateNewTaskRunnerId()
            {
                static uint32_t s_TaskRunnerId = 0;
                return Sync::Increment(&s_TaskRunnerId);
            }

            uint32_t __YYAPI GetWaitTimeSpan(TickCount<TimePrecise::Millisecond> _uWakeupTickCount) noexcept
            {
                if (_uWakeupTickCount == TickCount<TimePrecise::Millisecond>::GetMax())
                    return UINT32_MAX;

                auto _nTimeSpan = _uWakeupTickCount - TickCount<TimePrecise::Millisecond>::GetCurrent();
                if (_nTimeSpan.GetInternalValue() <= 0)
                {
                    return 0ul;
                }
                else if (_nTimeSpan.GetMilliseconds() >= UINT32_MAX)
                {
                    return UINT32_MAX;
                }

                return (uint32_t)_nTimeSpan.GetMilliseconds();
            }
        }
    }
} // namespace YY::Base::Threading
