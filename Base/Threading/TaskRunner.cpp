#include "pch.h"
#include <Base/Threading/TaskRunner.h>

#include <Base/Threading/ThreadTaskRunnerImpl.h>


#pragma warning(disable : 28251)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            ThreadTaskRunner ThreadTaskRunner::GetCurrentThreadTaskRunner()
            {
                auto _pThreadTaskRunner = ThreadTaskRunnerImpl::GetCurrentThreadTaskRunner();
                ThreadTaskRunner _Tmp(_pThreadTaskRunner);
                _pThreadTaskRunner->Release();
                return _Tmp;
            }
        }
    }
} // namespace YY
