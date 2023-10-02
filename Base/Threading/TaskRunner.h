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
        // ��֤�ύ��������
        Sequenced = 0x00000001,
        // ӵ�й̶��̣߳�û�д˱�׼��ʾʵ��ָ�������߳̿�����ʱ�仯��
        FixedThread = 0x00000002,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);
            
    enum class TaskEntryStyle
    {
        None = 0,
        // ����ͬ�����С�
        Sync = 0x00000001,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskEntryStyle);

    struct TaskEntry : public RefValue
    {
        TaskEntryStyle fStyle;

        // ���������������ܱ�ȡ����
        HRESULT hr;

        TaskEntry(TaskEntryStyle _eStyle);

        TaskEntry(const TaskEntry&) = delete;
        TaskEntry& operator=(const TaskEntry&) = delete;

        virtual void __YYAPI operator()() = 0;

        /// <summary>
        /// ���ô�����룬��������صȴ��ߡ�
        /// </summary>
        /// <param name="_hrCode">�����˳����롣</param>
        /// <returns></returns>
        void __YYAPI Wakeup(_In_ HRESULT _hrCode);

        /// <summary>
        /// �ȴ���������ɡ�
        /// </summary>
        /// <param name="_uMilliseconds">��Ҫ�ȴ��ĺ�������</param>
        /// <returns></returns>
        bool __YYAPI Wait(_In_ uint32_t _uMilliseconds = UINT32_MAX);
    };

    /*class ThreadPool : public RefValue
    {
    public:

    };*/
    class TaskRunnerDispatchImplByIoCompletionImpl;

    // ͨ������ִ�����ĳ����
    class TaskRunner : public RefValue
    {
        friend TaskRunnerDispatchImplByIoCompletionImpl;

    public:

        /// <summary>
        /// ��ȡ�����ߵ� TaskRunner
        /// </summary>
        /// <returns>
        /// ������� nullptr�����ܵ�ǰ���������̳߳أ�Ҳ���������ⲿ�̡߳�
        /// </returns>
        static RefPtr<TaskRunner> __YYAPI GetCurrent();

        /// <summary>
        /// ���� TaskRunner ��ΨһId��ע�⣬�ⲻ���߳�Id��
        /// </summary>
        /// <returns></returns>
        virtual uint32_t __YYAPI GetId() = 0;

        virtual TaskRunnerStyle __YYAPI GetStyle() = 0;

        /// <summary>
        /// �������첽ִ�С�
        /// </summary>
        /// <param name="_pfnCallback"></param>
        /// <param name="_pUserData"></param>
        /// <returns></returns>
        HRESULT __YYAPI PostTask(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);

        /// <summary>
        /// �������첽ִ�С�
        /// </summary>
        /// <param name="pfnLambdaCallback">��Ҫ�첽ִ�е�Lambda���ʽ��</param>
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
        /// ����һ���첽�� co_await ����
        /// </summary>
        /// <param name="_pfnLambdaCallback">��Ҫ�첽ִ�е� Lambda ���ʽ</param>
        /// <returns>awaitable</returns>
        template<typename LambdaCallback>
        auto __YYAPI AsyncTask(_In_ LambdaCallback&& _pfnLambdaCallback)
        {
            struct AsyncTaskEntry : public TaskEntry
            {
                // ���������ɺ����»ص��� TaskRunner
                RefPtr<TaskRunner> pResumeTaskRunner;
                // 0����ʾ��δ���ã�-1��ʾ������ɡ�
                intptr_t hHandle;
                // ��Ҫ�첽ִ�е� Callback��ע����ڽṹ��ĩβ�����ڱ������ظ�����ϲ�
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
                        // ��� pResumeTaskRunner == nullptr��Ŀ�겻�����κ�һ�� SequencedTaskRunner����ܿ������񲻹����Ƿ���Ҫ����
                        // ��� pResumeTaskRunner == SequencedTaskRunner::GetCurrent()����û�е������ PostTask��ͽ��������
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
        /// ͬ��ִ��Callback�����ؾ��棺��������������ߣ�������������������
        /// </summary>
        /// <param name="_pfnCallback"></param>
        /// <param name="_pUserData"></param>
        /// <returns></returns>
        HRESULT __YYAPI SendTask(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);
                
        /// <summary>
        /// ͬ��ִ��һ��Lambda���ʽ�����ؾ��棺��������������ߣ�������������������
        /// </summary>
        /// <param name="_pfnLambdaCallback">��Ҫͬ��ִ�е� Lambda ���ʽ��</param>
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

    // ��˳��ִ�е�Task����һ���󶨹̶������̣߳�ֻ��֤�����У�
    class SequencedTaskRunner : public TaskRunner
    {
    public:
        /// <summary>
        /// ��ȡ��ǰ���������� SequencedTaskRunner��
        /// </summary>
        /// <returns>
        /// ������� nullptr�����ܵ�ǰ���� SequencedTaskRunner�������̳߳أ�Ҳ���������ⲿ�������̣߳��������κ�һ�� TaskRunner��
        /// </returns>
        static RefPtr<SequencedTaskRunner> __YYAPI GetCurrent();
        
        /// <summary>
        /// ���̳߳ش���һ��TaskRunner������PostTask�ύ�󽫱��ִ��С�ע�⣺����֤��֤�Ƿ�ͬһ���̣߳�����֤�����У�
        /// </summary>
        /// <returns>����TaskRunnerָ�룬������������ʧ�ܣ���������ڴ治�㣬��ô������ nullptr��</returns>
        static RefPtr<SequencedTaskRunner> __YYAPI Create();

    };

    // �����в���ӵ�й̶��̵߳�����ִ����
    class ThreadTaskRunner : public SequencedTaskRunner
    {
    public:
        /// <summary>
        /// ���̳߳�ȡһ���̲߳������TaskRunner�󶨣�ʼ�ձ�֤����������ͬһ���߳���ִ�С�
        /// ���ThreadTaskRunner�������ڽ�������̹߳黹�̳߳ء�
        /// ��ܰ��ʾ�����ȿ���ʹ�� `SequencedTaskRunner::Create()`��ThreadTaskRunner�Ĵ�������Ϊ�߰���
        /// </summary>
        /// <returns></returns>
        // static RefPtr<ThreadTaskRunner> __YYAPI Create();

        /// <summary>
        /// ��ȡ��ǰ�̰߳󶨵�TaskRunner��
        /// </summary>
        /// <returns>
        /// ����ڷ��� nullptr�����߳̿������� SequencedTaskRunner��û�й̶������̡߳�Ҳ���� RunUIMessageLoop ��δ���á�
        /// </returns>
        static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

        /// <summary>
        /// ��ȡ�󶨵��߳�Id��
        /// </summary>
        /// <returns></returns>
        virtual uint32_t __YYAPI GetThreadId() = 0;
                

        /// <summary>
        /// ����UI�߳�ר����Ϣѭ����
        /// * ���߳�������Ϣѭ�����������ʹ�� ThreadTaskRunner::GetCurrent();
        /// * ��� RunUIMessageLoop �˳���������PostTask������ʧ�ܡ�
        /// </summary>
        /// <param name="_pfnCallback">����ѭ��֮ǰ���еĺ������ã�Callback�����ڼ����ʹ��`ThreadTaskRunner::GetCurrent()`��</param>
        /// <param name="_pUserData">�������ݸ� _pfnCallback �� _pUserData</param>
        /// <returns>��Ϣѭ���˳����롣</returns>
        static uintptr_t __YYAPI RunUIMessageLoop(_In_opt_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData);

        /// <summary>
        /// ����UI�߳�ר����Ϣѭ����
        /// * ���߳�������Ϣѭ�����������ʹ�� ThreadTaskRunner::GetCurrent();
        /// * ��� RunUIMessageLoop �˳���������PostTask������ʧ�ܡ�
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
