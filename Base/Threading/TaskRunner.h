#pragma once
#include <type_traits>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Base/YY.h>
#include <Base/Memory/RefPtr.h>

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
                // 串行，没有此标志则代表并行。
                Sequenced = 0x00000001,
                // 拥有固定线程，没有此标准表示实际指向线程可能随时变化。
                FixedThread = 0x00000002,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);

            class DECLSPEC_NOVTABLE TaskRunner
            {
            public:
#ifndef _MSC_VER
                virtual ~TaskRunner()
                {
                }
#endif

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
                virtual HRESULT __YYAPI Async(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) = 0;

                /// <summary>
                /// 将任务异步执行。
                /// </summary>
                /// <param name="pfnLambdaCallback">需要异步执行的Lambda表达式。</param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Async(_In_ std::function<void()>&& _pfnLambdaCallback) = 0;

                /// <summary>
                /// 将任务同步执行。
                /// </summary>
                /// <param name="_pfnCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Sync(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) = 0;
                
                template<class _FunType>
                HRESULT __YYAPI Sync(_FunType&& _Fun)
                {
                    return Sync(
                        [](void* _pUserData)
                        {
                            auto& _Fun = *(_FunType*)_pUserData;
                            _Fun();
                        },
                        (void*)&_Fun);
                }
            };

            // 按顺序执行的Task（不一定绑定固定物理线程，只保证任务串行）
            class DECLSPEC_NOVTABLE SequencedTaskRunner : public TaskRunner
            {
            public:
                // 从线程池创建一个 TaskRunner，注意任务执行时只保证不保证始终是同一个线程，仅保证任务串行。
                static RefPtr<SequencedTaskRunner> __YYAPI Create();
            };

            // 拥有固定线程的Task
            class DECLSPEC_NOVTABLE ThreadTaskRunner : public SequencedTaskRunner
            {
            public:
                static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

                /// <summary>
                /// 获取绑定的线程Id。
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;

                virtual uintptr_t __YYAPI RunMessageLoop() = 0;
            };

            // 并行执行的Task，不保证任务按顺序执行
            class DECLSPEC_NOVTABLE ParallelTaskRunner : public TaskRunner
            {
            public:
            };

        } // namespace Threading
    }     // namespace Base

    using namespace YY::Base::Threading;
} // namespace YY

#pragma pack(push, pop)
