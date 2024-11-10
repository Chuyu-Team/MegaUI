#include "ThreadTaskRunnerImpl.h"

#include <Base/ErrorCode.h>
#include <Base/Sync/Sync.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
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
            uintptr_t __YYAPI ThreadTaskRunnerImpl::RunUIMessageLoop()
            {
                MSG _oMsg;
                for (;;)
                {
                    for (;;)
                    {
                        if (!IsShared())
                        {
                            return HRESULT_From_LSTATUS(ERROR_CANCELLED);
                        }

                        const auto _bRet = PeekMessageW(&_oMsg, NULL, 0, 0, PM_REMOVE);
                        if (_bRet == -1)
                            continue;

                        if (!_bRet)
                            break;

                        if (_oMsg.message == WM_QUIT)
                        {
                            break;
                        }
                        TranslateMessage(&_oMsg);
                        DispatchMessage(&_oMsg);
                    }

                    DispatchTimingWheel();

                    auto _uWakeupCount = Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw) / WakeupOnceRaw;
                    const auto _uWakeupCountBackup = _uWakeupCount;
                    for (;;)
                    {
                        // uPushLock 占用1bit，所以 uWakeupCount += 1 等价于 uWakeupCountAndPushLock += 2
                        if (_uWakeupCount == 0)
                        {
                            if (_uWakeupCountBackup && Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw * _uWakeupCountBackup) >= WakeupOnceRaw)
                            {
                                // 队列已经插入新的Task，重新检查消息循环。
                            }
                            else
                            {
                                // uWakeupCount 已经归零，进入睡眠状态
                                const auto _uWaitTIme = GetMinimumWaitTime();
                                MsgWaitForMultipleObjectsEx(0, nullptr, _uWaitTIme, QS_ALLINPUT, MWMO_ALERTABLE);
                            }
                            // 消息循环本身因为处于激活状态，所以
                            Sync::Add(&uWakeupCountAndPushLock, uint32_t(WakeupOnceRaw));
                            break;
                        }
                        else
                        {
                            --_uWakeupCount;
                            if (!IsShared())
                            {
                                return HRESULT_From_LSTATUS(ERROR_CANCELLED);
                            }

                            auto _pTask = oTaskQueue.Pop();
                            assert(_pTask != nullptr);

                            if (_pTask)
                            {
                                _pTask->operator()();
                                _pTask->Release();
                            }
                        }
                    }
                }
                return _oMsg.wParam;
            }
#endif

            void __YYAPI ThreadTaskRunnerImpl::RunBackgroundLoop()
            {
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
                            DispatchTimingWheel();
                            Sync::WaitOnAddress(&uWakeupCountAndPushLock, &_uCurrent, sizeof(_uCurrent), GetMinimumWaitTime());

                            if (!IsShared())
                            {
                                goto END_LOOP;
                            }
                        }

                        Sync::Add(&uWakeupCountAndPushLock, uint32_t(WakeupOnceRaw));
                    }
                }

            END_LOOP:
                ;
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

            HRESULT __YYAPI ThreadTaskRunnerImpl::SetTimerInternal(RefPtr<Timer> _pTask)
            {
                _pTask->pOwnerTaskRunnerWeak = this;

                if (_pTask->IsCanceled())
                {
                    return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
                }

                if (uThreadId != GetCurrentThreadId())
                {
                    PostTask([this, _pTask]() mutable
                        {
                            ThreadPoolTimerManger::SetTimerInternalNolock(std::move(_pTask));
                        });
                }
                else
                {
                    ThreadPoolTimerManger::SetTimerInternalNolock(std::move(_pTask));
                }

                return S_OK;
            }

            void __YYAPI ThreadTaskRunnerImpl::operator()()
            {
                uThreadId = GetCurrentThreadId();
                Sync::WakeByAddressAll(const_cast<uint32_t*>(&uThreadId));
                g_pTaskRunnerWeak = this;

                if (bBackgroundLoop)
                {
                    RunBackgroundLoop();
                }
                else
                {
                    RunUIMessageLoop();
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
                if (uThreadId == UINT32_MAX)
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
        }
    }
} // namespace YY
