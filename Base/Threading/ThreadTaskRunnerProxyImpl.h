#pragma once

#include <new>
#include <functional>

#include <Base/Threading/ThreadTaskRunnerImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class ThreadTaskRunnerProxyImpl
                : public ThreadTaskRunnerBaseImpl
                , public ThreadPoolTimerManger
            {
                static constexpr auto kExecuteTaskRunnerMsg = WM_APP;
                static constexpr auto kTimerTaskTimerId = 100;

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
                        volatile uint32_t bBackgroundLoop : 1;
                        volatile uint32_t uWakeupCount : 29;
                    };
                };
                enum : uint32_t
                {
                    LockedQueuePushBitIndex = 0,
                    StopWakeupBitIndex,
                    InterruptBitIndex,
                    BackgroundLoopIndex,
                    WakeupCountStartBitIndex,
                    StopWakeupRaw = 1 << StopWakeupBitIndex,
                    InterruptRaw = 1 << InterruptBitIndex,
                    BackgroundLoopRaw = 1 << BackgroundLoopIndex,
                    WakeupOnceRaw = 1 << WakeupCountStartBitIndex,
                    UnlockQueuePushLockBitAndWakeupOnceRaw = WakeupOnceRaw - (1u << LockedQueuePushBitIndex),
                    TerminateTaskRunnerRaw = StopWakeupRaw | InterruptRaw,
                };

                uint32_t uThreadId = Threading::GetCurrentThreadId();
                // 故意打开一次句柄，保证ThreadTaskRunner释放前， uThreadId 始终有效。
                HANDLE hThread = nullptr;
                HWND hTaskRunnerWnd = nullptr;
                TickCount<TimePrecise::Microsecond> uTimerWakeupTickCount = TickCount<TimePrecise::Microsecond>::GetMax();

            public:
                ThreadTaskRunnerProxyImpl() = default;

                ~ThreadTaskRunnerProxyImpl()
                {
                    CleanupTaskQueue();

                    if (hThread)
                    {
                        CloseHandle(hThread);
                    }
                }

                ThreadTaskRunnerProxyImpl(const ThreadTaskRunnerProxyImpl&) = delete;

                ThreadTaskRunnerProxyImpl& operator=(const ThreadTaskRunnerProxyImpl&) = delete;

                bool Init()
                {
                    hThread = OpenThread(SYNCHRONIZE, FALSE, uThreadId);
                    if (!hThread)
                        return false;

                    hTaskRunnerWnd = CreateWindowExW(0, L"Message", nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
                    if (!hTaskRunnerWnd)
                        return false;

                    ::SetWindowLongPtr(hTaskRunnerWnd, GWLP_WNDPROC, (LONG_PTR)&ThreadTaskRunnerProxyImpl::TaskRunnerProc);
                    return true;
                }

                /////////////////////////////////////////////////////
                // TaskRunner
                TaskRunnerStyle __YYAPI GetStyle() const noexcept override
                {
                    return TaskRunnerStyle::FixedThread | TaskRunnerStyle::Sequenced;
                }

                /////////////////////////////////////////////////////
                // ThreadTaskRunner

                uint32_t __YYAPI GetThreadId() override
                {
                    return uThreadId;
                }

                HRESULT __YYAPI Join(TimeSpan<TimePrecise::Millisecond> _nWaitTimeOut) noexcept override
                {
                    if (GetCurrent() == this)
                    {
                        // 自己怎么Join自己？
                        return E_UNEXPECTED;
                    }
                    const auto _uWakeupCountAndPushLock = Sync::BitOr(&uWakeupCountAndPushLock, StopWakeupRaw);
                    if (_uWakeupCountAndPushLock < WakeupOnceRaw)
                    {
                        return S_OK;
                    }

                    uint32_t _uTargetValye = TerminateTaskRunnerRaw;
                    static_assert(sizeof(uWakeupCountAndPushLock) == sizeof(_uTargetValye), "");
                    if (!WaitEqualOnAddress(&uWakeupCountAndPushLock, &_uTargetValye, sizeof(_uTargetValye), _nWaitTimeOut))
                    {
                        return __HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                    }

                    return S_OK;
                }

                HRESULT __YYAPI Interrupt() noexcept override
                {
                    Sync::BitOr(&uWakeupCountAndPushLock, StopWakeupRaw | InterruptRaw);
                    return S_OK;
                }

                //
                ////////////////////////////////////////////////////

                uintptr_t __YYAPI RunTaskRunnerLoop() override
                {
                    throw YY::Exception(_S("代理模式无法调用启动循环。"), E_NOTIMPL);
                    return -1;
                }

            private:
                HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) override
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

                uint32_t uSetTimerCount = 0;
                HRESULT __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pTask) override
                {

                    if (_pTask->IsCanceled())
                    {
                        return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
                    }

                    _pTask->pOwnerTaskRunnerWeak = this;

                    if (uThreadId != GetCurrentThreadId())
                    {
                        return PostTask([this, _pTask]() mutable
                            {
                                ThreadPoolTimerManger::SetTimerInternal(std::move(_pTask));
                            });
                    }
                    else
                    {
                        ++uSetTimerCount;
                        auto _hr = ThreadPoolTimerManger::SetTimerInternal(std::move(_pTask));
                        if (SUCCEEDED(_hr))
                        {
                            if (uSetTimerCount == 1)
                            {
                                ExecuteTimerTasks();
                            }
                        }
                        --uSetTimerCount;

                        return _hr;
                    }
                }

                void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) override
                {
                    if (_pTimerTask)
                        _pTimerTask->operator()();
                }

                void __YYAPI CleanupTaskQueue() noexcept
                {
                    for (;;)
                    {
                        auto _pTask = RefPtr<TaskEntry>::FromPtr(oTaskQueue.Pop());
                        if (!_pTask)
                            break;

                        _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                    }

                    Sync::Exchange(&uWakeupCountAndPushLock, TerminateTaskRunnerRaw);
                    WakeByAddressAll((PVOID)&uWakeupCountAndPushLock);
                }

                HRESULT __YYAPI Wakeup() noexcept
                {
                    if (bStopWakeup)
                        return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);

                    if (PostMessageW(hTaskRunnerWnd, kExecuteTaskRunnerMsg, 0, 0))
                        return S_OK;

                    return YY::Base::HRESULT_From_LSTATUS(GetLastError());
                }

                void __YYAPI ExecuteTaskRunner()
                {
                    const auto _uWakeupCount = uWakeupCountAndPushLock / WakeupOnceRaw;
                    if (_uWakeupCount == 0)
                        return;

                    uint32_t _uProcessingCount = 0ul;
                    do
                    {
                        if (!IsShared())
                        {
                            break;
                        }

                        if (bInterrupt)
                        {
                            break;
                        }

                        auto _pTask = oTaskQueue.Pop();
                        assert(_pTask != nullptr);

                        if (_pTask)
                        {
                            ++_uProcessingCount;
                            _pTask->operator()();
                            _pTask->Release();
                        }
                        else
                        {
                            break;
                        }
                    } while (_uProcessingCount != _uWakeupCount);

                    if (Sync::Subtract(&uWakeupCountAndPushLock, WakeupOnceRaw * _uProcessingCount) >= WakeupOnceRaw)
                    {
                        // 队列已经插入新的Task，安排计划重新执行。
                        // 这里不立即重新执行是为了防止Task自身内部PostTask导致这个循环无法退出。
                        Wakeup();
                    }
                }

                void __YYAPI ExecuteTimerTasks()
                {
                    ProcessingTimerTasks();
                    auto _uWakeupTickCount = GetMinimumWakeupTickCount();
                    if (_uWakeupTickCount == TickCount<TimePrecise::Microsecond>::GetMax())
                    {
                        if (uTimerWakeupTickCount != TickCount<TimePrecise::Microsecond>::GetMax())
                        {
                            uTimerWakeupTickCount = TickCount<TimePrecise::Microsecond>::GetMax();
                            KillTimer(hTaskRunnerWnd, kTimerTaskTimerId);
                        }
                    }
                    else
                    {
                        if (_uWakeupTickCount != uTimerWakeupTickCount)
                        {
                            uTimerWakeupTickCount = _uWakeupTickCount;
                            auto _uWaitTime = GetWaitTimeSpan(_uWakeupTickCount);
                            if (_uWaitTime == 0)
                                _uWaitTime = 1;

                            SetTimer(hTaskRunnerWnd, kTimerTaskTimerId, _uWaitTime, nullptr);
                        }
                    }
                }

                static LRESULT CALLBACK TaskRunnerProc(
                    _In_ HWND _hWnd,
                    _In_ UINT _uMsg,
                    _In_ WPARAM _wParam,
                    _In_ LPARAM _lParam)
                {
                    if (_uMsg == kExecuteTaskRunnerMsg || (_uMsg == WM_TIMER && _wParam == kTimerTaskTimerId))
                    {
                        auto _pCurrentTaskRunner = g_pTaskRunnerWeak.Get();
                        if (!_pCurrentTaskRunner)
                        {
                            return 0;
                        }

                        auto _pThreadTaskRunner = dynamic_cast<ThreadTaskRunnerProxyImpl*>(_pCurrentTaskRunner.Get());
                        if (!_pThreadTaskRunner)
                        {
                            return 0;
                        }

                        if (_uMsg == kExecuteTaskRunnerMsg)
                        {
                            _pThreadTaskRunner->ExecuteTaskRunner();

                            if (_pThreadTaskRunner->IsShared() == false
                                || _pThreadTaskRunner->bInterrupt
                                || (_pThreadTaskRunner->bStopWakeup && _pThreadTaskRunner->uWakeupCount == 0))
                            {
                                _pThreadTaskRunner->CleanupTaskQueue();
                            }
                        }
                        else if (_uMsg == WM_TIMER)
                        {
                            _pThreadTaskRunner->ExecuteTimerTasks();
                        }
                        return 0;
                    }

                    return ::CallWindowProcW(&DefWindowProcW, _hWnd, _uMsg, _wParam, _lParam);
                }
            };
        }
    }
}
#pragma pack(pop)
