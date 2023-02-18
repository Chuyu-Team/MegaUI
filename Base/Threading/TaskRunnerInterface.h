#pragma once
#include <type_traits>

#include <Windows.h>

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            typedef void(__YYAPI *TaskRunnerCallback)(void* _pUserData);

            enum class TaskRunnerStyle
            {
                None = 0,
                // 串行，没有此标志则代表并行。
                Sequenced = 0x00000001,
                // 拥有固定线程，没有此标准表示实际指向线程可能随时变化。
                FixedThread = 0x00000002,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);

            class DECLSPEC_NOVTABLE ITaskRunner
            {
            public:
                virtual uint32_t __YYAPI AddRef() = 0;

                virtual uint32_t __YYAPI Release() = 0;

                /// <summary>
                /// 返回 ITaskRunner 的唯一 Id，注意，这不是线程Id。
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetId() = 0;

                virtual TaskRunnerStyle __YYAPI GetStyle() = 0;

                /// <summary>
                /// 将任务异步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Async(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) = 0;

                /// <summary>
                /// 将任务同步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Sync(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) = 0;
            };

            // 按顺序执行的Task（不一定绑定固定物理线程，只保证任务串行）
            class DECLSPEC_NOVTABLE ISequencedTaskRunner : public ITaskRunner
            {
            public:
            };
            
            // 拥有固定线程的Task
            class DECLSPEC_NOVTABLE IThreadTaskRunner : public ISequencedTaskRunner
            {
            public:
                /// <summary>
                /// 获取绑定的线程Id。
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;
            };


            // 并行执行的Task，不保证任务按顺序执行
            class DECLSPEC_NOVTABLE IParallelTaskRunner : public ITaskRunner
            {
            public:

            };

        } // namespace Async
    } // namespace Base
} // namespace YY
