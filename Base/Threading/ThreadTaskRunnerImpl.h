#pragma once

#include <new>
#include <functional>

#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Sync/InterlockedQueue.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Threading
{
    class ThreadTaskRunnerImpl : public ThreadTaskRunner
    {
    private:
        InterlockedQueue<TaskEntry> oTaskQueue;

        // |uWeakCount| bStopWeakup | bPushLock |
        // | 31  ~  2 |     1       |    0      |
        union
        {
            uint32_t uWeakupCountAndPushLock;
            struct
            {
                uint32_t bPushLock : 1;
                uint32_t bStopWeakup : 1;
                uint32_t uWeakupCount : 30;
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
        
        uint32_t uThreadId;

    public:
        ThreadTaskRunnerImpl();

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
                
        uintptr_t __YYAPI RunUIMessageLoop(_In_opt_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);
                
        void __YYAPI EnableWeakup(_In_ bool _bEnable);

    private:
        HRESULT __YYAPI PostTaskInternal(RefPtr<TaskEntry> _pTask) override;

        void __YYAPI CleanupTaskQueue() noexcept;
    };
} // namespace YY

#pragma pack(pop)
