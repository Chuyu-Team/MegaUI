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
#if defined(_WIN32)
            struct IoTaskEntry
                : public TaskEntry
                , public OVERLAPPED

            {
                IoTaskEntry()
                    : TaskEntry()
                    , OVERLAPPED{}
                {
                }
            };
#endif

            class TaskRunnerDispatchImplByIoCompletionImpl
                : public ThreadPoolTimerManger
                , public ThreadPoolWaitMangerForMultiThreading
            {
                friend ThreadPool;
            private:
#if defined(_WIN32)
                // HANDLE hIoCompletionPort;
#else
                ThreadHandle hThread = NullThreadHandle;
#endif
                enum : uint32_t
                {
                    SetTimerLockIndex = 1,
                    SetWaitLockIndex,
                };
                uint32_t fFlags = 0ul;
                
                InterlockedQueue<Timer> oPendingTimerTaskQueue;
                InterlockedQueue<Wait> oPendingWaitTaskQueue;

                TaskRunnerDispatchImplByIoCompletionImpl();

            public:
                TaskRunnerDispatchImplByIoCompletionImpl(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

                TaskRunnerDispatchImplByIoCompletionImpl& operator=(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

                ~TaskRunnerDispatchImplByIoCompletionImpl();

                static _Ret_notnull_ TaskRunnerDispatchImplByIoCompletionImpl* __YYAPI Get() noexcept;

#if defined(_WIN32)
                bool __YYAPI BindIO(_In_ HANDLE _hHandle) const noexcept;
#endif

                void __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pDispatchTask) noexcept;

                void __YYAPI Weakup();

                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pTask) noexcept;

            protected:
                void __YYAPI operator()();

            private:
                void __YYAPI ExecuteTaskRunner();

                void ProcessingPendingTaskQueue() noexcept;

                void __YYAPI DispatchTask(RefPtr<TaskEntry> _pDispatchTask);

                void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) override;

                void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) override;
            };
        }
    }
} // namespace YY

#pragma pack(pop)
