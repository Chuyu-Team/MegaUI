#pragma once

#include <new>
#include <functional>

#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Threading/ProcessThreads.h>
#include <Base/Threading/ThreadPoolTimerManger.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class ThreadTaskRunnerImpl
                : public ThreadTaskRunner
                , public ThreadPoolTimerManger
            {
            private:
                InterlockedQueue<TaskEntry> oTaskQueue;

                // |uWeakupCount| bStopWakeup | bPushLock |
                // | 31   ~   2 |     1       |    0      |
                union
                {
                    volatile uint32_t uWakeupCountAndPushLock;
                    struct
                    {
                        uint32_t bPushLock : 1;
                        uint32_t bStopWakeup : 1;
                        uint32_t bBackgroundLoop : 1;
                        uint32_t uWakeupCount : 29;
                    };
                };
                enum : uint32_t
                {
                    LockedQueuePushBitIndex = 0,
                    StopWakeupBitIndex,
                    BackgroundLoopIndex,
                    WakeupCountStartBitIndex,
                    BackgroundLoopRaw = 1 << BackgroundLoopIndex,
                    WakeupOnceRaw = 1 << WakeupCountStartBitIndex,
                    UnlockQueuePushLockBitAndWakeupOnceRaw = WakeupOnceRaw - (1u << LockedQueuePushBitIndex),
                };

                volatile uint32_t uThreadId;

            public:
                ThreadTaskRunnerImpl(_In_ uint32_t _uThreadId = Threading::GetCurrentThreadId(), _In_ bool _bBackgroundLoop = false);

                /// <summary>
                /// 从线程池借用一个线程，执行TaskRunner。
                /// </summary>
                /// <param name="_uTaskRunnerId"></param>
                // ThreadTaskRunnerImpl(_In_ uint32_t _uTaskRunnerId);

                ThreadTaskRunnerImpl(const ThreadTaskRunnerImpl&) = delete;

                ~ThreadTaskRunnerImpl();

                ThreadTaskRunnerImpl& operator=(const ThreadTaskRunnerImpl&) = delete;

                /////////////////////////////////////////////////////
                // TaskRunner
                virtual TaskRunnerStyle __YYAPI GetStyle() const noexcept override;

                /////////////////////////////////////////////////////
                // ThreadTaskRunner

                virtual uint32_t __YYAPI GetThreadId() override;

                //
                ////////////////////////////////////////////////////

                uintptr_t __YYAPI RunUIMessageLoop();

                /// <summary>
                /// 运行后台循环，改模式无法支持UI相关工作。
                /// </summary>
                /// <returns></returns>
                void __YYAPI RunBackgroundLoop();

                void __YYAPI EnableWakeup(_In_ bool _bEnable);

                void __YYAPI operator()();

            private:
                HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) override;

                HRESULT __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pTask) override;

                void __YYAPI CleanupTaskQueue() noexcept;

                HRESULT __YYAPI Wakeup() noexcept;
            };
        }
    }
} // namespace YY

#pragma pack(pop)
