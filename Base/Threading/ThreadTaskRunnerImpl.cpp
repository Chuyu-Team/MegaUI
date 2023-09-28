#include "pch.h"
#include "ThreadTaskRunnerImpl.h"

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
            // RunUIMessageLoop ����Ĵ�����
            // ���� ThreadTaskRunner ��˵���뱣֤��Ϣѭ���ڲ�������������
            static thread_local uint32_t s_uUIMessageLoopEnterCount = 0u;

            ThreadTaskRunnerImpl::ThreadTaskRunnerImpl()
                : uTaskRunnerId(GenerateNewTaskRunnerId())
                , uThreadId(GetCurrentThreadId())
                , uWeakupCountAndPushLock(WeakupOnceRaw)
            {
            }

            ThreadTaskRunnerImpl::~ThreadTaskRunnerImpl()
            {
            }

            uint32_t __YYAPI ThreadTaskRunnerImpl::GetId()
            {
                return uTaskRunnerId;
            }
            
            TaskRunnerStyle __YYAPI ThreadTaskRunnerImpl::GetStyle()
            {
                return TaskRunnerStyle::FixedThread;
            }
            
            uint32_t __YYAPI ThreadTaskRunnerImpl::GetThreadId()
            {
                return uThreadId;
            }
            
#ifdef _WIN32
            uintptr_t __YYAPI ThreadTaskRunnerImpl::RunUIMessageLoop(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                ++s_uUIMessageLoopEnterCount;
                if (s_uUIMessageLoopEnterCount == 1)
                {
                    g_pTaskRunner = this;
                    EnableWeakup(true);
                }

                if (_pfnCallback)
                    _pfnCallback(_pUserData);


                MSG _oMsg;
                for (;;)
                {
                    const auto _bRet = PeekMessageW(&_oMsg, NULL, 0, 0, PM_REMOVE);
                    if (_bRet == -1)
                        continue;

                    if (_bRet)
                    {
                        if (_oMsg.message == WM_QUIT)
                        {
                            break;
                        }
                        TranslateMessage(&_oMsg);
                        DispatchMessage(&_oMsg);
                    }
                    else
                    {
                        auto _pWorkEntry = oThreadWorkList.Pop();
                        if (_pWorkEntry)
                        {
                            (*_pWorkEntry)();
                            _pWorkEntry->Wakeup(S_OK);
                            _pWorkEntry->Release();
                        }

                        // uPushLock ռ��1bit������ uWeakCount += 1 �ȼ��� uWeakCountAndPushLock += 2
                        if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                        {
                            // uWeakCount �Ѿ����㣬����˯��״̬
                            WaitMessage();
                        }

                        // ��Ϣѭ��������Ϊ���ڼ���״̬������
                        Sync::Add(&uWeakupCountAndPushLock, WeakupOnceRaw);
                    }
                }

                --s_uUIMessageLoopEnterCount;
                if (s_uUIMessageLoopEnterCount == 0)
                {
                    g_pTaskRunner = nullptr;
                    EnableWeakup(false);
                }
                return _oMsg.wParam;
            }

            void __YYAPI ThreadTaskRunnerImpl::EnableWeakup(bool _bEnable)
            {
                if (_bEnable)
                {
                    Sync::BitReset(&uWeakupCountAndPushLock, StopWeakupBitIndex);
                }
                else
                {
                    Sync::BitSet(&uWeakupCountAndPushLock, StopWeakupBitIndex);
                    CleanupWorkEntryQueue();
                }
            }
#endif

#ifdef _WIN32
            HRESULT __YYAPI ThreadTaskRunnerImpl::PushThreadWorkEntry(RefPtr<ThreadWorkEntry> _pWorkEntry)
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

                // ���� ���������uPushLock = 0 ���ҽ� uWeakCount += 1
                // ��Ϊ�ղ� uWeakCountAndPushLock �Ѿ�����һ�����λ����λ 1
                // ���������� uWeakCountAndPushLock += 1���ɡ�
                // uWeakCount + 1 <==> uWeakCountAndPushLock + 2 <==> (uWeakCountAndPushLock | 1) + 1
                if (Sync::Add(&uWeakupCountAndPushLock, UnlockQueuePushLockBitAndWeakupOnceRaw) < WeakupOnceRaw * 2u)
                {
                    // Ϊ 1 ��˵����ǰ���ڵȴ�������Ϣ������δ��������
                    // ��˵��� PostAppMessageW ���Ի���Ŀ���̵߳���Ϣѭ����
                    auto _bRet = PostAppMessageW(uThreadId, WM_APP, 0, 0);

                    // Post ʧ�ܴ�����ʱ�������������ǵ�ǰϵͳ��Դ���㣬��Ȼ�Ѿ������˶��������������ɡ�
                }
                return S_OK;
            }

            void __YYAPI ThreadTaskRunnerImpl::CleanupWorkEntryQueue()
            {
                AddRef();
                for (;;)
                {
                    auto _pWorkEntry = oThreadWorkList.Pop();
                    if (!_pWorkEntry)
                        break;
                    _pWorkEntry->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                    _pWorkEntry->Release();
                    Release();

                    if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                        break;
                }
                Release();
            }
#endif
        } // namespace Threading
    } // namespace Base
} // namespace YY
