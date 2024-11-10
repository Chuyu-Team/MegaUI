#include "TaskRunnerDispatchImpl.h"

#include <utility>

#ifndef _WIN32
#include <signal.h>
#endif

#include <Base/Time/TimeSpan.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            TaskRunnerDispatchImplByIoCompletionImpl::TaskRunnerDispatchImplByIoCompletionImpl()
            {
#ifdef _WIN32
                hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u);
#endif
            }

            TaskRunnerDispatchImplByIoCompletionImpl::~TaskRunnerDispatchImplByIoCompletionImpl()
            {
#ifdef _WIN32
                if (hIoCompletionPort)
                {
                    CloseHandle(hIoCompletionPort);
                }
#else
                hThread = NullThreadHandle;
#endif
            }

            TaskRunnerDispatchImplByIoCompletionImpl* __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Get() noexcept
            {
                static TaskRunnerDispatchImplByIoCompletionImpl s_Dispatch;
                return &s_Dispatch;
            }

#ifdef _WIN32
            bool __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::BindIO(HANDLE _hHandle) const noexcept
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
#endif

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::SetTimerInternal(RefPtr<Timer> _pDispatchTask) noexcept
            {
                if (!_pDispatchTask)
                    return;

                for (;;)
                {
                    if (!Sync::BitSet(&fFlags, 1))
                    {
                        ThreadPoolTimerManger::SetTimerInternalNolock(std::move(_pDispatchTask));
                        Sync::BitReset(&fFlags, 1);
                        break;
                    }
                }
                Weakup();
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::SetWaitInternal(RefPtr<Wait> _pTask) noexcept
            {
                if (!_pTask)
                    return;
                for (;;)
                {
                    if (!Sync::BitSet(&fFlags, 2))
                    {
                        ThreadPoolWaitManger::SetWaitInternalNolock(std::move(_pTask));
                        Sync::BitReset(&fFlags, 2);
                        break;
                    }
                }

                Weakup();
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Weakup()
            {
                if (!Sync::BitSet(&fFlags, 0))
                {
                    auto _hr = ThreadPool::PostTaskInternalWithoutAddRef(this);
                    if (FAILED(_hr))
                    {
                        throw Exception(_hr);
                    }
                }
                else
                {
#ifdef _WIN32

                    // TODO: 按需唤醒
                    PostQueuedCompletionStatus(hIoCompletionPort, 0, 0, nullptr);
#else
                    if (hThread != NullThreadHandle)
                    {
                        pthread_kill(hThread, SIGUSR1);
                    }
#endif
                }
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::operator()()
            {
                ExecuteTaskRunner();
            }

#ifdef _WIN32
            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::ExecuteTaskRunner()
            {
                OVERLAPPED_ENTRY _oCompletionPortEntries[16];
                ULONG _uNumEntriesRemoved;

                for (;;)
                {                
                    DispatchTimingWheel();
                    auto uMinimumWaitTime = GetMinimumWaitTime();

                    ProcessingPendingWaitQueue();

                    BOOL _bRet;
                    if (cWaitHandle)
                    {
                        hWaitHandles[cWaitHandle] = hIoCompletionPort;
                        // 额外等待完成端口，所以 cWaitHandle + 1
                        const auto uWaitResult = WaitForMultipleObjectsEx(cWaitHandle + 1, hWaitHandles, FALSE, uMinimumWaitTime, FALSE);
                        if (uWaitResult == WAIT_OBJECT_0 + cWaitHandle)
                        {
                            _bRet = GetQueuedCompletionStatusEx(hIoCompletionPort, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, 0, FALSE);
                        }
                        else
                        {
                            ProcessingWaitResult(uWaitResult);
                            continue;
                        }
                    }
                    else
                    {
                        _bRet = GetQueuedCompletionStatusEx(hIoCompletionPort, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, uMinimumWaitTime, FALSE);
                    }

                    if (!_bRet)
                    {
                        _uNumEntriesRemoved = 0;

                        auto _lStatus = GetLastError();

                        if (_lStatus == WAIT_TIMEOUT || _lStatus == WAIT_IO_COMPLETION)
                        {
                            // 非意外错误
                        }
                        else
                        {
                            // ERROR_ABANDONED_WAIT_0 ： 句柄关闭，线程应该退出。
                            return;
                        }
                    }

                    // 处理完成端口的数据，它的优先级最高
                    for (ULONG _uIndex = 0; _uIndex != _uNumEntriesRemoved; ++_uIndex)
                    {
                        auto _pDispatchTask = RefPtr<TaskEntry>::FromPtr(static_cast<IoTaskEntry*>(_oCompletionPortEntries[_uIndex].lpOverlapped));
                        if (!_pDispatchTask)
                            continue;

                        DispatchTask(std::move(_pDispatchTask));
                    }
                }
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchWaitTask(Wait* _pWaitTask, DWORD _uWaitResult)
            {
                if (!_pWaitTask)
                    return;

                _pWaitTask->uWaitResult = _uWaitResult;
                ThreadPoolTimerManger::DispatchTask(RefPtr<TaskEntry>::FromPtr(_pWaitTask));
            }
#else
            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::ExecuteTaskRunner()
            {
                uTimingWheelCurrentTick = GetNearTick(TickCount<TimePrecise::Millisecond>::GetCurrent());

                sigset_t set;
                sigemptyset(&set);
                sigaddset(&set, SIGUSR1);

                timespec _oTimeOut;

                hThread = pthread_self();

                for (;;)
                {
                    ProcessingPending();

                    const auto _uMinimumWaitTime = GetMinimumWaitTime();

                    if (_uMinimumWaitTime)
                    {
                        if (_uMinimumWaitTime == UINT32_MAX)
                        {
                            sigtimedwait(&set, nullptr, nullptr);
                        }
                        else
                        {
                            _oTimeOut.tv_sec = _uMinimumWaitTime / SecondsPerMillisecond;
                            _oTimeOut.tv_nsec = (_uMinimumWaitTime % SecondsPerMillisecond) * MillisecondsPerMicrosecond * MicrosecondPerNanosecond;
                            sigtimedwait(&set, nullptr, &_oTimeOut);
                        }
                    }

                    auto _oCurrent = GetNearTick(TickCount<TimePrecise::Millisecond>::GetCurrent());
                    DispatchTimingWheel(_oCurrent);
                    uTimingWheelCurrentTick = _oCurrent;
                }
            }
#endif
        }
    }
}
