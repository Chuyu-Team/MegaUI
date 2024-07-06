#include "pch.h"
#include "ThreadTaskRunnerImpl.h"

#include <Base/ErrorCode.h>
#include <Base/Sync/Sync.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    // RunUIMessageLoop 进入的次数。
    // 对于 ThreadTaskRunner 来说必须保证消息循环在才能正常工作。
    static thread_local uint32_t s_uUIMessageLoopEnterCount = 0u;

    ThreadTaskRunnerImpl::ThreadTaskRunnerImpl(uint32_t _uThreadId, bool _bBackgroundLoop)
        : uWakeupCountAndPushLock(_bBackgroundLoop ? (WakeupOnceRaw | BackgroundLoopRaw) : WakeupOnceRaw)
        , uThreadId(_uThreadId)
    {
    }

    ThreadTaskRunnerImpl::~ThreadTaskRunnerImpl()
    {
        CleanupTaskQueue();
    }
            
    TaskRunnerStyle __YYAPI ThreadTaskRunnerImpl::GetStyle() const noexcept
    {
        return TaskRunnerStyle::FixedThread | TaskRunnerStyle::Sequenced;
    }
            
    uint32_t __YYAPI ThreadTaskRunnerImpl::GetThreadId()
    {
        if (uThreadId == 0u)
        {
            const uint32_t uThreadIdNone = 0;
            Sync::WaitOnAddress(&uThreadId, (void*)(&uThreadIdNone), sizeof(uThreadIdNone), 10);
        }
        return uThreadId;
    }
            
#ifdef _WIN32
    uintptr_t __YYAPI ThreadTaskRunnerImpl::RunUIMessageLoop(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
    {
        EnterLoop();

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
                auto _pTask = oTaskQueue.Pop();
                if (_pTask)
                {
                    _pTask->operator()();
                    _pTask->Release();
                }

                // uPushLock 占用1bit，所以 uWakeupCount += 1 等价于 uWakeupCountAndPushLock += 2
                if (Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw) < WakeupOnceRaw)
                {
                    // uWakeupCount 已经归零，进入睡眠状态
                    WaitMessage();
                }

                // 消息循环本身因为处于激活状态，所以
                Sync::Add(&uWakeupCountAndPushLock, uint32_t(WakeupOnceRaw));
            }
        }
        LeaveLoop();
        return _oMsg.wParam;
    }
#endif

    void __YYAPI ThreadTaskRunnerImpl::RunBackgroundLoop()
    {
        EnterLoop();

        for (;;)
        {
            if (!IsShared())
            {
                break;
            }

            auto _pTask = oTaskQueue.Pop();
            if (_pTask)
            {
                _pTask->operator()();
                _pTask->Release();
            }

            if (Sync::Subtract(&uWakeupCountAndPushLock, uint32_t(WakeupOnceRaw)) < WakeupOnceRaw)
            {
                for (auto _uCurrent = uWakeupCountAndPushLock; _uCurrent < WakeupOnceRaw; _uCurrent = uWakeupCountAndPushLock)
                {
                    Sync::WaitOnAddress(&uWakeupCountAndPushLock, &_uCurrent, sizeof(_uCurrent), UINT32_MAX);

                    if (!IsShared())
                    {
                        goto END_LOOP;
                    }
                }

                Sync::Add(&uWakeupCountAndPushLock, uint32_t(WakeupOnceRaw));
            }
        }
        
    END_LOOP:
        LeaveLoop();
    }

    void __YYAPI ThreadTaskRunnerImpl::EnableWakeup(bool _bEnable)
    {
        if (_bEnable)
        {
            Sync::BitReset(&uWakeupCountAndPushLock, StopWakeupBitIndex);
        }
        else
        {
            Sync::BitSet(&uWakeupCountAndPushLock, StopWakeupBitIndex);
        }
    }

    HRESULT __YYAPI ThreadTaskRunnerImpl::PostTaskInternal(RefPtr<TaskEntry> _pTask)
    {
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
            // 为 1 是说明当前正在等待输入消息，并且未主动唤醒
            // 如果唤醒失败处理，暂时不做处理，可能是当前系统资源不足，既然已经加入了队列我们先这样吧。
            Wakeup();
        }
        return S_OK;
    }

    void __YYAPI ThreadTaskRunnerImpl::operator()()
    {
        s_uUIMessageLoopEnterCount = 0;
        uThreadId = GetCurrentThreadId();
        Sync::WakeByAddressAll(const_cast<uint32_t*>(&uThreadId));

        if (bBackgroundLoop)
        {
            RunBackgroundLoop();
        }
        else
        {
            RunUIMessageLoop(nullptr, nullptr);
        }

        uThreadId = UINT32_MAX;
        g_pTaskRunnerWeak = nullptr;
    }

    void __YYAPI ThreadTaskRunnerImpl::CleanupTaskQueue() noexcept
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
    }

#ifdef _WIN32
    HRESULT __YYAPI ThreadTaskRunnerImpl::Wakeup() noexcept
    {
        if(uThreadId == UINT32_MAX)
            return HRESULT_From_LSTATUS(ERROR_CANCELLED);

        if (bBackgroundLoop)
        {
            Sync::WakeByAddressAll((void*)(&uWakeupCountAndPushLock));
            return S_OK;
        }
        else
        {
            // 因此调用 PostAppMessageW 尝试唤醒目标线程的消息循环。
            auto _bRet = PostAppMessageW(uThreadId, WM_APP, 0, 0);
            return _bRet ? S_OK : YY::Base::HRESULT_From_LSTATUS(GetLastError());
        }
    }
#endif
    
    void __YYAPI ThreadTaskRunnerImpl::EnterLoop() noexcept
    {
        ++s_uUIMessageLoopEnterCount;
        if (s_uUIMessageLoopEnterCount == 1)
        {
            g_pTaskRunnerWeak = this;
            EnableWakeup(true);
        }
    }
    
    void __YYAPI ThreadTaskRunnerImpl::LeaveLoop() noexcept
    {
        if (s_uUIMessageLoopEnterCount == 0)
        {
            std::abort();
        }

        --s_uUIMessageLoopEnterCount;
        if (s_uUIMessageLoopEnterCount == 0)
        {
            // 对于线程来说，交给weak_ptr控制
            // 线程退出或者交还线程池时再统释放。
            // g_pTaskRunnerWeak = nullptr;
            EnableWakeup(false);
        }
    }
} // namespace YY
