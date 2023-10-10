#include "pch.h"
#include "TaskRunnerDispatchImpl.h"

#include <Base/Time/TimeSpan.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    // 时间轮的最小分辨率，单位 ms
    constexpr auto TimingWheelBaseTick = 10;

    static TickCount<TimePrecise::Millisecond> __YYAPI GetNearTick(TickCount<TimePrecise::Millisecond> _uTick) noexcept
    {
        // 故意对齐到就近的 TimingWheelBaseTick 值。便于快速判断。
        auto _nMillisecond = (_uTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds();
        _nMillisecond += TimingWheelBaseTick / 2;
        _nMillisecond -= _nMillisecond % TimingWheelBaseTick;
        _nMillisecond += TimingWheelBaseTick - 1;
        return TickCount<TimePrecise::Millisecond>() + TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(_nMillisecond);
    }

    TaskRunnerDispatchImplByIoCompletionImpl::TaskRunnerDispatchImplByIoCompletionImpl()
        : fFlags(0u)
        , uTimingWheelCurrentTick()
    {
        hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u);
        //hWeakupEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    }

    TaskRunnerDispatchImplByIoCompletionImpl::~TaskRunnerDispatchImplByIoCompletionImpl()
    {
        if (hIoCompletionPort)
        {
            CloseHandle(hIoCompletionPort);
        }
    }

    TaskRunnerDispatchImplByIoCompletionImpl* __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Get() noexcept
    {
        static TaskRunnerDispatchImplByIoCompletionImpl s_Dispatch;
        return &s_Dispatch;
    }

    bool __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::BindIO(HANDLE _hHandle) const noexcept
    {
        if (_hHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        if (CreateIoCompletionPort(_hHandle, hIoCompletionPort, 0, 1) != hIoCompletionPort)
        {
            return false;
        }
        return true;
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Weakup()
    {
        if (!Sync::BitSet(&fFlags, 0))
        {
            auto _bRet = TrySubmitThreadpoolCallback(
                [](_Inout_ PTP_CALLBACK_INSTANCE Instance, _In_ PVOID Context) -> void
                {
                    auto _pDispatch = ((TaskRunnerDispatchImplByIoCompletionImpl*)Context);

                    _pDispatch->ExecuteTaskRunner();
                },
                this,
                nullptr);

            if (!_bRet)
            {
                throw Exception(YY::Base::HRESULT_From_LSTATUS(GetLastError()));
            }
        }
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::AddDelayTask(RefPtr<TaskEntry> _pDispatchTask) noexcept
    {
        for (;;)
        {
            if (!Sync::BitSet(&fFlags, 1))
            {
                arrTimerDispatchPending.Push(_pDispatchTask.Detach());
                Sync::BitReset(&fFlags, 1);
                break;
            }
        }

        Weakup();
        // TODO: 按需唤醒
        PostQueuedCompletionStatus(hIoCompletionPort, 0, 0, nullptr);    
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::ExecuteTaskRunner()
    {
        uTimingWheelCurrentTick = GetNearTick(TickCount<TimePrecise::Millisecond>::GetCurrent());

        OVERLAPPED_ENTRY _oCompletionPortEntries[16];
        ULONG _uNumEntriesRemoved;

        for (;;)
        {
            for (;;)
            {
                auto _pDispatchTask = RefPtr<TaskEntry>::FromPtr(arrTimerDispatchPending.Pop());
                if (!_pDispatchTask)
                    break;

                if (_pDispatchTask->uExpire <= uTimingWheelCurrentTick)
                {
                    Dispatch(std::move(_pDispatchTask));
                    continue;
                }

                AddTimingWheel(std::move(_pDispatchTask));
            }

            auto uMinimumWaitTime = GetMinimumWaitTime();
            auto _bRet = GetQueuedCompletionStatusEx(hIoCompletionPort, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, uMinimumWaitTime, FALSE);

            if (!_bRet)
            {
                _uNumEntriesRemoved = 0;

                auto _lStatus = GetLastError();

                if (_lStatus == WAIT_TIMEOUT || _lStatus == WAIT_IO_COMPLETION)
                {
                    // 非意外错误
                }
                else
                {
                    // ERROR_ABANDONED_WAIT_0 ： 句柄关闭，线程应该退出。
                    return;
                }
            }

            // 处理完成端口的数据，它的优先级最高
            for (ULONG _uIndex = 0; _uIndex != _uNumEntriesRemoved; ++_uIndex)
            {
                auto _pDispatchTask = RefPtr<TaskEntry>::FromPtr(static_cast<IoTaskEntry*>(_oCompletionPortEntries[_uIndex].lpOverlapped));
                if (!_pDispatchTask)
                    continue;

                Dispatch(std::move(_pDispatchTask));
            }

            auto _oCurrent = GetNearTick(TickCount<TimePrecise::Millisecond>::GetCurrent());

            DispatchTimingWheel(_oCurrent);

            uTimingWheelCurrentTick = _oCurrent;
        }
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Dispatch(TimingWheelSimpleList& _oTimingWheelList)
    {
        while (auto _pItem = _oTimingWheelList.Pop())
        {
            Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
        }
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::FetchTimingWheel1(uint64_t _uTimingWheel2Index, TickCount<TimePrecise::Millisecond> _oCurrent)
    {
        const uint32_t _uTimingWheel2BlockIndex = _uTimingWheel2Index % std::size(arrTimingWheel2);

        if (_uTimingWheel2BlockIndex == 0u)
        {
            auto _uTimingWheel3Index = _uTimingWheel2Index / std::size(arrTimingWheel2);
            FetchTimingWheel2(_uTimingWheel3Index, _oCurrent);
            return;
        }

        auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick;

        auto& _oTimingWheel2 = arrTimingWheel2[_uTimingWheel2BlockIndex];

        while (auto _pItem = _oTimingWheel2.Pop())
        {
            if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
            {
                Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
                continue;
            }

            const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / TimingWheelBaseTick;
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

                Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
            }
        }

        oTimingWheel2BitMap.SetItem(_uTimingWheel2BlockIndex, false);
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::FetchTimingWheel2(uint64_t _uTimingWheel3Index, TickCount<TimePrecise::Millisecond> _oCurrent)
    {
        const uint32_t _uTimingWheel3BlockIndex = _uTimingWheel3Index % std::size(arrTimingWheel2);

        if (_uTimingWheel3BlockIndex == 0u)
        {
            FetchTimingWheel3(_oCurrent);
            return;
        }

        auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick;

        auto& _oTimingWheel3 = arrTimingWheel3[_uTimingWheel3BlockIndex];

        while (auto _pItem = arrTimingWheel3[_uTimingWheel3BlockIndex].Pop())
        {
            if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
            {
                Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
                continue;
            }

            const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / TimingWheelBaseTick;
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

                Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
            }
        }

        oTimingWheel3BitMap.SetItem(_uTimingWheel3BlockIndex, false);
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::FetchTimingWheel3(TickCount<TimePrecise::Millisecond> _oCurrent)
    {
        auto _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick;

        TimingWheelSimpleList _oNewTimingWheelOthers;

        while (auto _pItem = arrTimingWheelOthers.Pop())
        {
            if (_pItem->IsCanceled() || _pItem->uExpire <= _oCurrent)
            {
                Dispatch(RefPtr<TaskEntry>::FromPtr(_pItem));
                continue;
            }

            const uint64_t _uBaseBlockCount = (_pItem->uExpire - _oCurrent).GetMilliseconds() / TimingWheelBaseTick;
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

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::DispatchTimingWheel(TickCount<TimePrecise::Millisecond> _oCurrent)
    {
        uint64_t _uFirstBlockIndex = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick;

        uint64_t _uLastBlockIndexIndex = (_oCurrent - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick;

        uint64_t _uTimingWheelTickSpand = _uLastBlockIndexIndex - _uFirstBlockIndex;

        // 时间轮没有转动
        if (_uTimingWheelTickSpand == 0u)
            return;

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
                Dispatch(arrTimingWheel1[_uIndex]);
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

                    Dispatch(arrTimingWheel1[_uIndex]);
                    oTimingWheel1BitMap.SetItem(_uIndex, false);
                }
            }

            if (_uStartIndex1 == 0 || _uStartIndex1 > _uEndIndex1)
            {
                const uint64_t _uTimingWheelIndex2 = _uFirstBlockIndex / std::size(arrTimingWheel1) + 1;
                FetchTimingWheel1(_uTimingWheelIndex2, _oCurrent);
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
                Dispatch(arrTimingWheel1[_uIndex]);
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

                Dispatch(arrTimingWheel2[_uIndex]);
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
                    Dispatch(arrTimingWheel2[_uIndex]);
                    oTimingWheel2BitMap.SetItem(_uIndex, false);
                }
            }

            if (_uEndBlockIndex2 == _uLastBlockIndex2)
            {
                FetchTimingWheel1(_uEndBlockIndex2, _oCurrent);
            }

            if (_uStartIndex2 == 0 || _uStartIndex2 > _uEndIndex2)
            {
                const uint64_t _uTimingWheelIndex3 = _uFirstBlockIndex / std::size(arrTimingWheel1) / std::size(arrTimingWheel2) + 1;
                FetchTimingWheel2(_uTimingWheelIndex3, _oCurrent);
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

                Dispatch(arrTimingWheel1[_iIndex]);
                oTimingWheel1BitMap.SetItem(_iIndex, false);
            }

            for (int32_t _iIndex = 0;; ++_iIndex)
            {
                _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                if (_iIndex < 0)
                    break;
                const auto _uIndex = (uint32_t)_iIndex;
                Dispatch(arrTimingWheel2[_uIndex]);
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

                Dispatch(arrTimingWheel3[_uIndex]);
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
                    Dispatch(arrTimingWheel3[_uIndex]);
                    oTimingWheel3BitMap.SetItem(_uIndex, false);
                }
            }

            if (_uEndBlockIndex3 == _uLastBlockIndex3)
            {
                FetchTimingWheel2(_uEndBlockIndex3, _oCurrent);
            }

            if (_uStartIndex3 == 0 || _uStartIndex3 > _uEndIndex3)
            {
                FetchTimingWheel3(_oCurrent);
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

                Dispatch(arrTimingWheel1[_iIndex]);
                oTimingWheel1BitMap.SetItem(_iIndex, false);
            }

            for (int32_t _iIndex = 0;; ++_iIndex)
            {
                _iIndex = oTimingWheel2BitMap.Find(_iIndex);
                if (_iIndex < 0)
                    break;

                Dispatch(arrTimingWheel2[_iIndex]);
                oTimingWheel2BitMap.SetItem(_iIndex, false);
            }

            for (int32_t _iIndex = 0;; ++_iIndex)
            {
                _iIndex = oTimingWheel3BitMap.Find(_iIndex);
                if (_iIndex < 0)
                    break;

                Dispatch(arrTimingWheel3[_iIndex]);
                oTimingWheel3BitMap.SetItem(_iIndex, false);
            }
            //
            /////////////////////////////////////////

            FetchTimingWheel3(_oCurrent);
        }
    }

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::AddTimingWheel(RefPtr<TaskEntry> _pDispatchTask) noexcept
    {
        const auto _uSpanBlock = (_pDispatchTask->uExpire - uTimingWheelCurrentTick).GetMilliseconds() / TimingWheelBaseTick;
        auto _uBase1 = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick + _uSpanBlock;

        if (_uSpanBlock <= std::size(arrTimingWheel1))
        {
            // 可以容纳到一级时间轮
            const uint32_t _uIndex = (_uBase1) % std::size(arrTimingWheel1);
            arrTimingWheel1[_uIndex].Push(_pDispatchTask.Detach());
            oTimingWheel1BitMap.SetItem(_uIndex, true);
            return;
        }
        else if (_uSpanBlock <= std::size(arrTimingWheel1) * std::size(arrTimingWheel2))
        {
            const uint32_t _uIndex = _uBase1 / std::size(arrTimingWheel1) % std::size(arrTimingWheel2);
            arrTimingWheel2[_uIndex].Push(_pDispatchTask.Detach());
            oTimingWheel2BitMap.SetItem(_uIndex, true);
            return;
        }
        else if (_uSpanBlock <= std::size(arrTimingWheel1) * std::size(arrTimingWheel2) * std::size(arrTimingWheel3))
        {
            const uint32_t _uIndex = _uBase1 / std::size(arrTimingWheel1) / std::size(arrTimingWheel2) % std::size(arrTimingWheel3);
            arrTimingWheel3[_uIndex].Push(_pDispatchTask.Detach());
            oTimingWheel3BitMap.SetItem(_uIndex, true);
            return;
        }
        else
        {
            // 保底
            arrTimingWheelOthers.Push(_pDispatchTask.Detach());
        }
    }

    uint32_t __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::GetMinimumWaitTime() const noexcept
    {
        const auto _uTimingWheel1NextTick = (uTimingWheelCurrentTick - TickCount<TimePrecise::Millisecond>()).GetMilliseconds() / TimingWheelBaseTick + 1;
        const auto _uTimingWheel1BlockSize = TimingWheelBaseTick;

        {
            const uint32_t _uFirstIndex = _uTimingWheel1NextTick % std::size(arrTimingWheel1);
            auto _nFind = oTimingWheel1BitMap.Find(_uFirstIndex);
            if (_nFind >= 0)
            {
                return (_nFind - _uFirstIndex + 1) * _uTimingWheel1BlockSize;
            }
            else if (!oTimingWheel1BitMap.IsEmpty())
            {
                return (std::size(arrTimingWheel1) - _uFirstIndex) * _uTimingWheel1BlockSize;
            }
        }

        const auto _uTimingWheel2NextTick = _uTimingWheel1NextTick / std::size(arrTimingWheel1) + 1;
        const auto _uTimingWheel2BlockSize = _uTimingWheel1BlockSize * std::size(arrTimingWheel1);

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

    void __YYAPI TaskRunnerDispatchImplByIoCompletionImpl::Dispatch(RefPtr<TaskEntry> _pDispatchTask)
    {
        auto _pResumeTaskRunner = _pDispatchTask->pOwnerTaskRunnerWeak.Get();
        if (_pResumeTaskRunner == nullptr || _pDispatchTask->IsCanceled())
        {
            _pDispatchTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
        }
        else
        {
            _pResumeTaskRunner->PostTaskInternal(std::move(_pDispatchTask));
        }
    }
}
