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
            static thread_local ThreadTaskRunner* g_pThreadTaskRunner;

            ThreadTaskRunnerImpl::ThreadTaskRunnerImpl()
                : uRef(1)
                , uTaskRunnerId(GenerateNewTaskRunnerId())
                , uThreadId(GetCurrentThreadId())
                , uPushLock(0u)
                , uWeakCount(1u)
            {
                __YY_IGNORE_UNINITIALIZED_VARIABLE(&uWeakCountAndPushLock);
            }

            ThreadTaskRunnerImpl::~ThreadTaskRunnerImpl()
            {
                for (;;)
                {
                    auto _pWorkEntry = oThreadWorkList.Pop();
                    if (!_pWorkEntry)
                        break;

                    _pWorkEntry->hr = HRESULT_From_LSTATUS(ERROR_CANCELLED);
                    if (!HasFlags(_pWorkEntry->fStyle, ThreadWorkEntryStyle::Sync))
                    {
                        delete _pWorkEntry;
                    }
                }
            }
            
            RefPtr<ThreadTaskRunner> __YYAPI ThreadTaskRunnerImpl::GetCurrent()
            {
                auto _pThreadTaskRunner = g_pThreadTaskRunner;
                // �߳��Ѿ���ʼ���٣����ٴ���
                if (_pThreadTaskRunner == (ThreadTaskRunner*)-1)
                    return nullptr;

                if (_pThreadTaskRunner)
                {
                    return RefPtr<ThreadTaskRunner>(_pThreadTaskRunner);
                }

                RefPtr<ThreadTaskRunner> _pThreadTaskRunnerImpl = new (std::nothrow) ThreadTaskRunnerImpl();
                if (!_pThreadTaskRunnerImpl)
                    return nullptr;

                g_pThreadTaskRunner = _pThreadTaskRunnerImpl.Clone();
                return _pThreadTaskRunnerImpl;
            }

            uint32_t __YYAPI ThreadTaskRunnerImpl::AddRef()
            {
                return Sync::Increment(&uRef);
            }

            uint32_t __YYAPI ThreadTaskRunnerImpl::Release()
            {
                auto _uOldRef = Sync::Decrement(&uRef);
                if (_uOldRef == 0)
                {
                    delete this;
                }

                return _uOldRef;
            }

            uint32_t __YYAPI ThreadTaskRunnerImpl::GetId()
            {
                return uTaskRunnerId;
            }
            
            TaskRunnerStyle __YYAPI ThreadTaskRunnerImpl::GetStyle()
            {
                return TaskRunnerStyle::FixedThread | TaskRunnerStyle::Sequenced;
            }

            HRESULT __YYAPI ThreadTaskRunnerImpl::Async(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
            {
                auto _pWorkEntry = new (std::nothrow) ThreadWorkEntry(_pfnCallback, _pUserData, false);
                if (!_pWorkEntry)
                    return E_OUTOFMEMORY;

                PushThreadWorkEntry(_pWorkEntry);
                return S_OK;
            }

            HRESULT __YYAPI ThreadTaskRunnerImpl::Async(std::function<void()>&& _pfnLambdaCallback)
            {
                auto _pWorkEntry = new (std::nothrow) ThreadWorkEntry(std::move(_pfnLambdaCallback), false);
                if (!_pWorkEntry)
                    return E_OUTOFMEMORY;
                PushThreadWorkEntry(_pWorkEntry);
                return S_OK;
            }
            
            HRESULT __YYAPI ThreadTaskRunnerImpl::Sync(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData)
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
            
            uint32_t __YYAPI ThreadTaskRunnerImpl::GetThreadId()
            {
                return uThreadId;
            }
            
#ifdef _WIN32
            uintptr_t __YYAPI ThreadTaskRunnerImpl::RunMessageLoop()
            {
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
                            _pWorkEntry->DoWorkThenFree();
                        }

                        // uPushLock ռ��1bit������ uWeakCount += 1 �ȼ��� uWeakCountAndPushLock += 2
                        if (Sync::Subtract(&uWeakCountAndPushLock, 2u) < 2u)
                        {
                            // uWeakCount �Ѿ����㣬����˯��״̬
                            WaitMessage();
                        }

                        // ��Ϣѭ��������Ϊ���ڼ���״̬������
                        Sync::Add(&uWeakCountAndPushLock, 2u);
                    }
                }

                return _oMsg.wParam;
            }
#endif

#ifdef _WIN32
            void __YYAPI ThreadTaskRunnerImpl::PushThreadWorkEntry(ThreadWorkEntry* _pWorkEntry)
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
                    // ��˵��� PostAppMessageW ���Ի���Ŀ���̵߳���Ϣѭ����
                    PostAppMessageW(uThreadId, WM_APP, 0, 0);
                }
            }
#endif
        } // namespace Threading
    } // namespace Base
} // namespace YY
