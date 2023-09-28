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
            private:
                uint32_t uTaskRunnerId;
                uint32_t uThreadId;
                // |uWeakCount| bStopWeakup | bPushLock |
                // | 31  ~  2 |     1       |    0      |
                union
                {
                    uint32_t uWeakupCountAndPushLock;
                    struct
                    {
                        uint32_t bPushLock : 1;
                        uint32_t bStopWeakup : 1;
                        uint32_t uWeakupCount : 31;
                    };
                };
                enum : uint32_t
                {
                    LockedQueuePushBitIndex = 0,
                    StopWeakupBitIndex,
                    WeakupCountStartBitIndex,
                    WeakupOnceRaw = 1 << WeakupCountStartBitIndex,
                    UnlockQueuePushLockBitAndWeakupOnceRaw = WeakupOnceRaw - (1u << LockedQueuePushBitIndex),
                };
                InterlockedQueue<ThreadWorkEntry> oThreadWorkList;

            public:
                SequencedTaskRunnerImpl();

                SequencedTaskRunnerImpl(const SequencedTaskRunnerImpl&) = delete;

                SequencedTaskRunnerImpl& operator=(const SequencedTaskRunnerImpl&) = delete;

                /////////////////////////////////////////////////////
                // SequencedTaskRunner
                virtual uint32_t __YYAPI GetId() override;

                virtual TaskRunnerStyle __YYAPI GetStyle() override;

            private:
                HRESULT __YYAPI PushThreadWorkEntry(RefPtr<ThreadWorkEntry> _pWorkEntry) override;

                void __YYAPI CleanupWorkEntryQueue();

                void __YYAPI DoWorkList();
            };
        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
