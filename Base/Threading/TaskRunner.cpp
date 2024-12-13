#include <Base/Threading/TaskRunner.h>

#include <Base/Exception.h>
#include <Base/Threading/ThreadTaskRunnerImpl.h>
#include <Base/Threading/SequencedTaskRunnerImpl.h>
#include <Base/Threading/ParallelTaskRunnerImpl.hpp>
#include <Base/Sync/Sync.h>
#include <Base/Threading/TaskRunnerDispatchImpl.h>

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
                if (uInterval.GetMilliseconds() <= 0)
                {
                    return TaskEntry::RunTask();
                }
                else
                {
                    auto _uExpire = TickCount<TimePrecise::Microsecond>::GetCurrent() + uInterval;
                    auto _hr = TaskEntry::RunTask();
                    if (FAILED(_hr))
                        return _hr;

                    auto _pOwnerTaskRunner = pOwnerTaskRunnerWeak.Get();
                    // 任务被取消？
                    if (!_pOwnerTaskRunner)
                        return S_OK;

                    uExpire = _uExpire;
                    return _pOwnerTaskRunner->SetTimerInternal(this);
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
                RefPtr<ThreadTaskRunnerImpl> _pThreadTaskRunnerImpl;

                if (auto _pTaskRunner = g_pTaskRunnerWeak.Get())
                {
                    if (!HasFlags(_pTaskRunner->GetStyle(), TaskRunnerStyle::FixedThread))
                    {
                        // 非物理线程不能进行UI消息循环！！！
                        // 暂时设计如此，未来再说。
                        return nullptr;
                    }

                    _pThreadTaskRunnerImpl = std::move(_pTaskRunner);
                }
                else
                {
                    // 这是主线程？？？
                    _pThreadTaskRunnerImpl = RefPtr<ThreadTaskRunnerImpl>::Create();
                    if (!_pThreadTaskRunnerImpl)
                    {
                        return nullptr;
                    }
                    g_pTaskRunnerWeak = _pThreadTaskRunnerImpl;
                }

                return _pThreadTaskRunnerImpl;
            }

            uintptr_t __YYAPI ThreadTaskRunner::RunUIMessageLoop()
            {
                RefPtr<ThreadTaskRunnerImpl> _pThreadTaskRunnerImpl;

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

                auto _uResult = _pThreadTaskRunnerImpl->RunUIMessageLoop();
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
        }
    }
} // namespace YY
