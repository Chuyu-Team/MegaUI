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

#ifndef DECLSPEC_NOVTABLE
#define DECLSPEC_NOVTABLE
#endif

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            typedef void(__YYAPI* TaskRunnerSimpleCallback)(void* _pUserData);

            enum class TaskRunnerStyle
            {
                None = 0,
                // 拥有固定线程，没有此标准表示实际指向物理线程可能随时变化。
                FixedThread = 0x00000001,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);
            
            enum class ThreadWorkEntryStyle
            {
                None = 0,
                // 任务同步进行。
                Sync = 0x00000001,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(ThreadWorkEntryStyle);

            struct ThreadWorkEntry
            {
                // 此内存的引用计数
                uint32_t uRef;

                ThreadWorkEntryStyle fStyle;

                // 操作结果，任务可能被取消。
                HRESULT hr;

                ThreadWorkEntry(ThreadWorkEntryStyle _eStyle);

                virtual ~ThreadWorkEntry() = default;

                uint32_t __YYAPI AddRef();

                uint32_t __YYAPI Release();

                virtual void __YYAPI DoWork() = 0;

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
                bool __YYAPI Wait(_In_ uint32_t _uMilliseconds = uint32_max);
            };

            template<typename WorkEntryType>
            struct ThreadWorkEntryImpl
                : public ThreadWorkEntry
                , public WorkEntryType
            {
                template<typename... Args>
                ThreadWorkEntryImpl(ThreadWorkEntryStyle _eStyle, Args&&... _args)
                    : ThreadWorkEntry(_eStyle)
                    , WorkEntryType {std::forward<Args>(_args)...}
                {
                }

                ~ThreadWorkEntryImpl() override = default;

                virtual void __YYAPI DoWork() override
                {
                    WorkEntryType::operator()();
                }
            };

            // 按顺序执行的Task（不一定绑定固定物理线程，只保证任务串行）
            class DECLSPEC_NOVTABLE SequencedTaskRunner
            {
            public:
#ifndef _MSC_VER
                virtual ~SequencedTaskRunner()
                {
                }
#endif
                /// <summary>
                /// 从线程池创建一个TaskRunner，后续PostTask提交后将保持串行。注意：不保证保证是否同一个线程，仅保证任务串行！
                /// </summary>
                /// <returns>返回TaskRunner指针，函数几乎不会失败，但是如果内存不足，那么将返回 nullptr。</returns>
                static RefPtr<SequencedTaskRunner> __YYAPI Create();

                /// <summary>
                /// 获取当前任务的 SequencedTaskRunner
                /// </summary>
                /// <returns></returns>
                static RefPtr<SequencedTaskRunner> __YYAPI GetCurrent();

                virtual uint32_t __YYAPI AddRef() = 0;

                virtual uint32_t __YYAPI Release() = 0;

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
                    auto _pThreadWorkEntry = RefPtr<ThreadWorkEntryImpl<LambdaCallback>>::Create(ThreadWorkEntryStyle::None, std::move(_pfnLambdaCallback));
                    if (!_pThreadWorkEntry)
                        return E_OUTOFMEMORY;

                    return PushThreadWorkEntry(_pThreadWorkEntry);
                }

                /// <summary>
                /// 创建一个异步可 co_await 任务。
                /// </summary>
                /// <param name="_pfnLambdaCallback">需要异步执行的 Lambda 表达式</param>
                /// <returns>awaitable</returns>
                template<typename LambdaCallback>
                auto __YYAPI AsyncTask(_In_ LambdaCallback&& _pfnLambdaCallback)
                {
                    struct WorkEntryType
                    {
                        // 这个任务完成后重新回到此 TaskRunner
                        RefPtr<SequencedTaskRunner> pResumeTaskRunner;
                        // 0，表示尚未设置，-1表示任务完成。
                        intptr_t hHandle;
                        // 需要异步执行的 Callback，注意放在结构体末尾，便于编译器重复代码合并
                        LambdaCallback pfnLambdaCallback;

                        void operator()()
                        {
                            pfnLambdaCallback();

                            auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);

                            if (_hHandle)
                            {
                                // 如果 pResumeTaskRunner == nullptr，目标不属于任何一个 SequencedTaskRunner，这很可能任务不关下是否需要串行
                                // 如果 pResumeTaskRunner == SequencedTaskRunner::GetCurrent()，这没有道理进行 PostTask，徒增开销。
                                if (pResumeTaskRunner == nullptr || pResumeTaskRunner == YY::Base::Threading::SequencedTaskRunner::GetCurrent())
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

                    using ThreadWorkEntry = ThreadWorkEntryImpl<WorkEntryType>;

                    auto _pCurrent = YY::Base::Threading::SequencedTaskRunner::GetCurrent();
                    auto _pThreadWorkEntry = RefPtr<ThreadWorkEntry>::Create(ThreadWorkEntryStyle::None, _pCurrent.Get(), 0, std::move(_pfnLambdaCallback));
                    if (!_pThreadWorkEntry)
                        throw Exception();

                    struct awaitable
                    {
                    private:
                        RefPtr<ThreadWorkEntry> pThreadWorkEntry;

                    public:
                        awaitable(RefPtr<ThreadWorkEntry> _pThreadWorkEntry)
                            : pThreadWorkEntry(std::move(_pThreadWorkEntry))
                        {
                        }

                        awaitable(awaitable&&) = default;

                        bool await_ready() noexcept
                        {
                            return pThreadWorkEntry->hHandle == /*hReadyHandle*/ (intptr_t)-1;
                        }

                        bool await_suspend(std::coroutine_handle<> _hHandle) noexcept
                        {
                            return YY::Base::Sync::CompareExchange(&pThreadWorkEntry->hHandle, (intptr_t)_hHandle.address(), 0) == 0;
                        }

                        void await_resume() noexcept
                        {
                        }
                    };

                    auto _hr = PushThreadWorkEntry(_pThreadWorkEntry);
                    if (FAILED(_hr))
                        throw Exception();

                    return awaitable(std::move(_pThreadWorkEntry));
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
                virtual HRESULT __YYAPI PushThreadWorkEntry(_In_ ThreadWorkEntry* _pWorkEntry) = 0;
            };

            // 任务串行，拥有固定线程的Task
            class DECLSPEC_NOVTABLE ThreadTaskRunner : public SequencedTaskRunner
            {
            public:

                /// <summary>
                /// 获取绑定的线程Id。
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;
                
                /// <summary>
                /// 获取当前线程绑定的TaskRunner。
                /// </summary>
                /// <returns>
                /// 如果在返回 nullptr，该线程可能属于 SequencedTaskRunner，没有固定物理线程。也可能 RunUIMessageLoop 尚未调用。
                /// </returns>
                static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

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
        } // namespace Threading
    }     // namespace Base

    using namespace YY::Base::Threading;
} // namespace YY

#pragma pack(push, pop)
