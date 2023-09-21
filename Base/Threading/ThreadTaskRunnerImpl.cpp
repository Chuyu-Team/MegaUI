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
                // 线程已经开始销毁，不再创建
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
                // 同一个线程时直接调用 _pfnCallback，避免各种等待以及任务投递。
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

                        // uPushLock 占用1bit，所以 uWeakCount += 1 等价于 uWeakCountAndPushLock += 2
                        if (Sync::Subtract(&uWeakCountAndPushLock, 2u) < 2u)
                        {
                            // uWeakCount 已经归零，进入睡眠状态
                            WaitMessage();
                        }

                        // 消息循环本身因为处于激活状态，所以
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
                
                // 我们 解除锁定，uPushLock = 0 并且将 uWeakCount += 1
                // 因为刚才 uWeakCountAndPushLock 已经将第一个标记位设置位 1
                // 所以我们再 uWeakCountAndPushLock += 1即可。
                // uWeakCount + 1 <==> uWeakCountAndPushLock + 2 <==> (uWeakCountAndPushLock | 1) + 1
                if (Sync::Increment(&uWeakCountAndPushLock) == 2u)
                {
                    // 为 1 是说明当前正在等待输入消息，并且未主动唤醒
                    // 因此调用 PostAppMessageW 尝试唤醒目标线程的消息循环。
                    PostAppMessageW(uThreadId, WM_APP, 0, 0);
                }
            }
#endif
        } // namespace Threading
    } // namespace Base
} // namespace YY
