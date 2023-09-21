#include "pch.h"
#include "TaskRunner.h"

#include <Base/Exception.h>
#include <Base/Threading/ThreadTaskRunnerImpl.h>
#include <Base/Threading/SequencedTaskRunnerImpl.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::GetCurrent()
            {
                return ThreadTaskRunnerImpl::GetCurrent();
            }

            RefPtr<SequencedTaskRunner> __YYAPI SequencedTaskRunner::Create()
            {
                RefPtr<SequencedTaskRunner> _pSequencedTaskRunner;
                _pSequencedTaskRunner.Attach(new SequencedTaskRunnerImpl());
                return _pSequencedTaskRunner;
            }
        } // namespace Threading
    }
} // namespace YY
