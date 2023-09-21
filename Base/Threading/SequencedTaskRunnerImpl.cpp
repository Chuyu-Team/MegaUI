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
                : uRef(1u)
                , uTaskRunnerId(GenerateNewTaskRunnerId())
                , uThreadId(0u)
                , uPushLock(0u)
                , uWeakCount(0u)
            {
                __YY_IGNORE_UNINITIALIZED_VARIABLE(&uWeakCountAndPushLock);
            }

            uint32_t __YYAPI SequencedTaskRunnerImpl::AddRef()
            {
                return Sync::Increment(&uRef);
            }

            uint32_t __YYAPI SequencedTaskRunnerImpl::Release()
            {
                auto _uOldRef = Sync::Decrement(&uRef);
                if (_uOldRef == 0)
                {
                    delete this;
                }

                return _uOldRef;
            }

            uint32_t __YYAPI SequencedTaskRunnerImpl::GetId()
            {
                return uTaskRunnerId;
            }

            TaskRunnerStyle __YYAPI SequencedTaskRunnerImpl::GetStyle()
            {
                return TaskRunnerStyle::Sequenced;
            }

            HRESULT __YYAPI SequencedTaskRunnerImpl::Async(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                auto _pWorkEntry = new (std::nothrow) ThreadWorkEntry(_pfnCallback, _pUserData, false);
                if (!_pWorkEntry)
                    return E_OUTOFMEMORY;

                PushThreadWorkEntry(_pWorkEntry);
                return S_OK;
            }

            HRESULT __YYAPI SequencedTaskRunnerImpl::Async(std::function<void()>&& _pfnLambdaCallback)
            {
                auto _pWorkEntry = new (std::nothrow) ThreadWorkEntry(std::move(_pfnLambdaCallback), false);
                if (!_pWorkEntry)
                    return E_OUTOFMEMORY;
                PushThreadWorkEntry(_pWorkEntry);
                return S_OK;
            }

            HRESULT __YYAPI SequencedTaskRunnerImpl::Sync(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                // ͬһ���߳�ʱֱ�ӵ��� _pfnCallback��������ֵȴ��Լ�����Ͷ�ݡ�
                if (GetCurrentThreadId() == uThreadId)
                {
                    _pfnCallback(_pUserData);
                    return S_OK;
                }

                ThreadWorkEntry _oWorkEntry(_pfnCallback, _pUserData, true);
                PushThreadWorkEntry(&_oWorkEntry);
                HRESULT _hrTarget = E_PENDING;
                WaitOnAddress(&_oWorkEntry.hr, &_hrTarget, sizeof(_hrTarget), uint32_max);
                return _oWorkEntry.hr;
            }

#ifdef _WIN32
            void __YYAPI SequencedTaskRunnerImpl::PushThreadWorkEntry(ThreadWorkEntry* _pWorkEntry)
            {
                for (;;)
                {
                    if (!Sync::BitSet(&uWeakCountAndPushLock, 0u))
                    {
                        oThreadWorkList.Push(_pWorkEntry);
                        break;
                    }
                }

                // ���� ���������uPushLock = 0 ���ҽ� uWeakCount += 1
                // ��Ϊ�ղ� uWeakCountAndPushLock �Ѿ�����һ�����λ����λ 1
                // ���������� uWeakCountAndPushLock += 1���ɡ�
                // uWeakCount + 1 <==> uWeakCountAndPushLock + 2 <==> (uWeakCountAndPushLock | 1) + 1
                if (Sync::Increment(&uWeakCountAndPushLock) == 2u)
                {
                    // Ϊ 1 ��˵����ǰ���ڵȴ�������Ϣ������δ��������
                    auto _bRet = TrySubmitThreadpoolCallback(
                        [](_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context) -> void
                        {
                            ((SequencedTaskRunnerImpl*)Context)->DoWorkList();
                        },
                        this,
                        nullptr);
                }
            }
#endif

            void __YYAPI SequencedTaskRunnerImpl::DoWorkList()
            {
                uThreadId = GetCurrentThreadId();

                for (;;)
                {
                    auto _pWorkEntry = oThreadWorkList.Pop();
                    if (_pWorkEntry)
                    {
                        _pWorkEntry->DoWorkThenFree();
                    }

                    if (Sync::Subtract(&uWeakCountAndPushLock, 2u) < 2u)
                    {
                        break;
                    }
                }
                uThreadId = 0;
                return;
            }
        }
    } // namespace Base
} // namespace YY
