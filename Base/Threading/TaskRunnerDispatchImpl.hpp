#pragma once
#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/WeakPtr.h>

#pragma pack(push, __YY_PACKING)

/*
DispatchTaskRunner 仅处理调度任务（比如定时器、异步IO），无法参执行其他Task。

*/

namespace YY::Base::Threading
{
    struct DispatchEntry
        : public TaskEntry
        , public OVERLAPPED
    {
        WeakPtr<SequencedTaskRunner> pResumeTaskRunner;

        DispatchEntry()
            : TaskEntry(TaskEntryStyle::None)
            , OVERLAPPED {}
        {
        }
    };

    class TaskRunnerDispatchImplByIoCompletionImpl
    {
    private:
        HANDLE hIoCompletionPort;
        uint32_t fFlags;

        TaskRunnerDispatchImplByIoCompletionImpl()
            : fFlags(0u)
        {
            hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1u);
        }

    public:
        TaskRunnerDispatchImplByIoCompletionImpl(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

        TaskRunnerDispatchImplByIoCompletionImpl& operator=(const TaskRunnerDispatchImplByIoCompletionImpl&) = delete;

        ~TaskRunnerDispatchImplByIoCompletionImpl()
        {
            if (hIoCompletionPort)
            {
                CloseHandle(hIoCompletionPort);
            }
        }

        static _Ret_notnull_ TaskRunnerDispatchImplByIoCompletionImpl* __YYAPI Get()
        {
            static TaskRunnerDispatchImplByIoCompletionImpl s_Dispatch;
            return &s_Dispatch;
        }

        bool __YYAPI BindIO(_In_ HANDLE _hHandle)
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

        void Weakup()
        {
            if (!Sync::BitSet(&fFlags, 0))
            {
                auto _bRet = TrySubmitThreadpoolCallback(
                    [](_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID Context) -> void
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

        void ExecuteTaskRunner()
        {
            OVERLAPPED_ENTRY _oCompletionPortEntries[16];
            for (;;)
            {
                ULONG _uNumEntriesRemoved = 0;

                GetQueuedCompletionStatusEx(hIoCompletionPort, _oCompletionPortEntries, std::size(_oCompletionPortEntries), &_uNumEntriesRemoved, UINT32_MAX, FALSE);

                for (ULONG _uIndex = 0; _uIndex != _uNumEntriesRemoved; ++_uIndex)
                {
                    auto _pDispatchTask = static_cast<DispatchEntry*>(_oCompletionPortEntries[_uIndex].lpOverlapped);
                    if (!_pDispatchTask)
                        continue;


                    auto _pResumeTaskRunner = _pDispatchTask->pResumeTaskRunner.Get();
                    _pDispatchTask->pResumeTaskRunner = nullptr;
                    if (_pResumeTaskRunner)
                    {
                        _pResumeTaskRunner->PostTaskInternal(_pDispatchTask);
                    }
                    else
                    {
                        _pDispatchTask->Wakeup(YY::Base::HRESULT_From_LSTATUS(ERROR_CANCELLED));
                    }
                    _pDispatchTask->Release();
                }
            }
        }
    };
} // namespace YY

#pragma pack(pop)
