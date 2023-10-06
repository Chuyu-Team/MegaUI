#pragma once
#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/WeakPtr.h>
#include <Base/Time/TickCount.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Containers/BitMap.h>


#pragma pack(push, __YY_PACKING)

/*
TaskRunnerDispatch 仅处理调度任务（比如定时器、异步IO），无法参执行其他Task。

*/

namespace YY::Base::Threading
{
    struct IoDispatchEntry
        : public DispatchEntry
        , public OVERLAPPED

    {
        IoDispatchEntry()
            : OVERLAPPED {}
        {
        }
    };

    struct TimingWheelSimpleList
    {
        DelayDispatchEntry* pFirst = nullptr;
        DelayDispatchEntry* pLast = nullptr;

        TimingWheelSimpleList() = default;
        
        TimingWheelSimpleList(const TimingWheelSimpleList&) = delete;
        TimingWheelSimpleList& operator=(const TimingWheelSimpleList&) = delete;
        TimingWheelSimpleList& operator=(TimingWheelSimpleList&&) = default;


        _Ret_maybenull_ DelayDispatchEntry* Pop() noexcept
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

        void Push(_In_ DelayDispatchEntry* _pEntry) noexcept
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

        bool IsEmpty() const noexcept
        {
            return pFirst == nullptr;
        }
    };

    class TaskRunnerDispatchImplByIoCompletionImpl
    {
    private:
        HANDLE hIoCompletionPort;
        HANDLE hWeakupEvent;
        uint32_t fFlags;
        TickCount<TimePrecise::Millisecond> uTimingWheelCurrentTick;

        // 时间轮: UI库一般定时数百毫秒或者几分钟所以轮子设计时越容纳1小时左右，超过1小时的全部进入 arrTimingWheelOthers
        // 步进 10ms，0ms ~ 128'0 ms
        TimingWheelSimpleList arrTimingWheel1[128];
        // 步进 128'0 ms
        TimingWheelSimpleList arrTimingWheel2[64];
        TimingWheelSimpleList arrTimingWheel3[64];
        // 其余：655'360ms + 的元素
        TimingWheelSimpleList arrTimingWheelOthers;
        // 等待加入的列表
        InterlockedQueue<DelayDispatchEntry> arrTimerDispatchPending;

        // TimingWheel的位图缓存，加速轮子的遍历过程
        BitMap<sizeof(arrTimingWheel1) / sizeof(arrTimingWheel1[0])> oTimingWheel1BitMap;
        BitMap<sizeof(arrTimingWheel2) / sizeof(arrTimingWheel2[0])> oTimingWheel2BitMap;
        BitMap<sizeof(arrTimingWheel3) / sizeof(arrTimingWheel3[0])> oTimingWheel3BitMap;

        TaskRunnerDispatchImplByIoCompletionImpl();

    public:
        TaskRunnerDispatchImplByIoCompletionImpl(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

        TaskRunnerDispatchImplByIoCompletionImpl& operator=(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

        ~TaskRunnerDispatchImplByIoCompletionImpl();

        static _Ret_notnull_ TaskRunnerDispatchImplByIoCompletionImpl* __YYAPI Get() noexcept;

        bool __YYAPI BindIO(_In_ HANDLE _hHandle) const noexcept;

        void __YYAPI Weakup();

        void __YYAPI AddTimer(_In_ RefPtr<DelayDispatchEntry> _pDispatchTask) noexcept;

    private:
        void __YYAPI ExecuteTaskRunner();

        void __YYAPI Dispatch(_In_ RefPtr<DispatchEntry> _pTask);

        void __YYAPI Dispatch(_In_ TimingWheelSimpleList& _oTimingWheelList);

        void __YYAPI FetchTimingWheel1(uint64_t _uTimingWheel2Index, TickCount<TimePrecise::Millisecond> _oCurrent);

        void __YYAPI FetchTimingWheel2(uint64_t _uTimingWheel3Index, TickCount<TimePrecise::Millisecond> _oCurrent);

        void __YYAPI FetchTimingWheel3(TickCount<TimePrecise::Millisecond> _oCurrent);

        void __YYAPI DispatchTimingWheel(TickCount<TimePrecise::Millisecond> _oCurrent);


        void __YYAPI AddTimingWheel(_In_ RefPtr<DelayDispatchEntry> _pDispatchTask) noexcept;

        uint32_t __YYAPI GetMinimumWaitTime() const noexcept;
    };
} // namespace YY

#pragma pack(pop)
