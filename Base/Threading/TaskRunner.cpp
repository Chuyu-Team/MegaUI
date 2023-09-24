#include "pch.h"
#include "TaskRunner.h"

#include <Base/Exception.h>
#include <Base/Threading/ThreadTaskRunnerImpl.h>
#include <Base/Threading/SequencedTaskRunnerImpl.h>
#include <Base/Sync/Sync.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            struct SimpleWorkType
            {
                TaskRunnerSimpleCallback pfnCallback;
                void* pUserData;

                void operator()() const
                {
                    pfnCallback(pUserData);
                }
            };

            using SimpleWorkThreadWorkEntry = ThreadWorkEntryImpl<SimpleWorkType>;

            ThreadWorkEntry::ThreadWorkEntry(ThreadWorkEntryStyle _eStyle)
                : uRef(1u)
                , fStyle(_eStyle)
                , hr(E_PENDING)
            {
            }

            uint32_t ThreadWorkEntry::AddRef()
            {
                return Sync::Increment(&uRef);
            }

            uint32_t ThreadWorkEntry::Release()
            {
                const auto _uNewRef = Sync::Decrement(&uRef);
                if (_uNewRef == 0)
                {
                    delete this;
                }

                return _uNewRef;
            }

            void __YYAPI ThreadWorkEntry::Wakeup(HRESULT _hrCode)
            {
                hr = _hrCode;
                if (HasFlags(fStyle, ThreadWorkEntryStyle::Sync))
                {
                    WakeByAddressAll(&hr);
                }
            }

            bool __YYAPI ThreadWorkEntry::Wait(uint32_t _uMilliseconds)
            {
                HRESULT _hrTarget = E_PENDING;
                return WaitOnAddress(&hr, &_hrTarget, sizeof(_hrTarget), _uMilliseconds);
            }

            RefPtr<SequencedTaskRunner> __YYAPI SequencedTaskRunner::GetCurrent()
            {
                if (!g_pTaskRunner)
                    return nullptr;

                return RefPtr<SequencedTaskRunner>(g_pTaskRunner);
            }

            RefPtr<SequencedTaskRunner> __YYAPI SequencedTaskRunner::Create()
            {
                return RefPtr<SequencedTaskRunnerImpl>::Create();
            }

            HRESULT __YYAPI SequencedTaskRunner::PostTask(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                auto _pThreadWorkEntry = RefPtr<SimpleWorkThreadWorkEntry>::Create(ThreadWorkEntryStyle::None, _pfnCallback, _pUserData);
                if (!_pThreadWorkEntry)
                    return E_OUTOFMEMORY;

                return PushThreadWorkEntry(_pThreadWorkEntry);
            }

            HRESULT __YYAPI SequencedTaskRunner::SendTask(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                // 调用者的跟执行者属于同一个TaskRunner，这时我们直接调用 _pfnCallback，避免各种等待以及任务投递开销。
                if (SequencedTaskRunner::GetCurrent() == this)
                {
                    _pfnCallback(_pUserData);
                    return S_OK;
                }

                SimpleWorkThreadWorkEntry _oWorkEntry(ThreadWorkEntryStyle::Sync, _pfnCallback, _pUserData);
                auto _hr = PushThreadWorkEntry(&_oWorkEntry);            
                if (FAILED(_hr))
                {
                    return _hr;
                }
                _oWorkEntry.Wait();
                return _oWorkEntry.hr;
            }

            
            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunner::GetCurrent()
            {
                // 尚未初始化，没有运行 RunUIMessageLoop 或者其他。
                if (!g_pTaskRunner)
                    return nullptr;

                // 当前 TaskRunner 必须是物理线程。
                if (!HasFlags(g_pTaskRunner->GetStyle(), TaskRunnerStyle ::FixedThread))
                {
                    return nullptr;
                }
                return RefPtr<ThreadTaskRunner>((ThreadTaskRunner*)g_pTaskRunner);
            }

            uintptr_t __YYAPI ThreadTaskRunner::RunUIMessageLoop(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                RefPtr<ThreadTaskRunnerImpl> _pThreadTaskRunnerImpl;
                if (g_pTaskRunner)
                {
                    if (!HasFlags(g_pTaskRunner->GetStyle(), TaskRunnerStyle::FixedThread))
                    {
                        // 非物理线程不能进行UI消息循环！！！
                        // 暂时设计如此，未来再说。
                        throw Exception(E_INVALIDARG);
                    }

                    _pThreadTaskRunnerImpl = static_cast<ThreadTaskRunnerImpl*>(g_pTaskRunner);
                }
                else
                {
                    // 这是主线程？？？
                    _pThreadTaskRunnerImpl = RefPtr<ThreadTaskRunnerImpl>::Create();
                    if (!_pThreadTaskRunnerImpl)
                    {
                        return (uintptr_t)E_OUTOFMEMORY;
                    }
                }

                return _pThreadTaskRunnerImpl->RunUIMessageLoop(_pfnCallback, _pUserData);
            }
        } // namespace Threading
    }
} // namespace YY
