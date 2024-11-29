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
                LSTATUS lStatus = ERROR_IO_PENDING;

                IoTaskEntry()
                    : TaskEntry()
                    , OVERLAPPED{}
                {
                }

                bool __YYAPI OnComplete(LSTATUS _lStatus)
                {
                    return Sync::CompareExchange(&lStatus, _lStatus, ERROR_IO_PENDING) == ERROR_IO_PENDING;
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
#else
                ThreadHandle hThread = NullThreadHandle;
#endif
                // 该 Dispatch持有的任务引用计数，如果引用计数 <=0，那么可以归将线程归还给全局线程池。
                volatile int32_t nDispatchTaskRef = 0ul;

                enum : uint32_t
                {
                    PendingTaskQueuePushLockBitIndex = 0,
                    WakeupRefBitIndex,

                    WakeupRefOnceRaw = 1 << WakeupRefBitIndex,
                    UnlockQueuePushLockBitAndWakeupRefOnceRaw = WakeupRefOnceRaw - (1u << PendingTaskQueuePushLockBitIndex),
                };

                union
                {
                    volatile uint32_t fFlags = 0ul;
                    struct
                    {
                        uint32_t bPendingTaskQueuePushLock : 1;
                        // 唤醒的引用计数，这里偶尔因为时序，从 0 溢出也没事。
                        // 这里只是做一个记录。
                        uint32_t uWakeupRef : 31;
                    };
                };

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

                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pTask) noexcept;

                /// <summary>
                /// 发起异步请求成功后请调用此函数。内部将对完成端口进行监听。
                /// </summary>
                /// <returns></returns>
                void __YYAPI StartIo() noexcept;

            protected:
                void __YYAPI operator()();

            private:
                void __YYAPI ExecuteTaskRunner();

                void ProcessingPendingTaskQueue() noexcept;

                void __YYAPI DispatchTask(RefPtr<TaskEntry> _pDispatchTask);

                void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) override;

                void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) override;

                void __YYAPI Weakup(_In_ int32_t _nNewDispatchTaskRef, _In_ uint32_t _uNewFlags = UINT32_MAX);

                template<typename TaskType>
                void __YYAPI JoinPendingTaskQueue(InterlockedQueue<TaskType>& _oPendingTaskQueue, _In_ RefPtr<TaskType> _pTask) noexcept
                {
                    // 先增加任务计数，防止这段时间，服务线程不必要的意外归还线程池
                    const auto _nNewDispatchTaskRef = YY::Sync::Increment(&nDispatchTaskRef);
                    for (;;)
                    {
                        if (!Sync::BitSet(&fFlags, PendingTaskQueuePushLockBitIndex))
                        {
                            _oPendingTaskQueue.Push(_pTask.Detach());
                            const auto _uNewFlags = Sync::Add(&fFlags, UnlockQueuePushLockBitAndWakeupRefOnceRaw);
                            Weakup(_nNewDispatchTaskRef, _uNewFlags);
                            break;
                        }
                    }

                    return;
                }
            };
        }
    }
} // namespace YY

#pragma pack(pop)
