#pragma once

#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/WeakPtr.h>
#include <Base/Time/TickCount.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Containers/BitMap.h>
#include <Base/Threading/ProcessThreads.h>
#include <Base/Threading/ThreadPoolTimerManger.h>
#include <Base/Threading/ThreadPoolWaitManger.h>

#pragma pack(push, __YY_PACKING)

/*
TaskRunnerDispatch 仅处理调度任务（比如定时器、异步IO），无法参执行其他Task。

*/

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class TaskRunnerDispatch
            {
            public:
                static _Ret_notnull_ TaskRunnerDispatch* __YYAPI Get() noexcept;

#if defined(_WIN32)
                virtual bool __YYAPI BindIO(_In_ HANDLE _hHandle) const noexcept = 0;
#endif

                virtual void __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pTimer) noexcept = 0;

                virtual HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pWait) noexcept = 0;

                /// <summary>
                /// 发起异步请求成功后请调用此函数。内部将对完成端口进行监听。
                /// </summary>
                /// <returns></returns>
                virtual void __YYAPI StartIo() noexcept = 0;

            protected:
                static void __YYAPI DispatchTask(RefPtr<TaskEntry> _pDispatchTask) noexcept
                {
                    if (!_pDispatchTask)
                        return;

                    do
                    {
                        if (_pDispatchTask->IsCanceled())
                            break;

                        // 不属于任何TaskRunner，因此在线程池随机唤醒
                        if (_pDispatchTask->pOwnerTaskRunnerWeak == nullptr)
                        {
                            auto _hr = ThreadPool::PostTaskInternal(_pDispatchTask.Get());
                            return;
                        }

                        // 任务所属的 TaskRunner 已经释放？
                        auto _pResumeTaskRunner = _pDispatchTask->pOwnerTaskRunnerWeak.Get();
                        if (_pResumeTaskRunner == nullptr)
                            break;

                        _pResumeTaskRunner->PostTaskInternal(std::move(_pDispatchTask));
                        return;
                    } while (false);

                    _pDispatchTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                }
            };
        }
    }
} // namespace YY

#pragma pack(pop)
