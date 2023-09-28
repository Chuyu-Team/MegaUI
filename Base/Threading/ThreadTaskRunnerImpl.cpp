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
            // RunUIMessageLoop 进入的次数。
            // 对于 ThreadTaskRunner 来说必须保证消息循环在才能正常工作。
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

                        // uPushLock 占用1bit，所以 uWeakCount += 1 等价于 uWeakCountAndPushLock += 2
                        if (Sync::Subtract(&uWeakupCountAndPushLock, WeakupOnceRaw) < WeakupOnceRaw)
                        {
                            // uWeakCount 已经归零，进入睡眠状态
                            WaitMessage();
                        }

                        // 消息循环本身因为处于激活状态，所以
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

                // 我们 解除锁定，uPushLock = 0 并且将 uWeakCount += 1
                // 因为刚才 uWeakCountAndPushLock 已经将第一个标记位设置位 1
                // 所以我们再 uWeakCountAndPushLock += 1即可。
                // uWeakCount + 1 <==> uWeakCountAndPushLock + 2 <==> (uWeakCountAndPushLock | 1) + 1
                if (Sync::Add(&uWeakupCountAndPushLock, UnlockQueuePushLockBitAndWeakupOnceRaw) < WeakupOnceRaw * 2u)
                {
                    // 为 1 是说明当前正在等待输入消息，并且未主动唤醒
                    // 因此调用 PostAppMessageW 尝试唤醒目标线程的消息循环。
                    auto _bRet = PostAppMessageW(uThreadId, WM_APP, 0, 0);

                    // Post 失败处理，暂时不做处理，可能是当前系统资源不足，既然已经加入了队列我们先这样吧。
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
