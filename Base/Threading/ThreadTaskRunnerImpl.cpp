#include "pch.h"
#include "ThreadTaskRunnerImpl.h"

#include <MegaUI/Base/ErrorCode.h>
#include <Base/Sync/Sync.h>
#include <Base/Threading/ProcessThreads.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    // RunUIMessageLoop ����Ĵ�����
    // ���� ThreadTaskRunner ��˵���뱣֤��Ϣѭ���ڲ�������������
    static thread_local uint32_t s_uUIMessageLoopEnterCount = 0u;

    ThreadTaskRunnerImpl::ThreadTaskRunnerImpl()
        : uWeakupCountAndPushLock(WeakupOnceRaw)
        , uThreadId(GetCurrentThreadId())
    {
    }

    /*ThreadTaskRunnerImpl::ThreadTaskRunnerImpl(uint32_t _uTaskRunnerId)
        : uTaskRunnerId(_uTaskRunnerId)
        , uThreadId(0)
        , uWeakupCountAndPushLock(0)
    {

    }*/

    ThreadTaskRunnerImpl::~ThreadTaskRunnerImpl()
    {
        CleanupTaskQueue();
    }
            
    TaskRunnerStyle __YYAPI ThreadTaskRunnerImpl::GetStyle() const noexcept
    {
        return TaskRunnerStyle::FixedThread;
    }
            
    uint32_t __YYAPI ThreadTaskRunnerImpl::GetThreadId()
    {
        return uThreadId;
    }
            
#ifdef _WIN32
    uintptr_t __YYAPI ThreadTaskRunnerImpl::RunUIMessageLoop(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
    {
        ++s_uUIMessageLoopEnterCount;
        if (s_uUIMessageLoopEnterCount == 1)
        {
            g_pTaskRunnerWeak = this;
            EnableWeakup(true);
        }

        if (_pfnCallback)
            _pfnCallback(_pUserData);


        MSG _oMsg;
        for (;;)
        {
            if (!IsShared())
            {
                _oMsg.wParam = HRESULT_From_LSTATUS(ERROR_CANCELLED);
                break;
            }

            const auto _bRet = PeekMessageW(&_oMsg, NULL, 0, 0, PM_REMOVE);
            if (_bRet == -1)
                continue;

            if (_bRet)
            {
                if (_oMsg.message == WM_QUIT)
                {
                    break;
                }
                TranslateMessage(&_oMsg);
                DispatchMessage(&_oMsg);
            }
            else
            {
                if (auto _pTask = RefPtr<TaskEntry>::FromPtr(oTaskQueue.Pop()))
                {
                    _pTask->operator()();
                    _pTask->Wakeup(S_OK);
                }

                // uPushLock ռ��1bit������ uWeakCount += 1 �ȼ��� uWeakCountAndPushLock += 2
                if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                {
                    // uWeakCount �Ѿ����㣬����˯��״̬
                    WaitMessage();
                }

                // ��Ϣѭ��������Ϊ���ڼ���״̬������
                Sync::Add(&uWeakupCountAndPushLock, WeakupOnceRaw);
            }
        }

        --s_uUIMessageLoopEnterCount;
        if (s_uUIMessageLoopEnterCount == 0)
        {
            // �����߳���˵������weak_ptr����
            // �߳��˳����߽����̳߳�ʱ��ͳ�ͷš�
            // g_pTaskRunnerWeak = nullptr;
            EnableWeakup(false);
        }
        return _oMsg.wParam;
    }

    void __YYAPI ThreadTaskRunnerImpl::EnableWeakup(bool _bEnable)
    {
        if (_bEnable)
        {
            Sync::BitReset(&uWeakupCountAndPushLock, StopWeakupBitIndex);
        }
        else
        {
            Sync::BitSet(&uWeakupCountAndPushLock, StopWeakupBitIndex);
        }
    }
#endif

#ifdef _WIN32
    HRESULT __YYAPI ThreadTaskRunnerImpl::PostTaskInternal(RefPtr<TaskEntry> _pTask)
    {
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
        // ��Ϊ�ղ� uWeakCountAndPushLock �Ѿ�����һ�����λ����λ 1
        // ���������� uWeakCountAndPushLock += 1���ɡ�
        // uWeakCount + 1 <==> uWeakCountAndPushLock + 2 <==> (uWeakCountAndPushLock | 1) + 1
        if (Sync::Add(&uWeakupCountAndPushLock, UnlockQueuePushLockBitAndWeakupOnceRaw) < WeakupOnceRaw * 2u)
        {
            // Ϊ 1 ��˵����ǰ���ڵȴ�������Ϣ������δ��������
            // ��˵��� PostAppMessageW ���Ի���Ŀ���̵߳���Ϣѭ����
            auto _bRet = PostAppMessageW(uThreadId, WM_APP, 0, 0);

            // Post ʧ�ܴ�����ʱ�������������ǵ�ǰϵͳ��Դ���㣬��Ȼ�Ѿ������˶��������������ɡ�
        }
        return S_OK;
    }

    void __YYAPI ThreadTaskRunnerImpl::CleanupTaskQueue() noexcept
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
    }
#endif
} // namespace YY
