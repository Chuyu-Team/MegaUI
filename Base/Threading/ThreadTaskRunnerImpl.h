#pragma once

#include <new>
#include <functional>

#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Sync/InterlockedQueue.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class ThreadTaskRunnerImpl : public ThreadTaskRunner
            {
            private:
                uint32_t uRef;
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
                ThreadTaskRunnerImpl();

                ThreadTaskRunnerImpl(const ThreadTaskRunnerImpl&) = delete;

                ~ThreadTaskRunnerImpl();

                ThreadTaskRunnerImpl& operator=(const ThreadTaskRunnerImpl&) = delete;

                /////////////////////////////////////////////////////
                // SequencedTaskRunner

                virtual uint32_t __YYAPI AddRef() override;

                virtual uint32_t __YYAPI Release() override;

                virtual uint32_t __YYAPI GetId() override;

                virtual TaskRunnerStyle __YYAPI GetStyle() override;
                
                /////////////////////////////////////////////////////
                // ThreadTaskRunner

                virtual uint32_t __YYAPI GetThreadId() override;
                
                uintptr_t __YYAPI RunUIMessageLoop(_In_opt_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);
                
                void __YYAPI EnableWeakup(_In_ bool _bEnable);

            private:
                HRESULT __YYAPI PushThreadWorkEntry(ThreadWorkEntry* _pWorkEntry) override;

                void __YYAPI CleanupWorkEntryQueue();
            };
        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
