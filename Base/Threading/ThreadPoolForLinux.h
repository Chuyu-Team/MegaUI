#pragma once
#include <pthread.h>

#include <Base/ErrorCode.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Sync/InterlockedSingleLinkedList.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Threading
{
    typedef void(__YYAPI* ThreadPoolSimpleCallback)(void* _pUserData);

    struct ThreadPoolTaskEntry
    {
        ThreadPoolSimpleCallback pfnCallback = nullptr;
        void* pUserData = nullptr;
    };

    class ThreadPool;

    struct ThreadInfoEntry
    {
        ThreadInfoEntry* pNext = nullptr;
        pthread_t hThread = 0;
        ThreadPoolSimpleCallback pfnCallback = nullptr;
        void* pUserData = nullptr;
        ThreadPool* pThreadPool = nullptr;
    };

    class ThreadPool
    {
    private:
        InterlockedSingleLinkedList<ThreadInfoEntry> oIdleThreadQueue;
        InterlockedQueue<ThreadPoolTaskEntry> oPendingTaskQueue;

        volatile uint32_t uThreadCount = 0;

        constexpr ThreadPool() = default;

    public:

        HRESULT __YYAPI ExecuteTask(_In_ ThreadPoolSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) noexcept;

        template<typename Task>
        static HRESULT __YYAPI PostTaskInternalWithoutAddRef(_In_ Task* _pTask) noexcept
        {
            return Get()->ExecuteTask(
                [](_In_ void* _pUserData)
                {
                    auto _pTask = reinterpret_cast<Task*>(_pUserData);
                    _pTask->operator()();
                },
                _pTask);
        }

        template<typename Task>
        static HRESULT __YYAPI PostTaskInternal(_In_ Task* _pTask) noexcept
        {
            _pTask->AddRef();
            auto _hr = Get()->ExecuteTask(
                [](_In_ void* _pUserData)
                {
                    auto _pTask = reinterpret_cast<Task*>(_pUserData);
                    _pTask->operator()();
                    _pTask->Release();
                },
                _pTask);

            if (FAILED(_hr))
            {
                _pTask->Release();
            }
            return _hr;
        }

    private:
        void* TaskExecuteRoutine(ThreadInfoEntry* _pThread) noexcept;

        static _Ret_notnull_ ThreadPool* __YYAPI Get() noexcept;
    };
}

namespace YY
{
    using namespace YY::Base::Threading;
}

#pragma pack(pop)
