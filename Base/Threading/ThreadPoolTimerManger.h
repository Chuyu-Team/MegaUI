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
            struct TimingWheelSimpleList
            {
                Timer* pFirst = nullptr;
                Timer* pLast = nullptr;

                TimingWheelSimpleList() = default;

                ~TimingWheelSimpleList()
                {
                    while (auto _pTask = Pop())
                    {
                        _pTask->Cancel();
                        _pTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                        _pTask->Release();
                    }
                }

                TimingWheelSimpleList(const TimingWheelSimpleList&) = delete;
                TimingWheelSimpleList& operator=(const TimingWheelSimpleList&) = delete;
                TimingWheelSimpleList& operator=(TimingWheelSimpleList&&) = default;


                _Ret_maybenull_ Timer* Pop() noexcept
                {
                    if (pFirst == nullptr)
                        return nullptr;

                    auto _pOldFirst = pFirst;
                    pFirst = _pOldFirst->pNext;

                    if (!pFirst)
                    {
                        pLast = nullptr;
                    }

                    return _pOldFirst;
                }

                void Push(_In_ Timer* _pEntry) noexcept
                {
                    _pEntry->pNext = nullptr;

                    if (pLast)
                    {
                        pLast->pNext = _pEntry;
                    }
                    else
                    {
                        pFirst = _pEntry;
                    }

                    pLast = _pEntry;
                }

                void Push(TimingWheelSimpleList&& _oList) noexcept
                {
                    if (this == &_oList)
                        return;

                    if (_oList.IsEmpty())
                        return;

                    if (pLast == nullptr)
                    {
                        pFirst = _oList.pFirst;
                        pLast = _oList.pLast;
                    }
                    else
                    {
                        pLast->pNext = _oList.pFirst;
                        pLast = _oList.pLast;
                    }
                    _oList.pFirst = nullptr;
                    _oList.pLast = nullptr;
                }

                bool IsEmpty() const noexcept
                {
                    return pFirst == nullptr;
                }
            };


            // 采用时间轮算法: UI库一般定时数百毫秒或者几分钟所以轮子设计时越容纳1小时左右，超过1小时的全部进入 arrTimingWheelOthers
            class ThreadPoolTimerManger
            {
            private:
                // 时间轮的最小分辨率，单位 ms
                static constexpr auto kTimingWheelBaseTick = 10;

                // 当前时间
                TickCount<TimePrecise::Millisecond> uTimingWheelCurrentTick = TickCount<TimePrecise::Millisecond>::GetCurrent();

                // 步进 10ms，(0, 640] ms
                TimingWheelSimpleList arrTimingWheel1[128];
                // 步进 640ms (640, 10'240] ms
                TimingWheelSimpleList arrTimingWheel2[64];
                // 步进 81'920ms (10'240, 163'840]
                TimingWheelSimpleList arrTimingWheel3[64];
                // 其余：655'360ms + 的元素
                TimingWheelSimpleList arrTimingWheelOthers;

                // TimingWheel的位图缓存，加速轮子的遍历过程
                BitMap<sizeof(arrTimingWheel1) / sizeof(arrTimingWheel1[0])> oTimingWheel1BitMap;
                BitMap<sizeof(arrTimingWheel2) / sizeof(arrTimingWheel2[0])> oTimingWheel2BitMap;
                BitMap<sizeof(arrTimingWheel3) / sizeof(arrTimingWheel3[0])> oTimingWheel3BitMap;

            protected:
                HRESULT __YYAPI SetTimerInternal(_In_ RefPtr<Timer> _pDispatchTask) noexcept
                {
                    if (!_pDispatchTask)
                        return E_INVALIDARG;

                    if (_pDispatchTask->uExpire <= uTimingWheelCurrentTick)
                    {
                        DispatchTimerTask(std::move(_pDispatchTask));
                        return S_OK;
                    }

                    const auto _uSpanBlock = (_pDispatchTask->uExpire - uTimingWheelCurrentTick).GetMilliseconds() / kTimingWheelBaseTick;
                    auto _uBase1 = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick + _uSpanBlock;

                    if (_uSpanBlock <= std::size(arrTimingWheel1))
                    {
                        // 可以容纳到一级时间轮
                        const uint32_t _uIndex = (_uBase1) % std::size(arrTimingWheel1);
                        arrTimingWheel1[_uIndex].Push(_pDispatchTask.Detach());
                        oTimingWheel1BitMap.SetItem(_uIndex, true);
                        return S_OK;
                    }
                    else if (_uSpanBlock <= std::size(arrTimingWheel1) * std::size(arrTimingWheel2))
                    {
                        const uint32_t _uIndex = _uBase1 / std::size(arrTimingWheel1) % std::size(arrTimingWheel2);
                        arrTimingWheel2[_uIndex].Push(_pDispatchTask.Detach());
                        oTimingWheel2BitMap.SetItem(_uIndex, true);
                        return S_OK;
                    }
                    else if (_uSpanBlock <= std::size(arrTimingWheel1) * std::size(arrTimingWheel2) * std::size(arrTimingWheel3))
                    {
                        const uint32_t _uIndex = _uBase1 / std::size(arrTimingWheel1) / std::size(arrTimingWheel2) % std::size(arrTimingWheel3);
                        arrTimingWheel3[_uIndex].Push(_pDispatchTask.Detach());
                        oTimingWheel3BitMap.SetItem(_uIndex, true);
                        return S_OK;
                    }
                    else
                    {
                        // 保底
                        arrTimingWheelOthers.Push(_pDispatchTask.Detach());
                        return S_OK;
                    }
                }

                void __YYAPI ProcessingTimerTasks() noexcept
                {
                    TickCount<TimePrecise::Millisecond> _oCurrent = TickCount<TimePrecise::Millisecond>::GetCurrent();

                    uint64_t _uFirstBlockIndex = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick;

                    uint64_t _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick;

                    uint64_t _uTimingWheelTickSpand = _uLastBlockIndexIndex - _uFirstBlockIndex;

                    // 时间轮没有转动
                    if (_uTimingWheelTickSpand == 0u)
                    {
                        uTimingWheelCurrentTick = _oCurrent;
                        return;
                    }

                    // 防止 DispatchTimerTask 期间又插入时间轮，所有我们先把所有满足条件的Timer暂存在_oTimerPendingDispatchList。最后再一次性的 Dispatch
                    TimingWheelSimpleList _oTimerPendingDispatchList;

                    if (_uTimingWheelTickSpand < std::size(arrTimingWheel1))
                    {
                        /////////////////////////////////////////
                        //
                        // 快速检测没有发现任何一个轮耗尽，开始对第一个轮详细检测
                        //
                        /////////////////////////////////////////

                        const uint32_t _uStartIndex1 = (_uFirstBlockIndex + 1) % std::size(arrTimingWheel1);
                        const uint32_t _uEndIndex1 = (_uLastBlockIndexIndex + 1) % std::size(arrTimingWheel1);
                        // End可能回头，所以检测一下
                        const uint32_t _uMaxRigthEndIndex1 = _uStartIndex1 <= _uEndIndex1 ? _uEndIndex1 : std::size(arrTimingWheel1);

                        for (int32_t _iIndex = _uStartIndex1; ;)
                        {
                            _iIndex = oTimingWheel1BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;

                            const auto _uIndex = (uint32_t)_iIndex;
                            if (_uIndex >= _uMaxRigthEndIndex1)
                                break;
                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel1[_uIndex]));
                            oTimingWheel1BitMap.SetItem(_uIndex, false);
                        }

                        if (_uStartIndex1 > _uEndIndex1)
                        {
                            // 发生了回头？，再调头清理一遍

                            for (int32_t _iIndex = 0; ; ++_iIndex)
                            {
                                _iIndex = oTimingWheel1BitMap.Find(_iIndex);
                                if (_iIndex < 0)
                                    break;
                                const auto _uIndex = (uint32_t)_iIndex;
                                if (_uIndex >= _uEndIndex1)
                                    break;

                                _oTimerPendingDispatchList.Push(std::move(arrTimingWheel1[_uIndex]));
                                oTimingWheel1BitMap.SetItem(_uIndex, false);
                            }
                        }

                        if (_uStartIndex1 == 0 || _uStartIndex1 > _uEndIndex1)
                        {
                            const uint64_t _uTimingWheelIndex2 = _uFirstBlockIndex / std::size(arrTimingWheel1) + 1;
                            FetchTimingWheel1(_uTimingWheelIndex2, _oCurrent, &_oTimerPendingDispatchList);
                        }
                    }
                    else if (_uTimingWheelTickSpand < std::size(arrTimingWheel1) * std::size(arrTimingWheel2))
                    {
                        /////////////////////////////////////////
                        // 快速检测 1 轮已经耗尽，直接清空
                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel1BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;
                            auto _uIndex = (uint32_t)_iIndex;
                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel1[_uIndex]));
                            oTimingWheel1BitMap.SetItem(_uIndex, false);
                        }
                        //
                        /////////////////////////////////////////

                        auto _uStartBlockIndex2 = (_uFirstBlockIndex + 1) / std::size(arrTimingWheel1);
                        const auto _uLastBlockIndex2 = _uLastBlockIndexIndex / std::size(arrTimingWheel1);
                        auto _uEndBlockIndex2 = (_uLastBlockIndexIndex + 1) / std::size(arrTimingWheel1);

                        const uint32_t _uStartIndex2 = _uStartBlockIndex2 % std::size(arrTimingWheel2);
                        const uint32_t _uEndIndex2 = _uEndBlockIndex2 % std::size(arrTimingWheel2);
                        const uint32_t _uMaxRigthEndIndex2 = _uStartIndex2 <= _uEndIndex2 ? _uEndIndex2 : std::size(arrTimingWheel2);

                        for (int32_t _iIndex = _uStartIndex2; ; ++_iIndex)
                        {
                            _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;
                            const auto _uIndex = (uint32_t)_iIndex;
                            if (_uIndex >= _uMaxRigthEndIndex2)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel2[_uIndex]));
                            oTimingWheel2BitMap.SetItem(_uIndex, false);
                        }


                        if (_uStartIndex2 > _uEndIndex2)
                        {
                            // 发生了回头？，再调头清理一遍

                            for (int32_t _iIndex = 0; ; ++_iIndex)
                            {
                                _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                                if (_iIndex < 0)
                                    break;
                                const auto _uIndex = (uint32_t)_iIndex;
                                if (_uIndex >= _uEndIndex2)
                                    break;
                                _oTimerPendingDispatchList.Push(std::move(arrTimingWheel2[_uIndex]));
                                oTimingWheel2BitMap.SetItem(_uIndex, false);
                            }
                        }

                        if (_uEndBlockIndex2 == _uLastBlockIndex2)
                        {
                            FetchTimingWheel1(_uEndBlockIndex2, _oCurrent, &_oTimerPendingDispatchList);
                        }

                        if (_uStartIndex2 == 0 || _uStartIndex2 > _uEndIndex2)
                        {
                            const uint64_t _uTimingWheelIndex3 = _uFirstBlockIndex / std::size(arrTimingWheel1) / std::size(arrTimingWheel2) + 1;
                            FetchTimingWheel2(_uTimingWheelIndex3, _oCurrent, &_oTimerPendingDispatchList);
                        }
                    }
                    else if (_uTimingWheelTickSpand < std::size(arrTimingWheel1) * std::size(arrTimingWheel2) * std::size(arrTimingWheel3))
                    {
                        /////////////////////////////////////////
                        // 快速检测 1，2轮已经耗尽，直接清空
                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel1BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel1[_iIndex]));
                            oTimingWheel1BitMap.SetItem(_iIndex, false);
                        }

                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;
                            const auto _uIndex = (uint32_t)_iIndex;
                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel2[_uIndex]));
                            oTimingWheel2BitMap.SetItem(_uIndex, false);
                        }
                        //
                        /////////////////////////////////////////


                        auto _uStartBlockIndex3 = (_uFirstBlockIndex + 1) / std::size(arrTimingWheel1) / std::size(arrTimingWheel2);
                        const auto _uLastBlockIndex3 = _uLastBlockIndexIndex / std::size(arrTimingWheel1) / std::size(arrTimingWheel2);
                        auto _uEndBlockIndex3 = (_uLastBlockIndexIndex + 1) / std::size(arrTimingWheel1) / std::size(arrTimingWheel2);

                        const uint32_t _uStartIndex3 = _uStartBlockIndex3 % std::size(arrTimingWheel3);
                        const uint32_t _uEndIndex3 = _uEndBlockIndex3 % std::size(arrTimingWheel3);
                        const uint32_t _uMaxRigthEndIndex3 = _uStartIndex3 <= _uEndIndex3 ? _uEndIndex3 : std::size(arrTimingWheel3);

                        for (int32_t _iIndex = _uStartIndex3; ; ++_iIndex)
                        {
                            _iIndex = oTimingWheel3BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;
                            const auto _uIndex = (uint32_t)_iIndex;
                            if (_uIndex >= _uMaxRigthEndIndex3)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel3[_uIndex]));
                            oTimingWheel3BitMap.SetItem(_uIndex, false);
                        }

                        if (_uStartIndex3 > _uEndIndex3)
                        {
                            // 发生了回头？，再调头清理一遍

                            for (int32_t _iIndex = 0; ; ++_iIndex)
                            {
                                _iIndex = oTimingWheel3BitMap.Find(_iIndex);
                                if (_iIndex < 0)
                                    break;
                                const auto _uIndex = (uint32_t)_iIndex;
                                if (_uIndex >= _uEndIndex3)
                                    break;
                                _oTimerPendingDispatchList.Push(std::move(arrTimingWheel3[_uIndex]));
                                oTimingWheel3BitMap.SetItem(_uIndex, false);
                            }
                        }

                        if (_uEndBlockIndex3 == _uLastBlockIndex3)
                        {
                            FetchTimingWheel2(_uEndBlockIndex3, _oCurrent, &_oTimerPendingDispatchList);
                        }

                        if (_uStartIndex3 == 0 || _uStartIndex3 > _uEndIndex3)
                        {
                            FetchTimingWheel3(_oCurrent, &_oTimerPendingDispatchList);
                        }
                    }
                    else
                    {
                        /////////////////////////////////////////
                        // 快速检测 3个轮全部耗尽，直接清空
                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel1BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel1[_iIndex]));
                            oTimingWheel1BitMap.SetItem(_iIndex, false);
                        }

                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel2[_iIndex]));
                            oTimingWheel2BitMap.SetItem(_iIndex, false);
                        }

                        for (int32_t _iIndex = 0;; ++_iIndex)
                        {
                            _iIndex = oTimingWheel3BitMap.Find(_iIndex);
                            if (_iIndex < 0)
                                break;

                            _oTimerPendingDispatchList.Push(std::move(arrTimingWheel3[_iIndex]));
                            oTimingWheel3BitMap.SetItem(_iIndex, false);
                        }
                        //
                        /////////////////////////////////////////

                        FetchTimingWheel3(_oCurrent, &_oTimerPendingDispatchList);
                    }

                    uTimingWheelCurrentTick = _oCurrent;

                    while (auto _pItem = _oTimerPendingDispatchList.Pop())
                    {
                        DispatchTimerTask(RefPtr<Timer>::FromPtr(_pItem));
                    }
                }

                uint32_t __YYAPI GetMinimumWaitTime() noexcept
                {
                    const auto _uTimingWheel1NextTick = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick + 1;

                    {
                        const uint32_t _uFirstIndex = _uTimingWheel1NextTick % std::size(arrTimingWheel1);
                        auto _nFind = oTimingWheel1BitMap.Find(_uFirstIndex);
                        if (_nFind >= 0)
                        {
                            return (_nFind - _uFirstIndex + 1) * kTimingWheelBaseTick;
                        }
                        else if (!oTimingWheel1BitMap.IsEmpty())
                        {
                            return (std::size(arrTimingWheel1) - _uFirstIndex) * kTimingWheelBaseTick;
                        }
                    }

                    const auto _uTimingWheel2NextTick = _uTimingWheel1NextTick / std::size(arrTimingWheel1) + 1;
                    const auto _uTimingWheel2BlockSize = kTimingWheelBaseTick * std::size(arrTimingWheel1);

                    {
                        const uint32_t _uSecondIndex = _uTimingWheel2NextTick % std::size(arrTimingWheel2);
                        auto _nFind = oTimingWheel2BitMap.Find(_uSecondIndex);
                        if (_nFind >= 0)
                        {
                            return (_nFind - _uSecondIndex + 1) * _uTimingWheel2BlockSize;
                        }
                        else if (!oTimingWheel2BitMap.IsEmpty())
                        {
                            return (std::size(arrTimingWheel2) - _uSecondIndex) * _uTimingWheel2BlockSize;
                        }
                    }

                    const auto _uTimingWheel3NextTick = _uTimingWheel2NextTick / std::size(arrTimingWheel2) + 1;
                    const auto _uTimingWheel3BlockSize = _uTimingWheel2BlockSize * std::size(arrTimingWheel2);

                    {
                        const uint32_t _uIndex = _uTimingWheel2NextTick % std::size(arrTimingWheel3);
                        auto _nFind = oTimingWheel3BitMap.Find(_uIndex);
                        if (_nFind >= 0)
                        {
                            return (_nFind - _uIndex + 1) * _uTimingWheel3BlockSize;
                        }
                        else if (oTimingWheel3BitMap.IsEmpty() == false || arrTimingWheelOthers.IsEmpty() == false)
                        {
                            return (std::size(arrTimingWheel3) - _uIndex) * _uTimingWheel3BlockSize;
                        }
                    }

                    // 当前没有任何任务，可以进行无限期等待
                    return UINT32_MAX;
                }
                
            private:
                virtual void __YYAPI DispatchTimerTask(RefPtr<Timer> _pTimerTask) = 0;

                void __YYAPI FetchTimingWheel1(uint64_t _uTimingWheel2Index, TickCount<TimePrecise::Millisecond> _oCurrent, _Out_ TimingWheelSimpleList* _pTimerPendingDispatchList)
                {
                    const uint32_t _uTimingWheel2BlockIndex = _uTimingWheel2Index % std::size(arrTimingWheel2);

                    if (_uTimingWheel2BlockIndex == 0u)
                    {
                        auto _uTimingWheel3Index = _uTimingWheel2Index / std::size(arrTimingWheel2);
                        FetchTimingWheel2(_uTimingWheel3Index, _oCurrent, _pTimerPendingDispatchList);
                        return;
                    }

                    auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick;

                    auto& _oTimingWheel2 = arrTimingWheel2[_uTimingWheel2BlockIndex];

                    while (auto _pItem = _oTimingWheel2.Pop())
                    {
                        if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
                        {
                            _pTimerPendingDispatchList->Push(_pItem);
                            continue;
                        }

                        const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / kTimingWheelBaseTick;
                        auto _uNewLastBlockIndexIndex = _uLastBlockIndexIndex + _uBaseBlockCount;

                        if (_uBaseBlockCount < std::size(arrTimingWheel1))
                        {
                            const uint32_t _uIndex = _uNewLastBlockIndexIndex % std::size(arrTimingWheel1);
                            arrTimingWheel1[_uIndex].Push(_pItem);
                            oTimingWheel1BitMap.SetItem(_uIndex, true);
                        }
                        else
                        {
                            // 按理说不可能发生这种情况，3轮取出来的数据只能在2轮或者更低，这是内存损坏了？

                            assert(false);

                            _pTimerPendingDispatchList->Push(_pItem);
                        }
                    }

                    oTimingWheel2BitMap.SetItem(_uTimingWheel2BlockIndex, false);
                }

                void __YYAPI FetchTimingWheel2(uint64_t _uTimingWheel3Index, TickCount<TimePrecise::Millisecond> _oCurrent, _Out_ TimingWheelSimpleList* _pTimerPendingDispatchList)
                {
                    const uint32_t _uTimingWheel3BlockIndex = _uTimingWheel3Index % std::size(arrTimingWheel2);

                    if (_uTimingWheel3BlockIndex == 0u)
                    {
                        FetchTimingWheel3(_oCurrent, _pTimerPendingDispatchList);
                        return;
                    }

                    auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick;

                    auto& _oTimingWheel3 = arrTimingWheel3[_uTimingWheel3BlockIndex];

                    while (auto _pItem = arrTimingWheel3[_uTimingWheel3BlockIndex].Pop())
                    {
                        if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
                        {
                            _pTimerPendingDispatchList->Push(_pItem);
                            continue;
                        }

                        const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / kTimingWheelBaseTick;
                        auto _uNewLastBlockIndexIndex = _uLastBlockIndexIndex + _uBaseBlockCount;

                        if (_uBaseBlockCount < std::size(arrTimingWheel1))
                        {
                            uint32_t _uIndex = _uNewLastBlockIndexIndex % std::size(arrTimingWheel1);
                            arrTimingWheel1[_uIndex].Push(_pItem);
                            oTimingWheel1BitMap.SetItem(_uIndex, true);
                        }
                        else if (_uBaseBlockCount < std::size(arrTimingWheel1) * std::size(arrTimingWheel2))
                        {
                            uint32_t _uIndex = (_uNewLastBlockIndexIndex / std::size(arrTimingWheel1)) % std::size(arrTimingWheel2);
                            arrTimingWheel2[_uIndex].Push(_pItem);
                            oTimingWheel2BitMap.SetItem(_uIndex, true);
                        }
                        else
                        {
                            // 按理说不可能发生这种情况，3轮取出来的数据只能在2轮或者更低，这是内存损坏了？

                            assert(false);

                            _pTimerPendingDispatchList->Push(_pItem);
                        }
                    }

                    oTimingWheel3BitMap.SetItem(_uTimingWheel3BlockIndex, false);
                }

                void __YYAPI FetchTimingWheel3(TickCount<TimePrecise::Millisecond> _oCurrent, _Out_ TimingWheelSimpleList* _pTimerPendingDispatchList)
                {
                    auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / kTimingWheelBaseTick;

                    TimingWheelSimpleList _oNewTimingWheelOthers;

                    while (auto _pItem = arrTimingWheelOthers.Pop())
                    {
                        if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
                        {
                            _pTimerPendingDispatchList->Push(_pItem);
                            continue;
                        }

                        const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / kTimingWheelBaseTick;
                        auto _uNewLastBlockIndexIndex = _uLastBlockIndexIndex + _uBaseBlockCount;

                        if (_uBaseBlockCount < std::size(arrTimingWheel1))
                        {
                            uint32_t _uIndex = _uNewLastBlockIndexIndex % std::size(arrTimingWheel1);
                            arrTimingWheel1[_uIndex].Push(_pItem);
                            oTimingWheel1BitMap.SetItem(_uIndex, true);
                        }
                        else if (_uBaseBlockCount < std::size(arrTimingWheel1) * std::size(arrTimingWheel2))
                        {
                            uint32_t _uIndex = (_uNewLastBlockIndexIndex / std::size(arrTimingWheel1)) % std::size(arrTimingWheel2);
                            arrTimingWheel2[_uIndex].Push(_pItem);
                            oTimingWheel2BitMap.SetItem(_uIndex, true);
                        }
                        else if (_uBaseBlockCount < std::size(arrTimingWheel1) * std::size(arrTimingWheel2) * std::size(arrTimingWheel3))
                        {
                            uint32_t _uIndex = (_uNewLastBlockIndexIndex / std::size(arrTimingWheel1) / std::size(arrTimingWheel2)) % std::size(arrTimingWheel3);
                            arrTimingWheel3[_uIndex].Push(_pItem);
                            oTimingWheel3BitMap.SetItem(_uIndex, true);
                        }
                        else
                        {
                            // 时间还很长，暂时略过
                            _oNewTimingWheelOthers.Push(_pItem);
                        }
                    }

                    arrTimingWheelOthers = std::move(_oNewTimingWheelOthers);
                }
            };
        }
    }
}

#pragma pack(pop)
