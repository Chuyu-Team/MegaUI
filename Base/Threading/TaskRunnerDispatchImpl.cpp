#include "TaskRunnerDispatchImpl.h"

#include <utility>

#ifndef _WIN32
#include <signal.h>
#endif

#include <Base/Time/TimeSpan.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Utils/SystemInfo.h>
#include <Base/Threading/TaskRunnerImpl.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            template<typename DerivedClass>
            class TaskRunnerDispatchBaseImpl
                : public TaskRunnerDispatch
                , public ThreadPoolTimerManger
                , public ThreadPoolWaitMangerForMultiThreading
            {
            protected:
                // 该 Dispatch持有的任务引用计数，如果引用计数 <=0，那么可以归将线程归还给全局线程池。
                volatile int32_t nDispatchTaskRef = 0ul;

                enum : uint32_t
                {
                    PendingTaskQueuePushLockBitIndex = 0,
                    WakeupRefBitIndex,

                    WakeupRefOnceRaw = 1 << WakeupRefBitIndex,
                    UnlockQueuePushLockBitAndWakeupRefOnceRaw = WakeupRefOnceRaw - (1u << PendingTaskQueuePushLockBitIndex),
                };

                union
                {
                    volatile uint32_t fFlags = 0ul;
                    struct
                    {
                        uint32_t bPendingTaskQueuePushLock : 1;
                        // 唤醒的引用计数，这里偶尔因为时序，从 0 溢出也没事。
                        // 这里只是做一个记录。
                        uint32_t uWakeupRef : 31;
                    };
                };

                InterlockedQueue<Timer> oPendingTimerTaskQueue;
                InterlockedQueue<Wait> oPendingWaitTaskQueue;

            public:
#ifdef _WIN32
                TaskRunnerDispatchBaseImpl(HANDLE _hTaskRunnerServerHandle)
                    : ThreadPoolWaitMangerForMultiThreading(_hTaskRunnerServerHandle)
                {
                }
#endif

                ~TaskRunnerDispatchBaseImpl()
                {
                    while (auto _pTask = oPendingTimerTaskQueue.Pop())
                    {
                        _pTask->Cancel();
                        _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                        _pTask->Release();
                    }

                    while (auto _pTask = oPendingWaitTaskQueue.Pop())
                    {
                        _pTask->Cancel();
                        _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                        _pTask->Release();
                    }
                }

                void __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pTimer) noexcept override
                {
                    if (!_pTimer)
                        return;

                    JoinPendingTaskQueue(oPendingTimerTaskQueue, std::move(_pTimer));
                }

                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pWait) noexcept override
                {
                    if (_pWait == nullptr || _pWait->hHandle == NULL)
                        return E_INVALIDARG;

                    JoinPendingTaskQueue(oPendingWaitTaskQueue, std::move(_pWait));
                    return S_OK;
                }

            protected:
                size_t ProcessingPendingTaskQueue() noexcept
                {
                    size_t _cTaskProcessed = 0;
                    for (;;)
                    {
                        auto _pTimerTask = oPendingTimerTaskQueue.Pop();
                        if (!_pTimerTask)
                            break;

                        YY::Sync::Subtract(&fFlags, WakeupRefOnceRaw);
                        auto _hr = ThreadPoolTimerManger::SetTimerInternal(RefPtr<Timer>::FromPtr(_pTimerTask));
                        if (FAILED(_hr))
                        {
                            ++_cTaskProcessed;
                        }
                    }

                    for (;;)
                    {
                        auto _pWait = oPendingWaitTaskQueue.Pop();
                        if (!_pWait)
                            break;

                        YY::Sync::Subtract(&fFlags, WakeupRefOnceRaw);
                        auto _hr = ThreadPoolWaitMangerForMultiThreading::SetWaitInternal(RefPtr<Wait>::FromPtr(_pWait));
                        if (_hr == S_FALSE ||  FAILED(_hr))
                        {
                            ++_cTaskProcessed;
                        }
                    }
                    return _cTaskProcessed;
                }

                void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) override
                {
                    if (_pTimerTask)
                    {
                        DispatchTask(std::move(_pTimerTask));
                    }
                }

                void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) override
                {
                    if (_pWaitTask)
                    {
                        DispatchTask(std::move(_pWaitTask));
                    }
                }

                template<typename TaskType>
                void __YYAPI JoinPendingTaskQueue(InterlockedQueue<TaskType>& _oPendingTaskQueue, _In_ RefPtr<TaskType> _pTask) noexcept
                {
                    // 先增加任务计数，防止这段时间，服务线程不必要的意外归还线程池
                    const auto _nNewDispatchTaskRef = YY::Sync::Increment(&nDispatchTaskRef);
                    for (;;)
                    {
                        if (!Sync::BitSet(&fFlags, PendingTaskQueuePushLockBitIndex))
                        {
                            _oPendingTaskQueue.Push(_pTask.Detach());
                            const auto _uNewFlags = Sync::Add(&fFlags, UnlockQueuePushLockBitAndWakeupRefOnceRaw);
                            static_cast<DerivedClass*>(this)->Weakup(_nNewDispatchTaskRef, _uNewFlags);
                            break;
                        }
                    }

                    return;
                }
            };

#ifdef _WIN32
            /// <summary>
            /// 用于Windows XP以及更高版本的Task调度器。
            /// 早期版本的系统 IoCompletionPort 无法等待，始终处于有信号状态。因此需要这个特殊的调度器。
            /// 此调度器普通状态下会启动二个线程：
            /// 其中一个线程专门用来处理 IoCompletionPort。
            /// 另外一个线程用于处理TimerManger与WaitManger。
            /// </summary>
            class TaskRunnerDispatchForWindowsXPOrLater
                : public TaskRunnerDispatchBaseImpl<TaskRunnerDispatchForWindowsXPOrLater>
            {
            private:
                HANDLE hIoCompletionPort = NULL;
                volatile int32_t nIoCompletionPortTaskRef = 0ul;

            public:
                TaskRunnerDispatchForWindowsXPOrLater()
                    : TaskRunnerDispatchBaseImpl(CreateEventW(nullptr, FALSE, FALSE, nullptr))
                    , hIoCompletionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u))
                {
                }

                ~TaskRunnerDispatchForWindowsXPOrLater()
                {
                    CloseHandle(hIoCompletionPort);
                    YY::Exchange(&nDispatchTaskRef, 0);
                    SetEvent(oDefaultWaitBlock.hTaskRunnerServerHandle);
                }

                bool __YYAPI BindIO(_In_ HANDLE _hHandle) const noexcept override
                {
                    if (_hHandle == INVALID_HANDLE_VALUE)
                    {
                        return false;
                    }

                    if (CreateIoCompletionPort(_hHandle, hIoCompletionPort, 0, 1) != hIoCompletionPort)
                    {
                        return false;
                    }

                    return true;
                }

                void __YYAPI StartIo() noexcept override
                {
                    const auto _nNewIoCompletionPortTaskRef = YY::Sync::Increment(&nIoCompletionPortTaskRef);
                    if (_nNewIoCompletionPortTaskRef == 1)
                    {
                        auto _bRet = TrySubmitThreadpoolCallback(
                            [](_Inout_ PTP_CALLBACK_INSTANCE _pInstance,
                                _In_   PVOID _pContext)
                            {
                                auto _pTask = reinterpret_cast<TaskRunnerDispatchForWindowsXPOrLater*>(_pContext);
                                SetThreadDescription(GetCurrentThread(), L"IOCP调度线程");
                                _pTask->ExecuteIoCompletionPort();
                                SetThreadDescription(GetCurrentThread(), L"");
                            },
                            this,
                            nullptr);

                        if (!_bRet)
                        {
                            throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                        }
                    }
                }

                void __YYAPI Weakup(int32_t _nNewDispatchTaskRef, uint32_t _uNewFlags = UINT32_MAX)
                {
                    if (_nNewDispatchTaskRef < 1)
                    {
                        // 当前一部分Task因为时序发生抢跑时，短时间里引用计数可能会小于 1，甚至小于 0
                        // 这时计数恢复时，任务其实已经被执行了。这时唤醒时我们不用做任何事情。
                    }
                    else if (_nNewDispatchTaskRef == 1)
                    {
                        // 之前等待任务计数是0，这说明它没有线程，我们需要给它安排一个线程。
                        auto _bRet = TrySubmitThreadpoolCallback(
                            [](_Inout_ PTP_CALLBACK_INSTANCE _pInstance,
                                _In_   PVOID _pContext)
                            {
                                auto _pTask = reinterpret_cast<TaskRunnerDispatchForWindowsXPOrLater*>(_pContext);
                                SetThreadDescription(GetCurrentThread(), L"Timer/Wait调度线程");
                                _pTask->ExecuteTaskRunner();
                                SetThreadDescription(GetCurrentThread(), L"");
                            },
                            this,
                            nullptr);

                        if (!_bRet)
                        {
                            throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                        }
                    }
                    else if (_uNewFlags < WakeupRefOnceRaw * 2)
                    {
                        // 之前的WakeupRef计数为 0，所以我们需要重新唤醒 Dispatch 线程。
                        if (!SetEvent(oDefaultWaitBlock.hTaskRunnerServerHandle))
                        {
                            throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                        }
                    }
                }

            private:
                void __YYAPI ExecuteIoCompletionPort() noexcept
                {
                    uint32_t _cTaskProcessed = 0;
                    OVERLAPPED_ENTRY _oCompletionPortEntries[16];
                    ULONG _uNumEntriesRemoved;
                    for(;;)
                    {
                        if (YY::Sync::Subtract(&nIoCompletionPortTaskRef, _cTaskProcessed) <= 0)
                            return;

                        _cTaskProcessed = 0;
                        auto _bRet = GetQueuedCompletionStatusEx(hIoCompletionPort, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, INFINITE, FALSE);
                        if (!_bRet)
                        {
                            const auto _lStatus = GetLastError();
                            if (_lStatus == WAIT_TIMEOUT || _lStatus == WAIT_IO_COMPLETION)
                            {
                                // 非意外错误
                                continue;
                            }
                            else
                            {
                                // ERROR_ABANDONED_WAIT_0 ： 这句柄关闭，线程应该退出了？所以我们也可以退出了？
                                return;
                            }
                        }

                        for (ULONG _uIndex = 0; _uIndex != _uNumEntriesRemoved; ++_uIndex)
                        {
                            auto _pDispatchTask = RefPtr<IoTaskEntry>::FromPtr(static_cast<IoTaskEntry*>(_oCompletionPortEntries[_uIndex].lpOverlapped));
                            if (!_pDispatchTask)
                                continue;

                            // 错误代码如果已经设置，那么可能调用者线程已经事先处理了。
                            if (_pDispatchTask->OnComplete(DosErrorFormNtStatus(long(_pDispatchTask->Internal))))
                            {
                                DispatchTask(std::move(_pDispatchTask));
                            }
                            ++_cTaskProcessed;
                        }
                    }
                }

                void __YYAPI ExecuteTaskRunner()
                {
                    size_t _cTaskProcessed = 0;
                    for (;;)
                    {
                        auto _oCurrent = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        _cTaskProcessed += ProcessingTimerTasks(_oCurrent);
                        _cTaskProcessed += ProcessingPendingTaskQueue();
                        if (YY::Sync::Subtract(&nDispatchTaskRef, int32_t(_cTaskProcessed)) <= 0)
                            return;

                        _cTaskProcessed = 0;

                        const auto _uTimerWakeupTickCount = GetMinimumWakeupTickCount();
                        const auto _uWaitTaskWakeupTickCount = oDefaultWaitBlock.GetWakeupTickCountNolock(_oCurrent);
                        auto _uWakeupTickCount = (std::min)(_uTimerWakeupTickCount, _uWaitTaskWakeupTickCount);
                        const auto uWaitResult = WaitForMultipleObjectsEx(oDefaultWaitBlock.cWaitHandle, oDefaultWaitBlock.hWaitHandles, FALSE, GetWaitTimeSpan(_uWakeupTickCount), FALSE);
                        if (uWaitResult == WAIT_OBJECT_0)
                        {
                            continue;
                        }
                        else if (uWaitResult == WAIT_TIMEOUT)
                        {
                            if (_uTimerWakeupTickCount < _uWaitTaskWakeupTickCount)
                            {
                                continue;
                            }
                        }
                        
                        _cTaskProcessed += ProcessingWaitTasks(oDefaultWaitBlock, uWaitResult, oDefaultWaitBlock.cWaitHandle);
                    }
                }
            };
#endif

#ifdef _WIN32
            /// <summary>
            /// 用于 Windows 10 10240 以及更高版本的Task调度器。
            /// 从Windows 10 10240 开始，IoCompletionPort也可以正常等待。因此IoCompletionPort、TimerManger、WaitManger三者复用同一个线程，以节省开销。
            /// </summary>
            class TaskRunnerDispatchForWindows10_0_10240OrLater
                : public TaskRunnerDispatchBaseImpl<TaskRunnerDispatchForWindows10_0_10240OrLater>
            {
            public:
                TaskRunnerDispatchForWindows10_0_10240OrLater()
                    : TaskRunnerDispatchBaseImpl(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u))
                {
                }

                bool __YYAPI BindIO(_In_ HANDLE _hHandle) const noexcept override
                {
                    if (_hHandle == INVALID_HANDLE_VALUE)
                    {
                        return false;
                    }

                    if (CreateIoCompletionPort(_hHandle, oDefaultWaitBlock.hTaskRunnerServerHandle, 0, 1) != oDefaultWaitBlock.hTaskRunnerServerHandle)
                    {
                        return false;
                    }

                    return true;
                }

                void __YYAPI StartIo() noexcept override
                {
                    const auto _nNewDispatchTaskRef = YY::Sync::Increment(&nDispatchTaskRef);
                    Weakup(_nNewDispatchTaskRef);
                }

                void __YYAPI Weakup(_In_ int32_t _nNewDispatchTaskRef, _In_ uint32_t _uNewFlags = UINT32_MAX)
                {
                    if (_nNewDispatchTaskRef < 1)
                    {
                        // 当前一部分Task因为时序发生抢跑时，短时间里引用计数可能会小于 1，甚至小于 0
                        // 这时计数恢复时，任务其实已经被执行了。这时唤醒时我们不用做任何事情。
                    }
                    else if (_nNewDispatchTaskRef == 1)
                    {
                        // 之前等待任务计数是0，这说明它没有线程，我们需要给它安排一个线程。
                        const auto _bRet = TrySubmitThreadpoolCallback(
                            [](_Inout_ PTP_CALLBACK_INSTANCE _pInstance,
                                _In_   PVOID _pContext)
                            {
                                auto _pTask = reinterpret_cast<TaskRunnerDispatchForWindows10_0_10240OrLater*>(_pContext);
                                SetThreadDescription(GetCurrentThread(), L"IOCP/Timer/Wait调度线程");
                                _pTask->ExecuteTaskRunner();
                                SetThreadDescription(GetCurrentThread(), L"");
                            },
                            this,
                            nullptr);

                        if (!_bRet)
                        {
                            throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                        }
                    }
                    else if (_uNewFlags < WakeupRefOnceRaw * 2)
                    {
                        // 之前的WakeupRef计数为 0，所以我们需要重新唤醒 Dispatch 线程。
                        if (!PostQueuedCompletionStatus(oDefaultWaitBlock.hTaskRunnerServerHandle, 0, 0, nullptr))
                        {
                            throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                        }
                    }
                }

            private:
                void __YYAPI ExecuteTaskRunner()
                {
                    size_t _cTaskProcessed = 0;
                    OVERLAPPED_ENTRY _oCompletionPortEntries[16];
                    ULONG _uNumEntriesRemoved;

                    for (;;)
                    {
                        auto _oCurrent = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        _cTaskProcessed += ProcessingTimerTasks(_oCurrent);
                        _cTaskProcessed += ProcessingPendingTaskQueue();
                        if (YY::Sync::Subtract(&nDispatchTaskRef, int32_t(_cTaskProcessed)) <= 0)
                            return;

                        _cTaskProcessed = 0;
                        const auto _uTimerWakeupTickCount = GetMinimumWakeupTickCount();

                        BOOL _bRet;
                        if (oDefaultWaitBlock.cWaitHandle > 1)
                        {
                            const auto _uWaitTaskWakeupTickCount = oDefaultWaitBlock.GetWakeupTickCountNolock(_oCurrent);
                            auto _uWakeupTickCount = (std::min)(_uTimerWakeupTickCount, _uWaitTaskWakeupTickCount);
                            const auto uWaitResult = WaitForMultipleObjectsEx(oDefaultWaitBlock.cWaitHandle, oDefaultWaitBlock.hWaitHandles, FALSE, GetWaitTimeSpan(_uWakeupTickCount), FALSE);
                            if (uWaitResult == WAIT_OBJECT_0)
                            {
                                _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, 0, FALSE);
                            }
                            else
                            {
                                if (uWaitResult == WAIT_TIMEOUT)
                                {
                                    if (_uTimerWakeupTickCount < _uWaitTaskWakeupTickCount)
                                    {
                                        continue;
                                    }
                                }

                                _cTaskProcessed += ProcessingWaitTasks(oDefaultWaitBlock, uWaitResult, oDefaultWaitBlock.cWaitHandle);
                                continue;
                            }
                        }
                        else
                        {
                            _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, GetWaitTimeSpan(_uTimerWakeupTickCount), FALSE);
                        }

                        for (;;)
                        {
                            if (!_bRet)
                            {
                                auto _lStatus = GetLastError();

                                if (_lStatus == WAIT_TIMEOUT || _lStatus == WAIT_IO_COMPLETION)
                                {
                                    // 非意外错误
                                    break;
                                }
                                else
                                {
                                    // ERROR_ABANDONED_WAIT_0 ： 句柄关闭，线程应该退出了。
                                    return;
                                }
                            }

                            // 处理完成端口的数据，它的优先级最高
                            for (ULONG _uIndex = 0; _uIndex != _uNumEntriesRemoved; ++_uIndex)
                            {
                                auto _pDispatchTask = RefPtr<IoTaskEntry>::FromPtr(static_cast<IoTaskEntry*>(_oCompletionPortEntries[_uIndex].lpOverlapped));
                                if (!_pDispatchTask)
                                    continue;

                                // 错误代码如果已经设置，那么可能调用者线程已经事先处理了。
                                if (_pDispatchTask->OnComplete(DosErrorFormNtStatus(long(_pDispatchTask->Internal))))
                                {
                                    DispatchTask(std::move(_pDispatchTask));
                                }

                                ++_cTaskProcessed;
                            }

                            _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, 0, FALSE);
                        }
                    }
                }
            };
#endif
            TaskRunnerDispatch* __YYAPI TaskRunnerDispatch::Get() noexcept
            {
                static TaskRunnerDispatch* s_pCurrentTaskRunnerDispatch = nullptr;
                if (!s_pCurrentTaskRunnerDispatch)
                {
#ifdef _WIN32
                    if (GetOperatingSystemVersion() >= kWindowsNT10_10240)
                    {
                        static TaskRunnerDispatchForWindows10_0_10240OrLater s_TaskRunnerDispatch;
                        s_pCurrentTaskRunnerDispatch = &s_TaskRunnerDispatch;
                    }
                    else
                    {
                        static TaskRunnerDispatchForWindowsXPOrLater s_TaskRunnerDispatch;
                        s_pCurrentTaskRunnerDispatch = &s_TaskRunnerDispatch;
                    }
#else
#error "其他系统尚未适配"
#endif
                }
                return s_pCurrentTaskRunnerDispatch;
            }
}
    }
}
