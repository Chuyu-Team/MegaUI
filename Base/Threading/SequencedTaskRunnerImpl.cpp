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
        : uTaskRunnerId(GenerateNewTaskRunnerId())
        , uThreadId(0u)
        , uWeakupCountAndPushLock(0u)
    {
    }

    SequencedTaskRunnerImpl::~SequencedTaskRunnerImpl()
    {
        CleanupTaskQueue();
    }

    uint32_t __YYAPI SequencedTaskRunnerImpl::GetId()
    {
        return uTaskRunnerId;
    }

    TaskRunnerStyle __YYAPI SequencedTaskRunnerImpl::GetStyle()
    {
        return TaskRunnerStyle::Sequenced;
    }

#ifdef _WIN32
    HRESULT __YYAPI SequencedTaskRunnerImpl::PostTaskInternal(RefPtr<TaskEntry> _pTask)
    {
        _pTask->hr = E_PENDING;

        if (bStopWeakup)
        {
            _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
            return E_FAIL;
        }

        for (;;)
        {
            if (!Sync::BitSet(&uWeakupCountAndPushLock, LockedQueuePushBitIndex))
            {
                oTaskQueue.Push(_pTask.Detach());
                break;
            }
        }

        // ���� ���������uPushLock = 0 ���ҽ� uWeakCount += 1
        // ��Ϊ�ղ� uWeakupCountAndPushLock �Ѿ�����һ�����λ����λ 1
        // ���������� uWeakupCountAndPushLock += 1���ɡ�
        // uWeakCount + 1 <==> uWeakupCountAndPushLock + 2 <==> (uWeakupCountAndPushLock | 1) + 1
        if (Sync::Add(&uWeakupCountAndPushLock, UnlockQueuePushLockBitAndWeakupOnceRaw) < WeakupOnceRaw * 2u)
        {
            // ��������һ�����ü���������TrySubmitThreadpoolCallback�ص������ڲ��Ѿ����ͷ���
            AddRef();
            auto _bRet = TrySubmitThreadpoolCallback(
                [](_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context) -> void
                {
                    // ע�� ������������ AddRef������������ RefPtr�ӹܵİ�ȫ�ġ�
                    auto _pSequencedTaskRunner = RefPtr<SequencedTaskRunnerImpl>::FromPtr(reinterpret_cast<SequencedTaskRunnerImpl*>(Context));
                    _pSequencedTaskRunner->ExecuteTaskRunner();
                },
                this,
                nullptr);

            if (!_bRet)
            {
                // ��ֹ�����ٻ����߳�
                Sync::BitSet(&uWeakupCountAndPushLock, StopWeakupBitIndex);
                auto _hr = YY::Base::HRESULT_From_LSTATUS(GetLastError());
                CleanupTaskQueue();

                // �ղŵ���ǰ TrySubmitThreadpoolCallback������������һ�����ü���
                // ���� TrySubmitThreadpoolCallback ʧ�ܣ��������¹黹���ü���
                Release();
                return _hr;
            }
        }

        return S_OK;
    }
#endif

    void __YYAPI SequencedTaskRunnerImpl::ExecuteTaskRunner()
    {
        uThreadId = GetCurrentThreadId();
        g_pTaskRunnerWeak = this;

        for (;;)
        {
            // ������ ExecuteTaskRunner ִ��ʱ���ü��� = 2����Ϊִ����ӵ��һ�����ü���
            // ���Ϊ 1 (IsShared() == false)����ô˵���û��Ѿ��ͷ������ TaskRunner
            // ��ʱ������Ҫ��ʱ���˳������Ὣ�����������ȫ��ȡ���ͷ��ڴ档
            if (!IsShared())
                break;

            auto _pTask = RefPtr<TaskEntry>::FromPtr(oTaskQueue.Pop());
            if (!_pTask)
                break;
                    
            _pTask->operator()();
            _pTask->Wakeup(S_OK);

            if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                break;
        }
        g_pTaskRunnerWeak = nullptr;
        uThreadId = 0;
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

            if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                break;
        }
        return;
    }
} // namespace YY
