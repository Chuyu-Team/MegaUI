#include "pch.h"
#include "SequencedTaskRunnerImpl.h"

#include <MegaUI/Base/ErrorCode.h>
#include <Base/Sync/Sync.h>
#include <Base/Threading/ProcessThreads.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            SequencedTaskRunnerImpl::SequencedTaskRunnerImpl()
                : uTaskRunnerId(GenerateNewTaskRunnerId())
                , uThreadId(0u)
                , uWeakupCountAndPushLock(0u)
            {
            }

            uint32_t __YYAPI SequencedTaskRunnerImpl::GetId()
            {
                return uTaskRunnerId;
            }

            TaskRunnerStyle __YYAPI SequencedTaskRunnerImpl::GetStyle()
            {
                return TaskRunnerStyle::None;
            }

#ifdef _WIN32
            HRESULT __YYAPI SequencedTaskRunnerImpl::PushThreadWorkEntry(RefPtr<ThreadWorkEntry> _pWorkEntry)
            {
                if (bStopWeakup)
                {
                    _pWorkEntry->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                    return E_FAIL;
                }

                AddRef();

                for (;;)
                {
                    if (!Sync::BitSet(&uWeakupCountAndPushLock, LockedQueuePushBitIndex))
                    {
                        oThreadWorkList.Push(_pWorkEntry.Detach());
                        break;
                    }
                }

                // 我们 解除锁定，uPushLock = 0 并且将 uWeakCount += 1
                // 因为刚才 uWeakupCountAndPushLock 已经将第一个标记位设置位 1
                // 所以我们再 uWeakupCountAndPushLock += 1即可。
                // uWeakCount + 1 <==> uWeakupCountAndPushLock + 2 <==> (uWeakupCountAndPushLock | 1) + 1
                if (Sync::Add(&uWeakupCountAndPushLock, UnlockQueuePushLockBitAndWeakupOnceRaw) < WeakupOnceRaw * 2u)
                {
                    auto _bRet = TrySubmitThreadpoolCallback(
                        [](_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context) -> void
                        {
                            auto pSequencedTaskRunner = ((SequencedTaskRunnerImpl*)Context);
                            
                            pSequencedTaskRunner->DoWorkList();
                        },
                        this,
                        nullptr);

                    if (!_bRet)
                    {
                        // 阻止后续再唤醒线程
                        Sync::BitSet(&uWeakupCountAndPushLock, StopWeakupBitIndex);
                        auto _hr = YY::Base::HRESULT_From_LSTATUS(GetLastError());
                        CleanupWorkEntryQueue();
                        return _hr;
                    }
                }

                return S_OK;
            }
#endif

            void __YYAPI SequencedTaskRunnerImpl::DoWorkList()
            {
                AddRef();

                uThreadId = GetCurrentThreadId();
                g_pTaskRunner = this;

                for (;;)
                {
                    auto _pWorkEntry = oThreadWorkList.Pop();
                    if (_pWorkEntry)
                    {
                        (*_pWorkEntry)();
                        _pWorkEntry->Wakeup(S_OK);
                        _pWorkEntry->Release();
                        Release();
                    }

                    if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                    {
                        break;
                    }
                }
                g_pTaskRunner = nullptr;
                uThreadId = 0;
                Release();
                return;
            }
            
            void __YYAPI SequencedTaskRunnerImpl::CleanupWorkEntryQueue()
            {
                AddRef();

                for (;;)
                {
                    auto _pWorkEntry = oThreadWorkList.Pop();
                    if (_pWorkEntry)
                    {
                        _pWorkEntry->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                        _pWorkEntry->Release();
                        Release();
                    }

                    if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                    {
                        break;
                    }
                }
                Release();
                return;
            }
        }
    } // namespace Base
} // namespace YY
