#pragma once
#include <type_traits>
#include <functional>
#include <coroutine>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Base/YY.h>
#include <Base/Memory/RefPtr.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Exception.h>
#include <MegaUI/Base/ErrorCode.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Threading
{
    typedef void(__YYAPI* TaskRunnerSimpleCallback)(void* _pUserData);

    enum class TaskRunnerStyle
    {
        None = 0,
        // 保证提交的任务串行
        Sequenced = 0x00000001,
        // 拥有固定线程，没有此标准表示实际指向物理线程可能随时变化。
        FixedThread = 0x00000002,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);
            
    enum class TaskEntryStyle
    {
        None = 0,
        // 任务同步进行。
        Sync = 0x00000001,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskEntryStyle);

    struct TaskEntry : public RefValue
    {
        TaskEntryStyle fStyle;

        // 操作结果，任务可能被取消。
        HRESULT hr;

        TaskEntry(TaskEntryStyle _eStyle);

        TaskEntry(const TaskEntry&) = delete;
        TaskEntry& operator=(const TaskEntry&) = delete;

        virtual void __YYAPI operator()() = 0;

        /// <summary>
        /// 设置错误代码，并唤醒相关等待者。
        /// </summary>
        /// <param name="_hrCode">任务退出代码。</param>
        /// <returns></returns>
        void __YYAPI Wakeup(_In_ HRESULT _hrCode);

        /// <summary>
        /// 等待此任务完成。
        /// </summary>
        /// <param name="_uMilliseconds">需要等待的毫秒数。</param>
        /// <returns></returns>
        bool __YYAPI Wait(_In_ uint32_t _uMilliseconds = UINT32_MAX);
    };

    /*class ThreadPool : public RefValue
    {
    public:

    };*/
    class TaskRunnerDispatchImplByIoCompletionImpl;

    // 通用任务执行器的抽象层
    class TaskRunner : public RefValue
    {
        friend TaskRunnerDispatchImplByIoCompletionImpl;

    public:

        /// <summary>
        /// 获取调用者的 TaskRunner
        /// </summary>
        /// <returns>
        /// 如果返回 nullptr，可能当前调用者是线程池，也可能来自外部线程。
        /// </returns>
        static RefPtr<TaskRunner> __YYAPI GetCurrent();

        /// <summary>
        /// 返回 TaskRunner 的唯一Id，注意，这不是线程Id。
        /// </summary>
        /// <returns></returns>
        virtual uint32_t __YYAPI GetId() = 0;

        virtual TaskRunnerStyle __YYAPI GetStyle() = 0;

        /// <summary>
        /// 将任务异步执行。
        /// </summary>
        /// <param name="_pfnCallback"></param>
        /// <param name="_pUserData"></param>
        /// <returns></returns>
        HRESULT __YYAPI PostTask(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);

        /// <summary>
        /// 将任务异步执行。
        /// </summary>
        /// <param name="pfnLambdaCallback">需要异步执行的Lambda表达式。</param>
        /// <returns></returns>
        template<typename LambdaCallback>
        HRESULT __YYAPI PostTask(_In_ LambdaCallback&& _pfnLambdaCallback)
        {
            struct LambdaTaskEntry : public TaskEntry
            {
                LambdaCallback pfnLambdaCallback;

                LambdaTaskEntry(TaskEntryStyle _eStyle, LambdaCallback _pfnLambdaCallback)
                    : TaskEntry(_eStyle)
                    , pfnLambdaCallback(std::move(_pfnLambdaCallback))
                {
                }

                void __YYAPI operator()() override
                {
                    pfnLambdaCallback();
                }
            };
            auto _pThreadWorkEntry = RefPtr<LambdaTaskEntry>::Create(TaskEntryStyle::None, std::move(_pfnLambdaCallback));
            if (!_pThreadWorkEntry)
                return E_OUTOFMEMORY;

            return PostTaskInternal(_pThreadWorkEntry);
        }

        /// <summary>
        /// 创建一个异步可 co_await 任务。
        /// </summary>
        /// <param name="_pfnLambdaCallback">需要异步执行的 Lambda 表达式</param>
        /// <returns>awaitable</returns>
        template<typename LambdaCallback>
        auto __YYAPI AsyncTask(_In_ LambdaCallback&& _pfnLambdaCallback)
        {
            struct AsyncTaskEntry : public TaskEntry
            {
                // 这个任务完成后重新回到此 TaskRunner
                RefPtr<TaskRunner> pResumeTaskRunner;
                // 0，表示尚未设置，-1表示任务完成。
                intptr_t hHandle;
                // 需要异步执行的 Callback，注意放在结构体末尾，便于编译器重复代码合并
                LambdaCallback pfnLambdaCallback;

                AsyncTaskEntry(TaskEntryStyle _eStyle, RefPtr<TaskRunner> _pResumeTaskRunner, LambdaCallback _pfnLambdaCallback)
                    : TaskEntry(_eStyle)
                    , pResumeTaskRunner(std::move(_pResumeTaskRunner))
                    , hHandle(0)
                    , pfnLambdaCallback(std::move(_pfnLambdaCallback))
                {
                }

                void __YYAPI operator()() override
                {
                    pfnLambdaCallback();

                    auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);

                    if (_hHandle)
                    {
                        // 如果 pResumeTaskRunner == nullptr，目标不属于任何一个 SequencedTaskRunner，这很可能任务不关下是否需要串行
                        // 如果 pResumeTaskRunner == SequencedTaskRunner::GetCurrent()，这没有道理进行 PostTask，徒增开销。
                        if (pResumeTaskRunner == nullptr || pResumeTaskRunner == YY::Base::Threading::TaskRunner::GetCurrent())
                        {
                            std::coroutine_handle<>::from_address(_hHandle).resume();
                        }
                        else
                        {
                            pResumeTaskRunner->PostTask(
                                [](void* _hHandle)
                                {
                                    std::coroutine_handle<>::from_address(_hHandle).resume();
                                },
                                (void*)_hHandle);
                        }
                    }
                }
            };

            auto _pCurrent = YY::Base::Threading::TaskRunner::GetCurrent();
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create(TaskEntryStyle::None, std::move(_pCurrent), std::move(_pfnLambdaCallback));
            if (!_pAsyncTaskEntry)
                throw Exception();

            struct awaitable
            {
            private:
                RefPtr<AsyncTaskEntry> pAsyncTaskEntry;

            public:
                awaitable(RefPtr<AsyncTaskEntry> _pAsyncTaskEntry)
                    : pAsyncTaskEntry(std::move(_pAsyncTaskEntry))
                {
                }

                awaitable(awaitable&&) = default;

                bool await_ready() noexcept
                {
                    return pAsyncTaskEntry->hHandle == /*hReadyHandle*/ (intptr_t)-1;
                }

                bool await_suspend(std::coroutine_handle<> _hHandle) noexcept
                {
                    return YY::Base::Sync::CompareExchange(&pAsyncTaskEntry->hHandle, (intptr_t)_hHandle.address(), 0) == 0;
                }

                void await_resume() noexcept
                {
                }
            };

            auto _hr = PostTaskInternal(_pAsyncTaskEntry);
            if (FAILED(_hr))
                throw Exception();

            return awaitable(std::move(_pAsyncTaskEntry));
        }

        /// <summary>
        /// 同步执行Callback。严重警告：这可能阻塞调用者，甚至产生死锁！！！
        /// </summary>
        /// <param name="_pfnCallback"></param>
        /// <param name="_pUserData"></param>
        /// <returns></returns>
        HRESULT __YYAPI SendTask(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);
                
        /// <summary>
        /// 同步执行一段Lambda表达式。严重警告：这可能阻塞调用者，甚至产生死锁！！！
        /// </summary>
        /// <param name="_pfnLambdaCallback">需要同步执行的 Lambda 表达式。</param>
        /// <returns></returns>
        template<typename LambdaCallback>
        HRESULT __YYAPI SendTask(LambdaCallback&& _pfnLambdaCallback)
        {
            return SendTask(
                [](void* _pUserData)
                {
                    auto& _pfnLambdaCallback = *(LambdaCallback*)_pUserData;
                    _pfnLambdaCallback();
                },
                (void*)&_pfnLambdaCallback);
        }

    protected:
        virtual HRESULT __YYAPI PostTaskInternal(_In_ RefPtr<TaskEntry> _pTask) = 0;
    };

    // 按顺序执行的Task（不一定绑定固定物理线程，只保证任务串行）
    class SequencedTaskRunner : public TaskRunner
    {
    public:
        /// <summary>
        /// 获取当前调用所属的 SequencedTaskRunner。
        /// </summary>
        /// <returns>
        /// 如果返回 nullptr，可能当前不是 SequencedTaskRunner，来自线程池，也可能来自外部创建的线程，不属于任何一个 TaskRunner。
        /// </returns>
        static RefPtr<SequencedTaskRunner> __YYAPI GetCurrent();
        
        /// <summary>
        /// 从线程池创建一个TaskRunner，后续PostTask提交后将保持串行。注意：不保证保证是否同一个线程，仅保证任务串行！
        /// </summary>
        /// <returns>返回TaskRunner指针，函数几乎不会失败，但是如果内存不足，那么将返回 nullptr。</returns>
        static RefPtr<SequencedTaskRunner> __YYAPI Create();

    };

    // 任务串行并且拥有固定线程的任务执行器
    class ThreadTaskRunner : public SequencedTaskRunner
    {
    public:
        /// <summary>
        /// 从线程池取一个线程并且与该TaskRunner绑定，始终保证后续任务在同一个线程中执行。
        /// 如果ThreadTaskRunner生命周期解除，则将线程归还线程池。
        /// 温馨提示：优先考虑使用 `SequencedTaskRunner::Create()`，ThreadTaskRunner的创建开销为高昂。
        /// </summary>
        /// <returns></returns>
        // static RefPtr<ThreadTaskRunner> __YYAPI Create();

        /// <summary>
        /// 获取当前线程绑定的TaskRunner。
        /// </summary>
        /// <returns>
        /// 如果在返回 nullptr，该线程可能属于 SequencedTaskRunner，没有固定物理线程。也可能 RunUIMessageLoop 尚未调用。
        /// </returns>
        static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

        /// <summary>
        /// 获取绑定的线程Id。
        /// </summary>
        /// <returns></returns>
        virtual uint32_t __YYAPI GetThreadId() = 0;
                

        /// <summary>
        /// 运行UI线程专属消息循环。
        /// * 主线程运行消息循环后才能正常使用 ThreadTaskRunner::GetCurrent();
        /// * 如果 RunUIMessageLoop 退出，后续的PostTask等请求将失败。
        /// </summary>
        /// <param name="_pfnCallback">启动循环之前进行的函数调用，Callback发生期间可以使用`ThreadTaskRunner::GetCurrent()`。</param>
        /// <param name="_pUserData">后续传递给 _pfnCallback 的 _pUserData</param>
        /// <returns>消息循环退出代码。</returns>
        static uintptr_t __YYAPI RunUIMessageLoop(_In_opt_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);

        /// <summary>
        /// 运行UI线程专属消息循环。
        /// * 主线程运行消息循环后才能正常使用 ThreadTaskRunner::GetCurrent();
        /// * 如果 RunUIMessageLoop 退出，后续的PostTask等请求将失败。
        /// </summary>
        /// <param name="_pfnLambdaCallback"></param>
        /// <returns></returns>
        template<typename LambdaCallback>
        static uintptr_t __YYAPI RunUIMessageLoop(_In_ LambdaCallback&& _pfnLambdaCallback)
        {
            return RunUIMessageLoop(
                [](void* _pUserData)
                {
                    auto& _pfnLambdaCallback = *(LambdaCallback*)_pUserData;
                    _pfnLambdaCallback();
                },
                (void*)&_pfnLambdaCallback);
        }
    };
} // namespace YY::Base::Threading;

namespace YY
{
    using namespace YY::Base::Threading;
}

#pragma pack(push, pop)
