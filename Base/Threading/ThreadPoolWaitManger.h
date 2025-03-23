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
#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Containers/DoublyLinkedList.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            struct WaitTaskList : public DoublyLinkedList<Wait>
            {
                /// <summary>
                /// 按超时时间，从近到远排序插入列表
                /// </summary>
                /// <param name="_pWait"></param>
                /// <returns></returns>
                void __YYAPI SortInsert(Wait* _pWait) noexcept
                {
                    if (!_pWait)
                        return;

                    auto _pLast = GetLast();
                    if (_pLast == nullptr || _pLast->uTimeOut <= _pWait->uTimeOut)
                    {
                        PushBack(_pWait);
                        return;
                    }

                    for (auto _pItem = GetFirst(); _pItem; _pItem= _pItem->pNext)
                    {
                        if (_pItem->uTimeOut > _pWait->uTimeOut)
                        {
                            Insert(_pItem, _pWait);
                            return;
                        }
                    }
                    
                    // 列表应该是有序的，为什么会出现在这里呢？
                    assert(false);
                }

                /// <summary>
                /// 将所有超时的任务移动到 _pPendingTimeout。
                /// </summary>
                /// <param name="_uCurrentTickCount">当前的 TickCount 计数</param>
                /// <param name="_pPendingTimeout">保存等待超时任务的链表。</param>
                /// <returns>
                /// 返回值代表Entry内是否任然存在元素。
                /// 返回 true: Entry内任然存在元素。
                /// 返回 false: Entry 已经为空，可以移除。
                /// </returns>
                bool __YYAPI ProcessingTimeoutNolock(_In_ TickCount<TimePrecise::Microsecond> _uCurrentTickCount, _Out_ WaitTaskList* _pPendingTimeout) noexcept
                {
                    while (auto _pWait = GetFirst())
                    {
                        if (_pWait->uTimeOut > _uCurrentTickCount)
                            break;

                        _pPendingTimeout->PushBack(PopFront());
                    }

                    return !IsEmpty();
                }
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
                    volatile uint32_t cWaitHandle = 0;

                    TickCount<TimePrecise::Microsecond> __YYAPI GetWakeupTickCountNolock(const TickCount<TimePrecise::Microsecond> _uTickCount) const noexcept
                    {
                        auto _uMinimumWakeupTickCount = TickCount<TimePrecise::Microsecond>::GetMax();
                        for (size_t i = 0; i < cWaitHandle; ++i)
                        {
                            auto& _oWaitTaskList = oWaitTaskLists[i];
        
                            if (auto _pFirst = _oWaitTaskList.GetFirst())
                            {
                                if (_uMinimumWakeupTickCount > _pFirst->uTimeOut)
                                {
                                    if (_uTickCount >= _pFirst->uTimeOut)
                                        return _uTickCount;

                                    _uMinimumWakeupTickCount = _pFirst->uTimeOut;
                                }
                            }
                        }

                        return _uMinimumWakeupTickCount;
                    }
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
                        oDefaultWaitBlock.oWaitTaskLists[i].SortInsert(_pWaitTask.Detach());
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
                    else if (WAIT_TIMEOUT == uWaitResult)
                    {
                        WaitTaskList _oDispatchPending;
                        const auto _uCurrentTickCount = TickCount<TimePrecise::Microsecond>::GetCurrent();

                        for (size_t i = oDefaultWaitBlock.cWaitHandle; i;)
                        {
                            --i;
                            if (!oDefaultWaitBlock.oWaitTaskLists[i].ProcessingTimeoutNolock(_uCurrentTickCount, &_oDispatchPending))
                            {
                                --oDefaultWaitBlock.cWaitHandle;
                                if (oDefaultWaitBlock.cWaitHandle != i)
                                {
                                    oDefaultWaitBlock.hWaitHandles[i] = oDefaultWaitBlock.hWaitHandles[oDefaultWaitBlock.cWaitHandle];
                                    oDefaultWaitBlock.oWaitTaskLists[i].PushBack(std::move(oDefaultWaitBlock.oWaitTaskLists[oDefaultWaitBlock.cWaitHandle]));
                                }
                                oDefaultWaitBlock.hWaitHandles[oDefaultWaitBlock.cWaitHandle] = nullptr;
                                oDefaultWaitBlock.oWaitTaskLists[oDefaultWaitBlock.cWaitHandle].Flush();
                            }                     
                        }

                        while (auto _pWaitTask = _oDispatchPending.PopFront())
                        {
                            _pWaitTask->uWaitResult = WAIT_TIMEOUT;
                            DispatchWaitTask(RefPtr<Wait>::FromPtr(_pWaitTask));
                            ++_cTaskProcessed;
                        }
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
                        oDefaultWaitBlock.oWaitTaskLists[_uDispatchIndex].PushBack(std::move(oDefaultWaitBlock.oWaitTaskLists[oDefaultWaitBlock.cWaitHandle]));
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
                    WaitTaskList oWaitTaskList;
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
                    volatile uint32_t cWaitHandle = 1;

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

                    TickCount<TimePrecise::Microsecond> __YYAPI GetWakeupTickCountNolock(const TickCount<TimePrecise::Microsecond> _uTickCount) const noexcept
                    {
                        auto _uMinimumWakeupTickCount = TickCount<TimePrecise::Microsecond>::GetMax();
                        for (size_t i = 1; i < cWaitHandle; ++i)
                        {
                            auto _pWaitHandleEntry = oWaitHandleEntries[i];
                            if (!_pWaitHandleEntry)
                                continue;

                            if (auto _pFirst = _pWaitHandleEntry->oWaitTaskList.GetFirst())
                            {
                                if (_uMinimumWakeupTickCount > _pFirst->uTimeOut)
                                {
                                    if (_uTickCount >= _pFirst->uTimeOut)
                                        return _uTickCount;

                                    _uMinimumWakeupTickCount = _pFirst->uTimeOut;
                                }
                            }
                        }

                        return _uMinimumWakeupTickCount;
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

                struct WaitHandleHashList : public DoublyLinkedList<WaitHandleEntry>
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
                DoublyLinkedList<WaitHandleBlock> oWaitBlockTaskRunnerList;

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
                            _pWaitItem->oWaitTaskList.SortInsert(_pTask.Detach());
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
                    _pWaitHandleEntry->oWaitTaskList.SortInsert(_pTask.Detach());

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
                                    uint32_t _cWaitHandle;
                                    TickCount<TimePrecise::Microsecond> _uWaitWakeupTickCount;

                                    for (;;)
                                    {
                                        {
                                            AutoSharedLock<SRWLock> _oRead(_pWaitHandleBlock->oLock);
                                            _cWaitHandle = _pWaitHandleBlock->cWaitHandle;
                                            if (_cWaitHandle <= 1)
                                                break;

                                            auto _bTerminate = _pWaitHandleBlock->bTerminate;
                                            if (_bTerminate)
                                                break;

                                            _uWaitWakeupTickCount = _pWaitHandleBlock->GetWakeupTickCountNolock(TickCount<TimePrecise::Microsecond>::GetCurrent());
                                        }

                                        const auto _uResult = WaitForMultipleObjects(_cWaitHandle, _pWaitHandleBlock->hWaitHandles, FALSE, GetWaitTimeSpan(_uWaitWakeupTickCount));
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
                
                size_t __YYAPI ProcessingWaitTasks(WaitHandleBlock& oWaitBlockTaskRunner, DWORD _uWaitResult, DWORD _cWaitHandle) noexcept
                {
                    size_t _cTaskProcessed = 0;

                    if (WAIT_OBJECT_0 <= _uWaitResult && _uWaitResult < WAIT_OBJECT_0 + _cWaitHandle)
                    {
                        _uWaitResult -= WAIT_OBJECT_0;
                        if (_uWaitResult != oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(oWaitBlockTaskRunner, _uWaitResult, WAIT_OBJECT_0);
                        }
                    }
                    else if (WAIT_ABANDONED_0 <= _uWaitResult && _uWaitResult < WAIT_ABANDONED_0 + _cWaitHandle)
                    {
                        _uWaitResult -= WAIT_ABANDONED_0;
                        if (_uWaitResult != oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                        {
                            _cTaskProcessed += DispatchWaitList(oWaitBlockTaskRunner, _uWaitResult, WAIT_ABANDONED_0);
                        }
                    }
                    else if (WAIT_IO_COMPLETION == _uWaitResult)
                    {
                    }
                    else if (WAIT_TIMEOUT == _uWaitResult)
                    {
                        // 检查所有超时的任务
                        WaitTaskList _oDispatchPending;
                        HANDLE _hPendingReomveWaitHandles[MAXIMUM_WAIT_OBJECTS] = {};
                        WaitHandleEntry* oPendingReomveWaitHandleEntries[MAXIMUM_WAIT_OBJECTS] = {};
                        uint32_t _cPendingReomveWaitHandleCount = 0;

                        {
                            AutoLock<SRWLock> _oDispatchLock(oWaitBlockTaskRunner.oLock);
                            const auto _uCurrentTickCount = TickCount<TimePrecise::Microsecond>::GetCurrent();

                            for (size_t i = _cWaitHandle; i; )
                            {
                                --i;
                                if (i == oWaitBlockTaskRunner.kTaskRunnerServerHandleIndex)
                                    continue;

                                auto _pWaitHandleEntry = oWaitBlockTaskRunner.oWaitHandleEntries[i];
                                if (!_pWaitHandleEntry)
                                    continue;

                                if (_pWaitHandleEntry->oWaitTaskList.ProcessingTimeoutNolock(_uCurrentTickCount, &_oDispatchPending))
                                    continue;

                                // _pWaitHandleEntry 以为空，所以需要移除
                                // 快速删除，因为句柄不要求有序，直接与最后的元素进行交换就能做到删除。
                                oPendingReomveWaitHandleEntries[_cPendingReomveWaitHandleCount] = _pWaitHandleEntry;
                                _hPendingReomveWaitHandles[_cPendingReomveWaitHandleCount] = oWaitBlockTaskRunner.hWaitHandles[i];
                                ++_cPendingReomveWaitHandleCount;

                                const auto _uLastIndex = --oWaitBlockTaskRunner.cWaitHandle;
                                if (oWaitBlockTaskRunner.cWaitHandle != i)
                                {
                                    oWaitBlockTaskRunner.hWaitHandles[i] = oWaitBlockTaskRunner.hWaitHandles[_uLastIndex];
                                    oWaitBlockTaskRunner.oWaitHandleEntries[i] = oWaitBlockTaskRunner.oWaitHandleEntries[_uLastIndex];
                                }
                                else
                                {
                                    oWaitBlockTaskRunner.hWaitHandles[i] = nullptr;
                                    oWaitBlockTaskRunner.oWaitHandleEntries[i] = nullptr;
                                }
                            }
                        }

                        while (auto _pWaitTask = _oDispatchPending.PopFront())
                        {
                            _pWaitTask->uWaitResult = WAIT_TIMEOUT;
                            DispatchWaitTask(RefPtr<Wait>::FromPtr(_pWaitTask));
                            ++_cTaskProcessed;
                        }

                        for(size_t i =0;i < _cPendingReomveWaitHandleCount;++i)
                        {
                            auto _pWaitHandleHashList = GetWaitHandleHashList(_hPendingReomveWaitHandles[i]);
                            AutoLock<SRWLock> _oRemoveWaitHandleEntry(_pWaitHandleHashList->oLock);
                            auto _pWaitHandleEntry = oPendingReomveWaitHandleEntries[i];

                            if (_pWaitHandleEntry->oWaitTaskList.IsEmpty())
                            {
                                _pWaitHandleHashList->Remove(_pWaitHandleEntry);
                                Delete(_pWaitHandleEntry);
                            }
                        }
                    }
                    else/* if (WAIT_FAILED == _uWaitResult)*/
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
                    while (auto _pWaitTask = _pWaitHandleEntry->oWaitTaskList.PopFront())
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
