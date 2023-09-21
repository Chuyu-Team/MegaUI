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
            uint32_t __YYAPI GenerateNewTaskRunnerId()
            {
                static uint32_t s_TaskRunnerId = 0;
                return Sync::Increment(&s_TaskRunnerId);
            }
            
            ThreadWorkEntry::ThreadWorkEntry(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData, bool _bSync/* = false*/)
                : fStyle(_bSync ? (ThreadWorkEntryStyle::SimpleCallback | ThreadWorkEntryStyle::Sync) : ThreadWorkEntryStyle::SimpleCallback)
                , hr(E_PENDING)
                , SimpleCallback {_pfnCallback, _pUserData}
            {
            }

            ThreadWorkEntry::ThreadWorkEntry(std::function<void()>&& _pfnLambdaCallback, bool _bSync/* = false*/)
                : fStyle(_bSync ? ThreadWorkEntryStyle::Sync : ThreadWorkEntryStyle::None)
                , hr(E_PENDING)
                , pfnLambdaCallback(std::move(_pfnLambdaCallback))
            {
            }
            
            ThreadWorkEntry::~ThreadWorkEntry()
            {
                if (!HasFlags(fStyle, ThreadWorkEntryStyle::SimpleCallback))
                {
                    pfnLambdaCallback.~function<void()>();
                }
            }

            void __YYAPI ThreadWorkEntry::DoWorkThenFree()
            {
                if (HasFlags(fStyle, ThreadWorkEntryStyle::SimpleCallback))
                {
                    SimpleCallback.pfnCallback(SimpleCallback.pUserData);
                }
                else
                {
                    pfnLambdaCallback();
                }

                hr = S_OK;
                if (HasFlags(fStyle, ThreadWorkEntryStyle::Sync))
                {
                    WakeByAddressSingle(&hr);
                }
                else
                {
                    delete this;
                }
            }
        } // namespace Threading
    }     // namespace Base
} // namespace YY
