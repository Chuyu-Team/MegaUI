#pragma once

#include <new>
#include <functional>

#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Threading/ProcessThreads.h>
#include <Base/Threading/ThreadPoolTimerManger.h>
#include <Base/Threading/ThreadPoolWaitManger.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class ThreadTaskRunnerBaseImpl
                : public ThreadTaskRunner
            {
            public:
                virtual uintptr_t __YYAPI RunTaskRunnerLoop() = 0;
            };

            class ThreadTaskRunnerImpl
                : public ThreadTaskRunnerBaseImpl
                , public ThreadPoolTimerManger
                , public ThreadPoolWaitMangerForSingleThreading
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
                        volatile uint32_t bPushLock : 1;
                        volatile uint32_t bStopWakeup : 1;
                        volatile uint32_t bInterrupt : 1;
                        volatile uint32_t bBackgroundLoop : 1;
                        uint32_t uWakeupCount : 28;
                    };
                };
                enum : uint32_t
                {
                    LockedQueuePushBitIndex = 0,
                    StopWakeupBitIndex,
                    InterruptBitIndex,
                    BackgroundLoopIndex,
                    WakeupCountStartBitIndex,
                    StopWakeupRaw = 1 << StopWakeupBitIndex,
                    InterruptRaw = 1 << InterruptBitIndex,
                    BackgroundLoopRaw = 1 << BackgroundLoopIndex,
                    WakeupOnceRaw = 1 << WakeupCountStartBitIndex,
                    UnlockQueuePushLockBitAndWakeupOnceRaw = WakeupOnceRaw - (1u << LockedQueuePushBitIndex),
                    TerminateTaskRunnerRaw = StopWakeupRaw | InterruptRaw,
                };

                volatile uint32_t uThreadId;
                uString szThreadDescription;

            public:
                ThreadTaskRunnerImpl(_In_ uint32_t _uThreadId = Threading::GetCurrentThreadId(), _In_ bool _bBackgroundLoop = false, uString _szThreadDescription = uString());

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

                HRESULT __YYAPI Join(TimeSpan<TimePrecise::Millisecond> _nWaitTimeOut) noexcept override;

                HRESULT __YYAPI Interrupt() noexcept override;

                /////////////////////////////////////////////////////
                // ThreadTaskRunner

                virtual uint32_t __YYAPI GetThreadId() override;

                //
                ////////////////////////////////////////////////////

                void __YYAPI EnableWakeup(_In_ bool _bEnable);

                void __YYAPI operator()();
                
                uintptr_t __YYAPI RunTaskRunnerLoop() override;

            private:
                HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) override;

                HRESULT __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pTask) override;

                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pTask) override;

                void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) override;

                void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) override;

                void __YYAPI CleanupTaskQueue() noexcept;

                HRESULT __YYAPI Wakeup() noexcept;
                
                uintptr_t __YYAPI RunUIMessageLoop();
                
                /// <summary>
                /// 运行后台循环，改模式UI相关处理极为滞后！
                /// </summary>
                /// <returns></returns>
                uintptr_t __YYAPI RunBackgroundLoop();
            };
        }
    }
} // namespace YY

#pragma pack(pop)
