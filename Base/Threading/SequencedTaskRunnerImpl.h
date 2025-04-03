#pragma once
#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/InterlockedQueue.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class SequencedTaskRunnerImpl : public SequencedTaskRunner
            {
                friend YY::Base::Threading::ThreadPool;
            private:
                InterlockedQueue<TaskEntry> oTaskQueue;

                union
                {
                    volatile uint32_t uWakeupCountAndPushLock;
                    struct
                    {
                        volatile uint32_t bPushLock : 1;
                        volatile uint32_t bStopWakeup : 1;
                        volatile uint32_t bInterrupt : 1;
                        volatile uint32_t uWakeupCount : 29;
                    };
                };
                enum : uint32_t
                {
                    LockedQueuePushBitIndex = 0,
                    StopWakeupBitIndex,
                    InterruptBitIndex,
                    WakeupCountStartBitIndex,
                    StopWakeupRaw = 1 << StopWakeupBitIndex,
                    InterruptRaw = 1 << InterruptBitIndex,
                    WakeupOnceRaw = 1 << WakeupCountStartBitIndex,
                    LockQueuePushLockAndWakeupOnceRaw = WakeupOnceRaw + (1u << LockedQueuePushBitIndex),
                    TerminateTaskRunnerRaw = StopWakeupRaw | InterruptRaw,
                };

                uString szThreadDescription;

            public:
                SequencedTaskRunnerImpl(uString _szThreadDescription = uString());

                ~SequencedTaskRunnerImpl() override;

                SequencedTaskRunnerImpl(const SequencedTaskRunnerImpl&) = delete;

                SequencedTaskRunnerImpl& operator=(const SequencedTaskRunnerImpl&) = delete;

                /////////////////////////////////////////////////////
                // TaskRunner
                virtual TaskRunnerStyle __YYAPI GetStyle() const noexcept override;

                HRESULT __YYAPI Join(TimeSpan<TimePrecise::Millisecond> _nWaitTimeOut) noexcept override;

                HRESULT __YYAPI Interrupt() noexcept override;

                void __YYAPI operator()();

            private:
                HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) override;

                void __YYAPI CleanupTaskQueue() noexcept;

                void __YYAPI ExecuteTaskRunner();
            };
        }
    }
} // namespace YY

#pragma pack(pop)
