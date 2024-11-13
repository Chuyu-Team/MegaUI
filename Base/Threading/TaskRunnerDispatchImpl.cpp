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
#ifdef _WIN32
                : ThreadPoolWaitMangerForMultiThreading(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u))
#endif
            {
            }

            TaskRunnerDispatchImplByIoCompletionImpl::~TaskRunnerDispatchImplByIoCompletionImpl()
            {
#ifdef _WIN32

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

                if (CreateIoCompletionPort(_hHandle, oDefaultWaitBlock.hTaskRunnerServerHandle, 0, 1) != oDefaultWaitBlock.hTaskRunnerServerHandle)
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
                    if (!Sync::BitSet(&fFlags, SetTimerLockIndex))
                    {
                        oPendingTimerTaskQueue.Push(_pDispatchTask.Detach());
                        Sync::BitReset(&fFlags, SetTimerLockIndex);
                        break;
                    }
                }
                Weakup();
            }

            HRESULT __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::SetWaitInternal(RefPtr<Wait> _pTask) noexcept
            {
                if (_pTask == nullptr || _pTask->hHandle == NULL)
                    return E_INVALIDARG;

                for (;;)
                {
                    if (!Sync::BitSet(&fFlags, SetWaitLockIndex))
                    {
                        oPendingWaitTaskQueue.Push(_pTask.Detach());
                        Sync::BitReset(&fFlags, SetWaitLockIndex);
                        break;
                    }
                }

                Weakup();
                return S_OK;
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
                    PostQueuedCompletionStatus(oDefaultWaitBlock.hTaskRunnerServerHandle, 0, 0, nullptr);
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
                    ProcessingPendingTaskQueue();

                    ProcessingTimerTasks();
                    auto uMinimumWaitTime = GetMinimumWaitTime();

                    BOOL _bRet;
                    if (oDefaultWaitBlock.cWaitHandle > 1)
                    {
                        const auto uWaitResult = WaitForMultipleObjectsEx(oDefaultWaitBlock.cWaitHandle, oDefaultWaitBlock.hWaitHandles, FALSE, uMinimumWaitTime, FALSE);
                        if (uWaitResult == WAIT_OBJECT_0)
                        {
                            _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, 0, FALSE);
                        }
                        else
                        {
                            ProcessingWaitTasks(oDefaultWaitBlock, uWaitResult, oDefaultWaitBlock.cWaitHandle);
                            continue;
                        }
                    }
                    else
                    {
                        _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, uMinimumWaitTime, FALSE);
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

            void TaskRunnerDispatchImplByIoCompletionImpl::ProcessingPendingTaskQueue() noexcept
            {
                for (;;)
                {
                    auto _pTimerTask = oPendingTimerTaskQueue.Pop();
                    if (!_pTimerTask)
                        break;

                    ThreadPoolTimerManger::SetTimerInternal(RefPtr<Timer>::FromPtr(_pTimerTask));
                }

                for (;;)
                {
                    auto _pWait = oPendingWaitTaskQueue.Pop();
                    if (!_pWait)
                        break;

                    ThreadPoolWaitMangerForMultiThreading::SetWaitInternal(RefPtr<Wait>::FromPtr(_pWait));
                }
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchTask(RefPtr<TaskEntry> _pDispatchTask)
            {
                if (!_pDispatchTask)
                    return;

                do
                {
                    if (_pDispatchTask->IsCanceled())
                        break;

                    // 不属于任何TaskRunner，因此在线程池随机唤醒
                    if (_pDispatchTask->pOwnerTaskRunnerWeak == nullptr)
                    {
                        auto _hr = ThreadPool::PostTaskInternal(_pDispatchTask.Get());
                        return;
                    }

                    // 任务所属的 TaskRunner 已经释放？
                    auto _pResumeTaskRunner = _pDispatchTask->pOwnerTaskRunnerWeak.Get();
                    if (_pResumeTaskRunner == nullptr)
                        break;

                    _pResumeTaskRunner->PostTaskInternal(std::move(_pDispatchTask));
                    return;
                } while (false);

                _pDispatchTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchTimerTask(RefPtr<Timer> _pTimerTask)
            {
                DispatchTask(std::move(_pTimerTask));
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchWaitTask(RefPtr<Wait> _pWaitTask)
            {
                DispatchTask(std::move(_pWaitTask));
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
