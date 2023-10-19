#include "pch.h"
#include "SequencedTaskRunnerImpl.h"

#include <MegaUI/Base/ErrorCode.h>
#include <Base/Sync/Sync.h>
#include <Base/Threading/ProcessThreads.h>
#include <Base/Memory/WeakPtr.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    SequencedTaskRunnerImpl::SequencedTaskRunnerImpl()
        : uWakeupCountAndPushLock(0u)
    {
    }

    SequencedTaskRunnerImpl::~SequencedTaskRunnerImpl()
    {
        CleanupTaskQueue();
    }

    TaskRunnerStyle __YYAPI SequencedTaskRunnerImpl::GetStyle() const noexcept
    {
        return TaskRunnerStyle::Sequenced;
    }

    HRESULT __YYAPI SequencedTaskRunnerImpl::PostTaskInternal(RefPtr<TaskEntry> _pTask)
    {
        _pTask->hr = E_PENDING;

        if (bStopWakeup)
        {
            return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
        }

        for (;;)
        {
            if (!Sync::BitSet(&uWakeupCountAndPushLock, LockedQueuePushBitIndex))
            {
                oTaskQueue.Push(_pTask.Detach());
                break;
            }
        }

        // 我们 解除锁定，uPushLock = 0 并且将 uWakeupCount += 1
        // 因为刚才 uWakeupCountAndPushLock 已经将第一个标记位设置位 1
        // 所以我们再 uWakeupCountAndPushLock += 1即可。
        // uWakeupCount + 1 <==> uWakeupCountAndPushLock + 2 <==> (uWakeupCountAndPushLock | 1) + 1
        if (Sync::Add(&uWakeupCountAndPushLock, uint32_t(UnlockQueuePushLockBitAndWakeupOnceRaw)) < WakeupOnceRaw * 2u)
        {
            auto _hr = ThreadPool::PostTaskInternal(this);
            if (FAILED(_hr))
            {
                // 阻止后续再唤醒线程
                Sync::BitSet(&uWakeupCountAndPushLock, StopWakeupBitIndex);
                CleanupTaskQueue();
                return _hr;
            }
        }

        return S_OK;
    }

    void __YYAPI SequencedTaskRunnerImpl::operator()()
    {
        ExecuteTaskRunner();
    }

    void __YYAPI SequencedTaskRunnerImpl::ExecuteTaskRunner()
    {
        g_pTaskRunnerWeak = this;

        for (;;)
        {
            // 理论上 ExecuteTaskRunner 执行时引用计数 = 2，因为执行器拥有一次引用计数
            // 如果为 1 (IsShared() == false)，那么说明用户已经释放了这个 TaskRunner
            // 这时我们需要及时的退出，随后会将队列里的任务全部取消释放内存。
            if (!IsShared())
                break;

            auto _pTask = oTaskQueue.Pop();
            if (!_pTask)
                break;
            _pTask->operator()();
            _pTask->Release();
            if (Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw) < WakeupOnceRaw)
                break;
        }
        g_pTaskRunnerWeak = nullptr;
        return;
    }
            
    void __YYAPI SequencedTaskRunnerImpl::CleanupTaskQueue() noexcept
    {
        for (;;)
        {
            auto _pTask = RefPtr<TaskEntry>::FromPtr(oTaskQueue.Pop());
            if (!_pTask)
                break;

            _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));

            if (Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw) < WakeupOnceRaw)
                break;
        }
        return;
    }
} // namespace YY
