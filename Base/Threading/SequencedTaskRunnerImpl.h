#pragma once
#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/InterlockedQueue.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Threading
{
    class SequencedTaskRunnerImpl : public SequencedTaskRunner
    {
        friend YY::Base::Threading::ThreadPool;
    private:
        InterlockedQueue<TaskEntry> oTaskQueue;

        // |uWakeupCount| bStopWakeup | bPushLock |
        // | 31   ~   2 |     1       |    0      |
        union
        {
            volatile uint32_t uWakeupCountAndPushLock;
            struct
            {
                uint32_t bPushLock : 1;
                uint32_t bStopWakeup : 1;
                uint32_t uWakeupCount : 30;
            };
        };
        enum : uint32_t
        {
            LockedQueuePushBitIndex = 0,
            StopWakeupBitIndex,
            WakeupCountStartBitIndex,
            WakeupOnceRaw = 1 << WakeupCountStartBitIndex,
            LockQueuePushLockAndWakeupOnceRaw = WakeupOnceRaw + (1u << LockedQueuePushBitIndex),
        };

    public:
        SequencedTaskRunnerImpl();

        ~SequencedTaskRunnerImpl() override;

        SequencedTaskRunnerImpl(const SequencedTaskRunnerImpl&) = delete;

        SequencedTaskRunnerImpl& operator=(const SequencedTaskRunnerImpl&) = delete;

        /////////////////////////////////////////////////////
        // TaskRunner
        virtual TaskRunnerStyle __YYAPI GetStyle() const noexcept override;

        void __YYAPI operator()();

    private:
        HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) override;

        void __YYAPI CleanupTaskQueue() noexcept;

        void __YYAPI ExecuteTaskRunner();
    };
} // namespace YY

#pragma pack(pop)
