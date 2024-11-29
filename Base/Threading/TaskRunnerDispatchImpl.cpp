#include "TaskRunnerDispatchImpl.h"

#include <utility>

#ifndef _WIN32
#include <signal.h>
#endif

#include <Base/Time/TimeSpan.h>
#include <Base/Sync/Interlocked.h>

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

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::SetTimerInternal(RefPtr<Timer> _pTimer) noexcept
            {
                if (!_pTimer)
                    return;

                JoinPendingTaskQueue(oPendingTimerTaskQueue, std::move(_pTimer));
            }

            HRESULT __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::SetWaitInternal(RefPtr<Wait> _pWait) noexcept
            {
                if (_pWait == nullptr || _pWait->hHandle == NULL)
                    return E_INVALIDARG;

                JoinPendingTaskQueue(oPendingWaitTaskQueue, std::move(_pWait));
                return S_OK;
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Weakup(int32_t _nNewDispatchTaskRef, uint32_t _uNewFlags)
            {
                if (_nNewDispatchTaskRef < 1)
                {
                    // 当前一部分Task因为时序发生抢跑时，短时间里引用计数可能会小于 1，甚至小于 0
                    // 这时计数恢复时，任务其实已经被执行了。这时唤醒时我们不用做任何事情。
                }
                else if (_nNewDispatchTaskRef == 1)
                {
                    // 之前等待任务计数是0，这说明它没有线程，我们需要给它安排一个线程。
                    auto _hr = ThreadPool::PostTaskInternalWithoutAddRef(this);
                    if (FAILED(_hr))
                    {
                        throw Exception(_hr);
                    }
                }
                else if(_uNewFlags < WakeupRefOnceRaw * 2)
                {
                    // 之前的WakeupRef计数为 0，所以我们需要重新唤醒 Dispatch 线程。
#ifdef _WIN32
                    if (!PostQueuedCompletionStatus(oDefaultWaitBlock.hTaskRunnerServerHandle, 0, 0, nullptr))
                    {
                        throw Exception(HRESULT_From_LSTATUS(GetLastError()));
                    }
#else
                    if (hThread != NullThreadHandle)
                    {
                        pthread_kill(hThread, SIGUSR1);
                    }
#endif
                }
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::StartIo() noexcept
            {
                const auto _nNewDispatchTaskRef = YY::Sync::Increment(&nDispatchTaskRef);                
                Weakup(_nNewDispatchTaskRef);
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

                for (; nDispatchTaskRef > 0;)
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

                    for(;;)
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
                            if (_pDispatchTask->OnComplete(DosErrorFormNtStatus(_pDispatchTask->Internal)))
                            {
                                DispatchTask(std::move(_pDispatchTask));
                            }
                            else
                            {
                                YY::Sync::Decrement(&nDispatchTaskRef);
                            }
                        }

                        _bRet = GetQueuedCompletionStatusEx(oDefaultWaitBlock.hTaskRunnerServerHandle, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, 0, FALSE);
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

                    YY::Sync::Subtract(&fFlags, WakeupRefOnceRaw);
                    auto _hr = ThreadPoolTimerManger::SetTimerInternal(RefPtr<Timer>::FromPtr(_pTimerTask));
                    if (FAILED(_hr))
                    {
                        YY::Sync::Decrement(&nDispatchTaskRef);
                    }
                }

                for (;;)
                {
                    auto _pWait = oPendingWaitTaskQueue.Pop();
                    if (!_pWait)
                        break;

                    YY::Sync::Subtract(&fFlags, WakeupRefOnceRaw);
                    auto _hr = ThreadPoolWaitMangerForMultiThreading::SetWaitInternal(RefPtr<Wait>::FromPtr(_pWait));
                    if (FAILED(_hr))
                    {
                        YY::Sync::Decrement(&nDispatchTaskRef);
                    }
                }
            }

            void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchTask(RefPtr<TaskEntry> _pDispatchTask)
            {
                if (!_pDispatchTask)
                    return;

                YY::Sync::Decrement(&nDispatchTaskRef);

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
