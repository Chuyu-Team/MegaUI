#pragma once
#include <Base/YY.h>
#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Containers/BitMap.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class ThreadPoolWaitManger
            {
            protected:
                static constexpr auto kMaxHandle = MAXIMUM_WAIT_OBJECTS - 1;
                InterlockedQueue<Wait> oPendingWaitQueue;

                HANDLE hWaitHandles[MAXIMUM_WAIT_OBJECTS] = {};
                Wait* pWaitTaskList[MAXIMUM_WAIT_OBJECTS] = {};
                DWORD cWaitHandle = 0;

                void __YYAPI SetWaitInternalNolock(_In_ RefPtr<Wait> _pTask) noexcept
                {
                    if (_pTask)
                        oPendingWaitQueue.Push(_pTask.Detach());
                }

                void __YYAPI ProcessingPendingWaitQueue() noexcept
                {
                    for (;;)
                    {
                        auto _pWaitTask = oPendingWaitQueue.Pop();
                        if (!_pWaitTask)
                            break;

                        AddWaitTask(_pWaitTask);
                    }
                }

                void __YYAPI AddWaitTask(Wait* _pWaitTask) noexcept
                {
                    size_t i = 0;
                    for (; i != cWaitHandle; ++i)
                    {
                        if (hWaitHandles[i] == _pWaitTask->hHandle)
                        {
                            break;
                        }
                    }

                    if (i >= kMaxHandle)
                    {
                        DispatchWaitTask(_pWaitTask, WAIT_FAILED);
                    }
                    else
                    {
                        if (i == cWaitHandle)
                            ++cWaitHandle;

                        hWaitHandles[i] = _pWaitTask->hHandle;
                        _pWaitTask->pNext = pWaitTaskList[i];
                        pWaitTaskList[i] = _pWaitTask;
                    }
                }


                virtual void __YYAPI DispatchWaitTask(Wait* _pWaitTask, DWORD _uWaitResult) = 0;

                void __YYAPI DispatchWaitList(size_t _uDispatchIndex, DWORD _uWaitResult)
                {
                    if (cWaitHandle <= _uDispatchIndex)
                        return;

                    auto _pWaitTask = pWaitTaskList[_uDispatchIndex];
                    pWaitTaskList[_uDispatchIndex] = nullptr;

                    for (; _pWaitTask;)
                    {
                        auto _pNext = _pWaitTask->pNext;
                        _pWaitTask->pNext = nullptr;
                        DispatchWaitTask(_pWaitTask, _uWaitResult);
                        _pWaitTask = _pNext;
                    }

                    // 快速删除，因为Wait列表不要求有序，直接与最后的元素进行交换就能做到删除。
                    --cWaitHandle;
                    if (cWaitHandle != _uDispatchIndex)
                    {
                        hWaitHandles[_uDispatchIndex] = hWaitHandles[cWaitHandle];
                        pWaitTaskList[_uDispatchIndex] = pWaitTaskList[cWaitHandle];
                    }

                    hWaitHandles[cWaitHandle] = nullptr;
                    pWaitTaskList[cWaitHandle] = nullptr;
                }

                void __YYAPI ProcessingWaitResult(DWORD uWaitResult)
                {
                    if (WAIT_OBJECT_0 <= uWaitResult && uWaitResult < WAIT_OBJECT_0 + cWaitHandle)
                    {
                        DispatchWaitList(uWaitResult - WAIT_OBJECT_0, WAIT_OBJECT_0);
                    }
                    else if (WAIT_ABANDONED_0 <= uWaitResult && uWaitResult < WAIT_ABANDONED_0 + cWaitHandle)
                    {
                        DispatchWaitList(uWaitResult - WAIT_ABANDONED_0, WAIT_ABANDONED_0);
                    }
                    else if (WAIT_FAILED == uWaitResult)
                    {
                        // 有句柄可能无效，检测一下无效句柄……然后报告
                        for (DWORD _uIndex = 0; _uIndex < cWaitHandle; )
                        {
                            const auto _uTaskWaitResult = WaitForSingleObject(hWaitHandles[_uIndex], 0);
                            if (_uTaskWaitResult == WAIT_TIMEOUT)
                            {
                                ++_uIndex;
                            }
                            else
                            {
                                DispatchWaitList(_uIndex, _uTaskWaitResult);
                            }
                        }
                    }
                }
            };
        }
    }
}
#pragma pack(pop)
