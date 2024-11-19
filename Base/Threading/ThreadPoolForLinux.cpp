#include "pch.h"
#include "ThreadPoolForLinux.h"

#include <signal.h>

#include <Base/Sync/Sync.h>
#include <Base/Memory/Alloc.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY::Base::Threading
{
    constexpr uint32_t MinThreadsCount = 10;
    constexpr uint32_t MaxThreadsCount = 500;

    HRESULT __YYAPI ThreadPool::ExecuteTask(ThreadPoolSimpleCallback _pfnCallback, void* _pUserData) noexcept
    {
        auto _pThread = oIdleThreadQueue.Pop();
        if (_pThread)
        {
            _pThread->pUserData = _pUserData;
            _pThread->pfnCallback = _pfnCallback;

            pthread_kill(_pThread->hThread, SIGUSR1);
            return S_OK;
        }

        for (auto uCurrentThreadCount = uThreadCount;;)
        {
            if (uCurrentThreadCount >= MaxThreadsCount)
            {
                // 达到线程创建上限
                auto _pTask = New<ThreadPoolTaskEntry>(_pfnCallback, _pUserData);
                if(!_pTask)
                    return E_OUTOFMEMORY;

                oPendingTaskQueue.Push(_pTask);
                return S_OK;
            }

            const auto _uLast = Sync::CompareExchange(&uThreadCount, uCurrentThreadCount + 1, uCurrentThreadCount);
            if (_uLast == uCurrentThreadCount)
            {
                break;
            }

            uCurrentThreadCount = _uLast;
        }

        _pThread = New<ThreadInfoEntry>();
        if (!_pThread)
        {
            Sync::Decrement(&uThreadCount);
            return E_OUTOFMEMORY;
        }

        _pThread->pfnCallback = _pfnCallback;
        _pThread->pUserData = _pUserData;
        _pThread->pThreadPool = this;

        const auto _iResult = pthread_create(&_pThread->hThread, nullptr,
            [](void* _pUserData) -> void*
            {
                auto _pThread = (ThreadInfoEntry*)_pUserData;
                return _pThread->pThreadPool->TaskExecuteRoutine(_pThread);
            }, _pThread);

        if (_iResult == 0)
        {
            return S_OK;
        }

        Sync::Decrement(&uThreadCount);
        Delete(_pThread);

        // TODO: 错误代码转换
        return E_FAIL;
    }

    void* ThreadPool::TaskExecuteRoutine(ThreadInfoEntry* _pThread) noexcept
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        int signo;

        for (;;)
        {
            if (_pThread->pfnCallback)
            {
                _pThread->pfnCallback(_pThread->pUserData);
                _pThread->pfnCallback = nullptr;
                _pThread->pUserData = nullptr;
            }

            for (;;)
            {
                auto _pTask = oPendingTaskQueue.Pop();
                if (!_pTask)
                    break;

                _pTask->pfnCallback(_pTask->pUserData);

                Delete(_pTask);
            }


            _pThread->pThreadPool->oIdleThreadQueue.Push(_pThread);

            pthread_sigmask(SIG_SETMASK, &set, nullptr);
            if (sigwait(&set, &signo) != 0)
            {
                break;
            }

            if (signo != SIGUSR1)
                break;
        }

        Sync::Decrement(&uThreadCount);
        Delete(_pThread);
        pthread_detach(pthread_self());
        return nullptr;
    }
    
    ThreadPool* __YYAPI ThreadPool::Get() noexcept
    {
        static ThreadPool s_ThreadPool;
        return &s_ThreadPool;
    }
}
