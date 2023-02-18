#pragma once

#include <Base/Threading/TaskRunnerInterface.h>

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class TaskRunner
            {
            protected:
                union
                {
                    ITaskRunner* pTaskRunner;

                    ISequencedTaskRunner* pSequencedTaskRunner;

                    IThreadTaskRunner* pThreadTaskRunner;

                    IParallelTaskRunner* pParallelTaskRunner;
                };
                
                TaskRunner(ITaskRunner* _pTask = nullptr)
                    : pTaskRunner(_pTask)
                {
                    if (pTaskRunner)
                        pTaskRunner->AddRef();
                }

            public:
                TaskRunner(const TaskRunner& _Other)
                    : pTaskRunner(_Other.pTaskRunner)
                {
                    if (pTaskRunner)
                        pTaskRunner->AddRef();
                }

                TaskRunner(TaskRunner && _Other) noexcept
                    : pTaskRunner(_Other.pTaskRunner)
                {
                    _Other.pTaskRunner = nullptr;
                }

                ~TaskRunner()
                {
                    if (pTaskRunner)
                        pTaskRunner->Release();
                }

                uint32_t __YYAPI GetId() const
                {
                    return pTaskRunner ? pTaskRunner->GetId() : 0;
                }

                TaskRunnerStyle __YYAPI GetStyle() const
                {
                    return pTaskRunner ? pTaskRunner->GetStyle() : TaskRunnerStyle::None;
                }

                /// <summary>
                /// 将任务异步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                HRESULT __YYAPI Async(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) const
                {
                    if (!pTaskRunner)
                        return E_NOTIMPL;

                    return pTaskRunner->Async(_pCallback, _pUserData);
                }

                /// <summary>
                /// 将任务同步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                HRESULT __YYAPI Sync(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) const
                {
                    if (!pTaskRunner)
                        return E_NOTIMPL;

                    return pTaskRunner->Sync(_pCallback, _pUserData);
                }

                template<class _FunType>
                HRESULT __YYAPI Sync(_FunType&& _Fun) const
                {
                    return Sync([](void* _pUserData)
                        {
                            auto& _Fun = *(_FunType*)_pUserData;
                            _Fun();
                        }, (void*)&_Fun);
                }


                TaskRunner& operator=(const TaskRunner& _Other)
                {
                    if (pTaskRunner != _Other.pTaskRunner)
                    {
                        if (pTaskRunner)
                            pTaskRunner->Release();

                        pTaskRunner = _Other.pTaskRunner;
                        if (pTaskRunner)
                            pTaskRunner->AddRef();
                    }

                    return *this;
                }
            };

            class SequencedTaskRunner : public TaskRunner
            {
            protected:
                SequencedTaskRunner(ISequencedTaskRunner* _pSequencedTask = nullptr)
                    : TaskRunner(_pSequencedTask)
                {
                }

            public:
                SequencedTaskRunner(const TaskRunner& _Other)
                {
                    if (HasFlags(_Other.GetStyle(), TaskRunnerStyle::Sequenced))
                        TaskRunner::operator=(_Other);
                }

                SequencedTaskRunner(const SequencedTaskRunner& _Other)
                    : TaskRunner(_Other.pSequencedTaskRunner)
                {         
                }

                SequencedTaskRunner(SequencedTaskRunner&& _Other) noexcept
                    : TaskRunner(std::move(_Other))
                {
                }

                SequencedTaskRunner& operator=(const SequencedTaskRunner& _Other)
                {
                    TaskRunner::operator=(_Other);

                    return *this;
                }
            };

            class ThreadTaskRunner : public SequencedTaskRunner
            {
            protected:
                ThreadTaskRunner(IThreadTaskRunner* _pThreadTask = nullptr)
                    : SequencedTaskRunner(_pThreadTask)
                {
                }

            public:
                ThreadTaskRunner(const TaskRunner& _Other)
                {
                    if (HasFlags(_Other.GetStyle(), TaskRunnerStyle::FixedThread))
                        TaskRunner::operator=(_Other);
                }

                ThreadTaskRunner(const ThreadTaskRunner& _Other)
                    : SequencedTaskRunner(_Other)
                {
                }

                ThreadTaskRunner(ThreadTaskRunner&& _Other) noexcept
                    : SequencedTaskRunner(std::move(_Other))
                {
                }

                uint32_t __YYAPI GetThreadId()
                {
                    return pThreadTaskRunner ? pThreadTaskRunner->GetThreadId() : 0;
                }

                /// <summary>
                /// 从当前线程获取 ThreadTaskRunner，严重警告：仅支持 来自 CreateThreadTaskRunner的线程 或者 自带消息循环的线程。
                /// </summary>
                /// <returns></returns>
                static ThreadTaskRunner __YYAPI GetCurrentThreadTaskRunner();

                /// <summary>
                /// 创建一个新线程并获取 ThreadTaskRunner
                /// </summary>
                /// <returns></returns>
                // static ThreadTaskRunner __YYAPI CreateThreadTaskRunner();
            };
 
        }
    } // namespace Base

    using namespace YY::Base::Threading;
} // namespace YY

