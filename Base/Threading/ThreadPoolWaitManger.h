#pragma once
#include <Base/YY.h>
#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Containers/BitMap.h>
#include <Base/Time/TickCount.h>
#include <Base/Sync/SRWLock.h>
#include <Base/Memory/Alloc.h>
#include <Base/Memory/UniquePtr.h>
#include <Base/Sync/AutoLock.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            template<typename ItemType>
            class List
            {
            private:
                ItemType* pFirst = nullptr;
                ItemType* pLast = nullptr;
                
            public:
                void __YYAPI Clear() noexcept
                {
                    pFirst = nullptr;
                    pLast = nullptr;
                }

                List __YYAPI Flush() noexcept
                {
                    List _oList;
                    _oList.pFirst = pFirst;
                    _oList.pLast = pLast;
                    Clear();
                    return _oList;
                }

                void __YYAPI PushBack(_In_ ItemType* _pEntry) noexcept
                {
                    _pEntry->pPrior = pLast;
                    _pEntry->pNext = nullptr;

                    if (pLast)
                    {
                        pLast->pNext = _pEntry;
                        pLast = _pEntry;
                    }
                    else
                    {
                        pFirst = pLast = _pEntry;           
                    }
                }

                _Ret_maybenull_ ItemType* __YYAPI PopBack() noexcept
                {
                    auto _pRet = pLast;
                    if (_pRet)
                    {
                        pLast = pLast->pPrior;
                        if (pLast)
                        {
                            pLast->pNext = nullptr;
                        }
                        else
                        {
                            pFirst = nullptr;
                        }

                        _pRet->pPrior = nullptr;
                    }

                    return _pRet;
                }

                _Ret_maybenull_ ItemType* __YYAPI PopFront() noexcept
                {
                    auto _pRet = pFirst;
                    if (_pRet)
                    {
                        pFirst = pFirst->pNext;
                        if (pFirst)
                        {
                            pFirst->pPrior = nullptr;
                        }
                        else
                        {
                            pLast = nullptr;
                        }
                        _pRet->pNext = nullptr;
                    }

                    return _pRet;
                }

                void __YYAPI Insert(_In_ ItemType* _pEntry) noexcept
                {
                    _pEntry->pPrior = nullptr;
                    _pEntry->pNext = pFirst;
                    if (pFirst)
                    {
                        pFirst->pPrior = _pEntry;
                        pFirst = _pEntry;
                    }
                    else
                    {
                        pFirst = pLast = _pEntry;
                    }
                }

                _Ret_maybenull_ ItemType* __YYAPI GetFirst() noexcept
                {
                    return pFirst;
                }

                void __YYAPI Remove(_In_ ItemType* _pEntry) noexcept
                {
                    auto _pPrior = _pEntry->pPrior;
                    _pEntry->pPrior = nullptr;
                    auto _pNext = _pEntry->pNext;
                    _pEntry->pNext = nullptr;

                    if (_pPrior)
                    {
                        _pPrior->pNext = _pNext;
                    }
                    else
                    {
                        assert(pFirst == _pEntry);
                        pFirst = _pNext;
                    }

                    if (_pNext)
                    {
                        _pNext->pPrior = _pPrior;
                    }
                    else
                    {
                        assert(pLast == _pEntry);
                        pLast = _pPrior;
                    }
                }

                bool __YYAPI IsEmpty() const noexcept
                {
                    return pFirst == nullptr;
                }
            };

            struct WaitTaskList : public List<Wait>
            {
            };

            /// <summary>
            /// 单线程的Wait Manger实现，最多等待63个句柄。
            /// </summary>
            class ThreadPoolWaitMangerForSingleThreading
            {
            private:
                struct WaitHandleBlock
                {
                    static constexpr auto kMaxWaitHandleCount = MAXIMUM_WAIT_OBJECTS - 1;
                    HANDLE hWaitHandles[MAXIMUM_WAIT_OBJECTS] = {};
                    WaitTaskList oWaitTaskLists[MAXIMUM_WAIT_OBJECTS] = {};
                    volatile DWORD cWaitHandle = 0;
                };

            protected:
                WaitHandleBlock oDefaultWaitBlock;

                ~ThreadPoolWaitMangerForSingleThreading()
                {
                    for (auto& _oWaitTaskList : oDefaultWaitBlock.oWaitTaskLists)
                    {
                        while (auto _pTask = _oWaitTaskList.PopFront())
                        {
                            _pTask->Cancel();
                            _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                            _pTask->Release();
                        }
                    }
                }

                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pWaitTask) noexcept
                {
                    if (!_pWaitTask)
                        return E_INVALIDARG;

                    if (!_pWaitTask->hHandle)
                        return E_INVALIDARG;
                    
                    size_t i = 0;
                    for (; i != oDefaultWaitBlock.cWaitHandle; ++i)
                    {
                        if (oDefaultWaitBlock.hWaitHandles[i] == _pWaitTask->hHandle)
                        {
                            break;
                        }
                    }

                    if (i >= oDefaultWaitBlock.kMaxWaitHandleCount)
                    {
                        // 尚未实现
                        return E_NOTIMPL;
                    }
                    else
                    {
                        if (i == oDefaultWaitBlock.cWaitHandle)
                        {
                            ++oDefaultWaitBlock.cWaitHandle;
                            oDefaultWaitBlock.hWaitHandles[i] = _pWaitTask->hHandle;
                        }
                        oDefaultWaitBlock.oWaitTaskLists[i].PushBack(_pWaitTask.Detach());
                        return S_OK;
                    }
                }

                size_t __YYAPI ProcessingWaitTasks(DWORD uWaitResult, DWORD _cWaitHandle, DWORD _uTaskRunnerServerHandleIndex) noexcept
                {
                    size_t _cTaskProcessed = 0;
                    if (WAIT_OBJECT_0 <= uWaitResult && uWaitResult < WAIT_OBJECT_0 + _cWaitHandle)
                    {
                        uWaitResult -= WAIT_OBJECT_0;
                        if (uWaitResult != _uTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(uWaitResult, WAIT_OBJECT_0);
                        }
                    }
                    else if (WAIT_ABANDONED_0 <= uWaitResult && uWaitResult < WAIT_ABANDONED_0 + _cWaitHandle)
                    {
                        uWaitResult -= WAIT_ABANDONED_0;
                        if (uWaitResult != _uTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(uWaitResult, WAIT_ABANDONED_0);
                        }
                    }
                    else if (WAIT_IO_COMPLETION == uWaitResult)
                    {
                        // APC被执行，不是存在信号
                    }
                    else/* if (WAIT_FAILED == uWaitResult)*/
                    {
                        // 有句柄可能无效，检测一下无效句柄……然后报告
                        // 这里需要从后向前移动，避免 DispatchWaitList 后 cWaitHandle不停的减小
                        if (_cWaitHandle)
                        {
                            do
                            {
                                --_cWaitHandle;
                                if (_cWaitHandle != _uTaskRunnerServerHandleIndex)
                                {
                                    const auto _uTaskWaitResult = WaitForSingleObject(oDefaultWaitBlock.hWaitHandles[_cWaitHandle], 0);
                                    if (_uTaskWaitResult != WAIT_TIMEOUT)
                                    {
                                        _cTaskProcessed += DispatchWaitList(_cWaitHandle, _uTaskWaitResult);
                                    }
                                }
                            } while (_cWaitHandle);
                        }
                    }

                    return _cTaskProcessed;
                }

            private:
                virtual void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) = 0;

                size_t __YYAPI DispatchWaitList(size_t _uDispatchIndex, DWORD _uWaitResult) noexcept
                {
                    HANDLE _hWaitEvent = oDefaultWaitBlock.hWaitHandles[_uDispatchIndex];
                    auto _oWaitTaskList = oDefaultWaitBlock.oWaitTaskLists[_uDispatchIndex].Flush();
                    // 快速删除，因为句柄不要求有序，直接与最后的元素进行交换就能做到删除。
                    --oDefaultWaitBlock.cWaitHandle;
                    if (oDefaultWaitBlock.cWaitHandle != _uDispatchIndex)
                    {
                        oDefaultWaitBlock.hWaitHandles[_uDispatchIndex] = oDefaultWaitBlock.hWaitHandles[oDefaultWaitBlock.cWaitHandle];
                        oDefaultWaitBlock.oWaitTaskLists[_uDispatchIndex] = oDefaultWaitBlock.oWaitTaskLists[oDefaultWaitBlock.cWaitHandle];
                    }
                    oDefaultWaitBlock.hWaitHandles[oDefaultWaitBlock.cWaitHandle] = nullptr;
                    oDefaultWaitBlock.oWaitTaskLists[oDefaultWaitBlock.cWaitHandle].Flush();

                    assert(_hWaitEvent);
                    if (_hWaitEvent == nullptr)
                    {
                        return 0;
                    }

                    size_t _cTaskProcessed = 0;
                    while (auto _pWaitTask = _oWaitTaskList.PopFront())
                    {
                        _pWaitTask->uWaitResult = _uWaitResult;
                        DispatchWaitTask(RefPtr<Wait>::FromPtr(_pWaitTask));
                        ++_cTaskProcessed;
                    }

                    return _cTaskProcessed;
                }
            };

            /// <summary>
            /// 多线程的Wait Manger实现，等待句柄数量没有限制。Manger内部会按需启动线程。每个线程最多等待63个句柄。
            /// </summary>
            class ThreadPoolWaitMangerForMultiThreading
            {
            protected:
                struct WaitHandleBlock;

                struct WaitHandleEntry
                {
                    WaitHandleEntry* pPrior = nullptr;
                    WaitHandleEntry* pNext = nullptr;
                    HANDLE hWaitHandle = nullptr;
                    WaitHandleBlock* pOwnerWaitHandleBlock = nullptr;
                    WaitTaskList oWaitTaskLists;
                };

                struct WaitHandleBlock
                {
                    WaitHandleBlock* pPrior = nullptr;
                    WaitHandleBlock* pNext = nullptr;

                    static constexpr auto kMaxWaitHandleCount = MAXIMUM_WAIT_OBJECTS - 1;
                    static constexpr auto kTaskRunnerServerHandleIndex = 0;

                    union
                    {
                        HANDLE hTaskRunnerServerHandle = nullptr;
                        HANDLE hWaitHandles[MAXIMUM_WAIT_OBJECTS];
                    };

                    WaitHandleEntry* oWaitHandleEntries[MAXIMUM_WAIT_OBJECTS] = {};
                    volatile DWORD cWaitHandle = 1;

                    SRWLock oLock;
                    // 执行等待任务的异步线程
                    RefPtr<SequencedTaskRunner> pWaitTaskRunner;
                    volatile bool bTerminate = false;

                    WaitHandleBlock(HANDLE _hTaskRunnerServerHandle)
                        : hTaskRunnerServerHandle(_hTaskRunnerServerHandle)
                    {
                    }

                    WaitHandleBlock()
                        : hTaskRunnerServerHandle(CreateEventW(nullptr, FALSE, FALSE, nullptr))
                        , pWaitTaskRunner(SequencedTaskRunner::Create(_S("Wait扩展调度线程")))
                    {
                    }

                    ~WaitHandleBlock()
                    {
                        if (hTaskRunnerServerHandle)
                            CloseHandle(hTaskRunnerServerHandle);
                    }

                    /// <summary>
                    /// 
                    /// </summary>
                    /// <param name="_pWaitHandleEntry"></param>
                    /// <returns>表示WaitHandleBlock内是否是第一个任务。</returns>
                    bool __YYAPI AddWaitHandleEntryNolock(_In_ WaitHandleEntry* _pWaitHandleEntry) noexcept
                    {
                        if (IsFull())
                        {
                            // throw Exception(_S("WaitHandleBlock Is Full!"), E_INVALIDARG);
                            assert(false);
                            return false;
                        }
                        else
                        {
                            hWaitHandles[cWaitHandle] = _pWaitHandleEntry->hWaitHandle;
                            oWaitHandleEntries[cWaitHandle] = _pWaitHandleEntry;
                            ++cWaitHandle;
                            return cWaitHandle == 2;
                        }
                    }

                    bool __YYAPI IsFull() const noexcept
                    {
                        return cWaitHandle >= kMaxWaitHandleCount;
                    }
                };

                struct WaitHandleHashList : public List<WaitHandleEntry>
                {
                    SRWLock oLock;
                    
                    WaitHandleEntry* __YYAPI FindWaitItem(HANDLE _hWaitHandle) noexcept
                    {
                        for (auto _pItem = GetFirst(); _pItem; _pItem = _pItem->pNext)
                        {
                            if (_pItem->hWaitHandle == _hWaitHandle)
                                return _pItem;
                        }

                        return nullptr;
                    }
                };

                WaitHandleHashList arrWaitHandleHashTable[128] = {};
                
                // 当前服务线程提供的 WaitHandleBlock，充分利用服务线程进行等待，减少线程启动。
                WaitHandleBlock oDefaultWaitBlock;
                List<WaitHandleBlock> oWaitBlockTaskRunnerList;

                ThreadPoolWaitMangerForMultiThreading(HANDLE _hTaskRunnerServerHandle)
                    : oDefaultWaitBlock(_hTaskRunnerServerHandle)
                {
                }

                ~ThreadPoolWaitMangerForMultiThreading()
                {
                    for (auto _pItem = oWaitBlockTaskRunnerList.GetFirst(); _pItem; _pItem = _pItem->pNext)
                    {
                        _pItem->bTerminate = true;
                        SetEvent(_pItem->hTaskRunnerServerHandle);
                    }

                    while (auto _pItem = oWaitBlockTaskRunnerList.PopFront())
                    {
                        Delete(_pItem);
                    }

                    for (auto& _oHashList : arrWaitHandleHashTable)
                    {
                        while (auto _pItem = _oHashList.PopFront())
                        {
                            Delete(_pItem);
                        }
                    }
                }

                /// <summary>
                /// 
                /// </summary>
                /// <param name="_pTask"></param>
                /// <returns>
                /// S_OK : 操作完成，成功添加到当前线程的等待队列。
                /// S_FALSE : 操作完成，成功添加到线程池其他异步线程的等待队列。
                /// < 0 : 操作失败，一般是内存不足等错误。 
                /// </returns>
                HRESULT __YYAPI SetWaitInternal(_In_ RefPtr<Wait> _pTask) noexcept
                {
                    auto _pWaitHandleHashList = GetWaitHandleHashList(_pTask->hHandle);

                    {
                        AutoLock<SRWLock> _oReadList(_pWaitHandleHashList->oLock);
                        auto _pWaitItem = _pWaitHandleHashList->FindWaitItem(_pTask->hHandle);
                        if (_pWaitItem)
                        {
                            _pWaitItem->oWaitTaskLists.PushBack(_pTask.Detach());
                            return S_OK;
                        }
                    }

                    auto _pWaitHandleBlock = GetWaitHandleBlock();
                    if (!_pWaitHandleBlock)
                    {
                        return E_OUTOFMEMORY;
                    }

                    auto _pWaitHandleEntry = New<WaitHandleEntry>();
                    if (!_pWaitHandleEntry)
                    {
                        return E_OUTOFMEMORY;
                    }

                    _pWaitHandleEntry->hWaitHandle = _pTask->hHandle;
                    _pWaitHandleEntry->pOwnerWaitHandleBlock = _pWaitHandleBlock;
                    _pWaitHandleEntry->oWaitTaskLists.PushBack(_pTask.Detach());

                    {
                        AutoLock<SRWLock> _oInsertList(_pWaitHandleHashList->oLock);
                        _pWaitHandleHashList->Insert(_pWaitHandleEntry);
                    }

                    bool _bFirstWaitHandle = false;
                    {
                        AutoLock<SRWLock> _oAddWaitTaskLock(_pWaitHandleBlock->oLock);
                        _bFirstWaitHandle = _pWaitHandleBlock->AddWaitHandleEntryNolock(_pWaitHandleEntry);
                    }

                    // DefaultWaitBlock 属于当前线程，所以他无需唤醒。
                    if (_pWaitHandleBlock != &oDefaultWaitBlock)
                    {
                        if (_bFirstWaitHandle)
                        {
                            // Wakeup
                            const auto _hr = _pWaitHandleBlock->pWaitTaskRunner->PostTask(
                                [this, _pWaitHandleBlock]()
                                {
                                    for (;;)
                                    {
                                        _pWaitHandleBlock->oLock.LockShared();
                                        auto _cWaitHandle = _pWaitHandleBlock->cWaitHandle;
                                        auto _bTerminate = _pWaitHandleBlock->bTerminate;
                                        _pWaitHandleBlock->oLock.UnlockShared();

                                        if (_bTerminate)
                                            break;

                                        if (_cWaitHandle <= 1)
                                            break;

                                        const auto _uResult = WaitForMultipleObjects(_cWaitHandle, _pWaitHandleBlock->hWaitHandles, FALSE, INFINITE);
                                        ProcessingWaitTasks(*_pWaitHandleBlock, _uResult, _cWaitHandle);
                                    }
                                });

                            if (FAILED(_hr))
                                return _hr;
                        }
                        else
                        {
                            SetEvent(_pWaitHandleBlock->hTaskRunnerServerHandle);
                        }

                        return S_FALSE;
                    }

                    return S_OK;
                }
                
                size_t __YYAPI ProcessingWaitTasks(WaitHandleBlock& oWaitBlockTaskRunner, DWORD uWaitResult, DWORD _cWaitHandle) noexcept
                {
                    size_t _cTaskProcessed = 0;

                    if (WAIT_OBJECT_0 <= uWaitResult && uWaitResult < WAIT_OBJECT_0 + _cWaitHandle)
                    {
                        uWaitResult -= WAIT_OBJECT_0;
                        if (uWaitResult != oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(oWaitBlockTaskRunner, uWaitResult, WAIT_OBJECT_0);
                        }
                    }
                    else if (WAIT_ABANDONED_0 <= uWaitResult && uWaitResult < WAIT_ABANDONED_0 + _cWaitHandle)
                    {
                        uWaitResult -= WAIT_ABANDONED_0;
                        if (uWaitResult != oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(oWaitBlockTaskRunner, uWaitResult, WAIT_ABANDONED_0);
                        }
                    }
                    else if (WAIT_IO_COMPLETION == uWaitResult)
                    {
                    }
                    else/* if (WAIT_FAILED == uWaitResult)*/
                    {
                        // 有句柄可能无效，检测一下无效句柄……然后报告
                        // 这里需要从后向前移动，避免 DispatchWaitList 后 cWaitHandle不停的减小
                        if (_cWaitHandle)
                        {
                            do
                            {
                                --_cWaitHandle;
                                if (_cWaitHandle != oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                                {
                                    const auto _uTaskWaitResult = WaitForSingleObject(oWaitBlockTaskRunner.hWaitHandles[_cWaitHandle], 0);
                                    if (_uTaskWaitResult != WAIT_TIMEOUT)
                                    {
                                        _cTaskProcessed += DispatchWaitList(oWaitBlockTaskRunner, _cWaitHandle, _uTaskWaitResult);
                                    }
                                }
                            } while (_cWaitHandle);
                        }
                    }
                    return _cTaskProcessed;
                }

            private:
                _Ret_notnull_ WaitHandleHashList* __YYAPI GetWaitHandleHashList(_In_ HANDLE _hWaitHandle) noexcept
                {
                    const auto _uIndex = (size_t(_hWaitHandle) / 4) % std::size(arrWaitHandleHashTable);
                    return &arrWaitHandleHashTable[_uIndex];
                }

                WaitHandleBlock* __YYAPI GetWaitHandleBlock() noexcept
                {
                    // 调用上下文必须是默认线程，所以访问 DefaultWaitBlock无需加锁
                    if (!oDefaultWaitBlock.IsFull())
                    {
                        return &oDefaultWaitBlock;
                    }

                    if (auto _pFirst = oWaitBlockTaskRunnerList.GetFirst())
                    {
                        auto _pItem = _pFirst;

                        do
                        {
                            if (!_pItem->IsFull())
                            {
                                return _pItem;
                            }

                            oWaitBlockTaskRunnerList.PushBack(oWaitBlockTaskRunnerList.PopFront());
                            _pItem = oWaitBlockTaskRunnerList.GetFirst();
                        } while (_pItem != _pFirst);
                    }

                    UniquePtr<WaitHandleBlock> _pNewTaskRunner(New<WaitHandleBlock>());
                    if (!_pNewTaskRunner)
                        return nullptr;

                    if (_pNewTaskRunner->hTaskRunnerServerHandle == nullptr || _pNewTaskRunner->pWaitTaskRunner == nullptr)
                        return nullptr;

                    oWaitBlockTaskRunnerList.Insert(_pNewTaskRunner);
                    return  _pNewTaskRunner.Detach();
                }

                size_t __YYAPI DispatchWaitList(WaitHandleBlock& oWaitBlockTaskRunner, size_t _uDispatchIndex, DWORD _uWaitResult) noexcept
                {
                    HANDLE _hWaitEvent = nullptr;
                    WaitHandleEntry* _pWaitHandleEntry = nullptr;
                    {
                        AutoLock<SRWLock> _oDispatchLock(oWaitBlockTaskRunner.oLock);
                        if (oWaitBlockTaskRunner.cWaitHandle <= _uDispatchIndex)
                            return 0;

                        _hWaitEvent = oWaitBlockTaskRunner.hWaitHandles[_uDispatchIndex];
                        _pWaitHandleEntry = oWaitBlockTaskRunner.oWaitHandleEntries[_uDispatchIndex];

                        // 快速删除，因为句柄不要求有序，直接与最后的元素进行交换就能做到删除。
                        --oWaitBlockTaskRunner.cWaitHandle;
                        if (oWaitBlockTaskRunner.cWaitHandle != _uDispatchIndex)
                        {
                            oWaitBlockTaskRunner.hWaitHandles[_uDispatchIndex] = oWaitBlockTaskRunner.hWaitHandles[oWaitBlockTaskRunner.cWaitHandle];
                            oWaitBlockTaskRunner.oWaitHandleEntries[_uDispatchIndex] = oWaitBlockTaskRunner.oWaitHandleEntries[oWaitBlockTaskRunner.cWaitHandle];
                        }

                        oWaitBlockTaskRunner.hWaitHandles[oWaitBlockTaskRunner.cWaitHandle] = nullptr;
                        oWaitBlockTaskRunner.oWaitHandleEntries[oWaitBlockTaskRunner.cWaitHandle] = nullptr;
                    }

                    assert(_hWaitEvent && _pWaitHandleEntry);
                    if (_hWaitEvent == nullptr || _pWaitHandleEntry == nullptr)
                    {
                        return 0;
                    }

                    {
                        auto _pWaitHandleHashList = GetWaitHandleHashList(_hWaitEvent);
                        AutoLock<SRWLock> _oRemoveWaitHandleEntry(_pWaitHandleHashList->oLock);
                        _pWaitHandleHashList->Remove(_pWaitHandleEntry);
                    }

                    size_t _cTaskProcessed = 0;
                    while (auto _pWaitTask = _pWaitHandleEntry->oWaitTaskLists.PopFront())
                    {
                        _pWaitTask->uWaitResult = _uWaitResult;
                        DispatchWaitTask(RefPtr<Wait>::FromPtr(_pWaitTask));
                        ++_cTaskProcessed;
                    }
                    return _cTaskProcessed;
                }

                virtual void __YYAPI DispatchWaitTask(RefPtr<Wait> _pWaitTask) = 0;
            };
        }
    }
}
#pragma pack(pop)
