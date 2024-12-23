#include <Base/Threading/TaskRunner.h>

#include <Base/Exception.h>
#include <Base/Threading/ThreadTaskRunnerImpl.h>
#include <Base/Threading/SequencedTaskRunnerImpl.h>
#include <Base/Threading/ParallelTaskRunnerImpl.hpp>
#include <Base/Sync/Sync.h>
#include <Base/Threading/TaskRunnerDispatchImpl.h>
#include <Base/Threading/ThreadTaskRunnerProxyImpl.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            void __YYAPI TaskEntry::Wakeup(HRESULT _hrCode)
            {
                hr = _hrCode;
                WakeByAddressAll(&hr);
            }

            bool __YYAPI TaskEntry::Wait(uint32_t _uMilliseconds)
            {
                HRESULT _hrTarget = E_PENDING;
                return WaitOnAddress(&hr, &_hrTarget, sizeof(_hrTarget), _uMilliseconds);
            }

            HRESULT __YYAPI Timer::RunTask()
            {
                if (IsCanceled())
                    return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);

                if (uInterval.GetMilliseconds() <= 0)
                {
                    pfnTaskCallback();
                    return S_OK;
                }
                else
                {
                    auto _uExpire = TickCount<TimePrecise::Microsecond>::GetCurrent() + uInterval;
                    if (pfnTimerCallback())
                    {
                        auto _pOwnerTaskRunner = pOwnerTaskRunnerWeak.Get();
                        // 任务被取消？
                        if (!_pOwnerTaskRunner)
                            return S_OK;

                        uExpire = _uExpire;
                        return _pOwnerTaskRunner->SetTimerInternal(this);
                    }
                    return S_OK;
                }
            }

            Threading::TaskRunner::TaskRunner()
                : uTaskRunnerId(GenerateNewTaskRunnerId())
            {
            }

            RefPtr<TaskRunner> __YYAPI Threading::TaskRunner::GetCurrent()
            {
                return g_pTaskRunnerWeak.Get();
            }

            bool __YYAPI TaskRunner::BindIO(HANDLE _hHandle) noexcept
            {
                return TaskRunnerDispatch::Get()->BindIO(_hHandle);
            }

            void __YYAPI TaskRunner::StartIo() noexcept
            {
                return TaskRunnerDispatch::Get()->StartIo();
            }

            HRESULT __YYAPI TaskRunner::PostDelayTask(TimeSpan<TimePrecise::Millisecond> _uAfter, std::function<void(void)>&& _pfnTaskCallback)
            {
                auto _uExpire = TickCount<TimePrecise::Microsecond>::GetCurrent() + _uAfter;
                auto _pTimer = RefPtr<Timer>::Create();
                if (!_pTimer)
                    return E_OUTOFMEMORY;

                _pTimer->pfnTaskCallback = std::move(_pfnTaskCallback);
                _pTimer->uExpire = _uExpire;
                return SetTimerInternal(std::move(_pTimer));
            }

            HRESULT __YYAPI TaskRunner::PostTask(std::function<void(void)>&& _pfnTaskCallback)
            {
                auto _pTask = RefPtr<TaskEntry>::Create();
                if (!_pTask)
                    return E_OUTOFMEMORY;

                _pTask->pfnTaskCallback = std::move(_pfnTaskCallback);
                return PostTaskInternal(_pTask);
            }

            RefPtr<SequencedTaskRunner> __YYAPI SequencedTaskRunner::GetCurrent()
            {
                auto _pTaskRunner = g_pTaskRunnerWeak.Get();
                if (_pTaskRunner == nullptr || HasFlags(_pTaskRunner->GetStyle(), TaskRunnerStyle::Sequenced) == false)
                    return nullptr;

                return RefPtr<SequencedTaskRunner>(std::move(_pTaskRunner));
            }

            RefPtr<SequencedTaskRunner> __YYAPI SequencedTaskRunner::Create(uString _szThreadDescription)
            {
                return RefPtr<SequencedTaskRunnerImpl>::Create(std::move(_szThreadDescription));
            }

#if defined(_HAS_CXX20) && _HAS_CXX20
            TaskAwaiter<void> __YYAPI TaskRunner::AsyncDelayTask(TimeSpan<TimePrecise::Millisecond> _uAfter, std::function<void(void)>&& _pfnTaskCallback)
            {
                struct AsyncTaskEntry
                    : public Timer
                    , public TaskAwaiter<void>::RefData
                {
                    uint32_t __YYAPI AddRef() noexcept override
                    {
                        return Timer::AddRef();
                    }

                    uint32_t __YYAPI Release() noexcept override
                    {
                        return Timer::Release();
                    }

                    HRESULT __YYAPI RunTask() override
                    {
                        auto _hr = Timer::RunTask();
                        if (FAILED(_hr))
                            return _hr;

                        auto _hHandle = (void*)YY::Base::Sync::Exchange(&hCoroutineHandle, /*hReadyHandle*/ (intptr_t)-1);
                        if (!_hHandle)
                            return S_OK;

                        // 如果 pResumeTaskRunner == nullptr，目标不属于任何一个 SequencedTaskRunner，这很可能任务不关下是否需要串行
                        // 如果 pResumeTaskRunner == SequencedTaskRunner::GetCurrent()，这没有道理进行 PostTask，徒增开销。
                        auto _pResumeTaskRunner = pResumeTaskRunnerWeak.Get();
                        if (pResumeTaskRunnerWeak == nullptr || _pResumeTaskRunner == YY::Base::Threading::TaskRunner::GetCurrent())
                        {
                            std::coroutine_handle<>::from_address(_hHandle).resume();
                            return S_OK;
                        }
                        else if (_pResumeTaskRunner)
                        {
                            // TODO: 如果 _pResumeTaskRunner 没有执行 resume，则将发生内存泄漏。
                            _pResumeTaskRunner->PostTask(
                                [_hHandle]()
                                {
                                    std::coroutine_handle<>::from_address(_hHandle).resume();
                                });

                            return S_OK;
                        }
                        else
                        {
                            // 任务被取消
                            std::coroutine_handle<>::from_address(_hHandle).destroy();
                            return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
                        }
                    }
                };

                auto _pCurrent = YY::Base::Threading::TaskRunner::GetCurrent();
                auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
                if (!_pAsyncTaskEntry)
                    throw Exception();

                _pAsyncTaskEntry->pfnTaskCallback = std::move(_pfnTaskCallback);
                _pAsyncTaskEntry->pResumeTaskRunnerWeak = _pCurrent;

                HRESULT _hr;
                if (_uAfter.GetInternalValue() > 0)
                {
                    _pAsyncTaskEntry->uExpire = TickCount<TimePrecise::Microsecond>::GetCurrent() + _uAfter;
                    _hr = SetTimerInternal(_pAsyncTaskEntry);
                }
                else
                {
                    _hr = PostTaskInternal(_pAsyncTaskEntry);
                }

                if (FAILED(_hr))
                    throw Exception();

                return TaskAwaiter<void>(std::move(_pAsyncTaskEntry));
            }
#endif

            HRESULT __YYAPI TaskRunner::SendTask(std::function<void(void)>&& pfnTaskCallback)
            {
                // 调用者的跟执行者属于同一个TaskRunner，这时我们直接调用 _pfnCallback，避免各种等待以及任务投递开销。
                if (TaskRunner::GetCurrent() == this)
                {
                    pfnTaskCallback();
                    return S_OK;
                }

                TaskEntry _oWorkEntry;
                _oWorkEntry.fStyle = TaskEntryStyle::Sync;
                _oWorkEntry.pfnTaskCallback = std::move(pfnTaskCallback);

                auto _hr = PostTaskInternal(&_oWorkEntry);
                if (FAILED(_hr))
                {
                    return _hr;
                }
                _oWorkEntry.Wait();
                return _oWorkEntry.hr;
            }

            RefPtr<Timer> __YYAPI TaskRunner::CreateTimer(TimeSpan<TimePrecise::Millisecond> _uInterval, std::function<bool(void)>&& _pfnTaskCallback)
            {
                if (_uInterval.GetMilliseconds() <= 0)
                    return nullptr;

                auto _uCurrent = TickCount<TimePrecise::Microsecond>::GetCurrent();
                auto _pTimer = RefPtr<Timer>::Create();
                if (!_pTimer)
                    return nullptr;

                _pTimer->pfnTimerCallback = std::move(_pfnTaskCallback);
                _pTimer->uInterval = _uInterval;
                _pTimer->uExpire = _uCurrent + _uInterval;
                auto _hr = SetTimerInternal(_pTimer);
                if (FAILED(_hr))
                    return nullptr;

                return _pTimer;
            }

            RefPtr<Wait> __YYAPI TaskRunner::CreateWait(HANDLE _hHandle, TimeSpan<TimePrecise::Millisecond> _nWaitTimeOut, std::function<bool(DWORD _uWaitResult)>&& _pfnTaskCallback)
            {
                if (_hHandle == nullptr || _hHandle == INVALID_HANDLE_VALUE)
                    return nullptr;

                if (!_pfnTaskCallback)
                    return nullptr;

                auto _pWait = RefPtr<Wait>::Create();
                if (!_pWait)
                    return nullptr;

                _pWait->hHandle = _hHandle;
                // >= UINT32_MAX 时认为是无限等待。
                if (_nWaitTimeOut >= TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(UINT32_MAX))
                {
                    _pWait->uTimeOut = TickCount<TimePrecise::Microsecond>::GetMax();
                }
                else
                {
                    _pWait->uTimeOut = TickCount<TimePrecise::Microsecond>::GetCurrent() + _nWaitTimeOut;
                }

                _pWait->pfnWaitTaskCallback = std::move(_pfnTaskCallback);
                auto _hr = SetWaitInternal(_pWait);
                if (FAILED(_hr))
                    return nullptr;

                return _pWait;
            }

            HRESULT __YYAPI TaskRunner::SetTimerInternal(RefPtr<Timer> _pTask)
            {
                _pTask->pOwnerTaskRunnerWeak = this;

                if (_pTask->IsCanceled())
                {
                    return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
                }

                if (_pTask->uExpire.GetInternalValue() == 0)
                    return PostTaskInternal(std::move(_pTask));

                // 现在的时间已经比过期时间大，那么立即触发任务，降低延迟
                auto _uCurrent = TickCount<TimePrecise::Microsecond>::GetCurrent();
                if (_pTask->uExpire <= _uCurrent)
                {
                    _pTask->uExpire = _uCurrent;
                    return PostTaskInternal(std::move(_pTask));
                }
                else
                {
                    TaskRunnerDispatch::Get()->SetTimerInternal(std::move(_pTask));
                    return S_OK;
                }
            }

            HRESULT __YYAPI TaskRunner::SetWaitInternal(RefPtr<Wait> _pTask)
            {
                _pTask->pOwnerTaskRunnerWeak = this;

                if (_pTask->IsCanceled())
                {
                    return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);
                }

                TaskRunnerDispatch::Get()->SetWaitInternal(std::move(_pTask));
                return S_OK;
            }

            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::Create(bool _bBackgroundLoop, uString _szThreadDescription)
            {
                auto _pTaskRunner = RefPtr<ThreadTaskRunnerImpl>::Create(0u, _bBackgroundLoop, std::move(_szThreadDescription));
                if (_pTaskRunner)
                {
                    auto _hr = ThreadPool::PostTaskInternal(_pTaskRunner.Get());
                    if (FAILED(_hr))
                    {
                        return nullptr;
                    }
                }
                return _pTaskRunner;
            }

            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::GetCurrent()
            {
                auto _pTaskRunner = g_pTaskRunnerWeak.Get();

                // 当前 TaskRunner 必须是物理线程。
                if (_pTaskRunner == nullptr || HasFlags(_pTaskRunner->GetStyle(), TaskRunnerStyle::FixedThread) == false)
                    return nullptr;

                return RefPtr<ThreadTaskRunner>(std::move(_pTaskRunner));
            }

            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::BindCurrentThread()
            {
                if (g_pTaskRunnerWeak.Get())
                    return nullptr;

                auto _pThreadTaskRunnerImpl = RefPtr<ThreadTaskRunnerImpl>::Create();
                if (!_pThreadTaskRunnerImpl)
                    return nullptr;

                g_pTaskRunnerWeak = _pThreadTaskRunnerImpl;
                return _pThreadTaskRunnerImpl;
            }

            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::BindCurrentThreadForProxyMode()
            {
                if (g_pTaskRunnerWeak.Get())
                    return nullptr;

                auto _pThreadTaskRunnerImpl = RefPtr<ThreadTaskRunnerProxyImpl>::Create();
                if (!_pThreadTaskRunnerImpl)
                    return nullptr;

                if (!_pThreadTaskRunnerImpl->Init())
                    return nullptr;

                g_pTaskRunnerWeak = _pThreadTaskRunnerImpl;
                return _pThreadTaskRunnerImpl;
            }

            uintptr_t __YYAPI ThreadTaskRunner::RunUIMessageLoop()
            {
                RefPtr<ThreadTaskRunnerBaseImpl> _pThreadTaskRunnerImpl;

                if (auto _pTaskRunner = g_pTaskRunnerWeak.Get())
                {
                    if (!HasFlags(_pTaskRunner->GetStyle(), TaskRunnerStyle::FixedThread))
                    {
                        // 非物理线程不能进行UI消息循环！！！
                        // 暂时设计如此，未来再说。
                        throw Exception(E_INVALIDARG);
                    }

                    _pThreadTaskRunnerImpl = std::move(_pTaskRunner);
                }
                else
                {
                    throw Exception(L"尚未调用 BindCurrentThread。", E_INVALIDARG);
                }

                auto _uResult = _pThreadTaskRunnerImpl->RunTaskRunnerLoop();
                return _uResult;
            }

            RefPtr<ParallelTaskRunner> __YYAPI ParallelTaskRunner::GetCurrent() noexcept
            {
                auto _pTaskRunner = g_pTaskRunnerWeak.Get();
                if (_pTaskRunner == nullptr || _pTaskRunner->GetStyle() != TaskRunnerStyle::None)
                    return nullptr;

                return RefPtr<ParallelTaskRunner>(std::move(_pTaskRunner));
            }

            RefPtr<ParallelTaskRunner> __YYAPI ParallelTaskRunner::Create(uint32_t _uParallelMaximum, uString _szThreadDescription) noexcept
            {
                return RefPtr<ParallelTaskRunnerImpl>::Create(_uParallelMaximum, std::move(_szThreadDescription));
            }

            HRESULT __YYAPI Wait::RunTask()
            {
                if (IsCanceled())
                    return YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED);

                if (pfnWaitTaskCallback(uWaitResult))
                {
                    if (auto _pOwnerTaskRunner = pOwnerTaskRunnerWeak.Get())
                    {
                        _pOwnerTaskRunner->SetWaitInternal(this);
                    }
                }
                return S_OK;
            }
        } // namespace Threading
    }
} // namespace YY
