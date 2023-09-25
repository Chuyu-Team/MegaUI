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
                // ӵ�й̶��̣߳�û�д˱�׼��ʾʵ��ָ�������߳̿�����ʱ�仯��
                FixedThread = 0x00000001,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);
            
            enum class ThreadWorkEntryStyle
            {
                None = 0,
                // ����ͬ�����С�
                Sync = 0x00000001,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(ThreadWorkEntryStyle);

            struct ThreadWorkEntry
            {
                // ���ڴ�����ü���
                uint32_t uRef;

                ThreadWorkEntryStyle fStyle;

                // ���������������ܱ�ȡ����
                HRESULT hr;

                ThreadWorkEntry(ThreadWorkEntryStyle _eStyle);

                virtual ~ThreadWorkEntry() = default;

                uint32_t __YYAPI AddRef();

                uint32_t __YYAPI Release();

                virtual void __YYAPI DoWork() = 0;

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

            // ��˳��ִ�е�Task����һ���󶨹̶������̣߳�ֻ��֤�����У�
            class DECLSPEC_NOVTABLE SequencedTaskRunner
            {
            public:
#ifndef _MSC_VER
                virtual ~SequencedTaskRunner()
                {
                }
#endif
                /// <summary>
                /// ���̳߳ش���һ��TaskRunner������PostTask�ύ�󽫱��ִ��С�ע�⣺����֤��֤�Ƿ�ͬһ���̣߳�����֤�����У�
                /// </summary>
                /// <returns>����TaskRunnerָ�룬������������ʧ�ܣ���������ڴ治�㣬��ô������ nullptr��</returns>
                static RefPtr<SequencedTaskRunner> __YYAPI Create();

                /// <summary>
                /// ��ȡ��ǰ����� SequencedTaskRunner
                /// </summary>
                /// <returns></returns>
                static RefPtr<SequencedTaskRunner> __YYAPI GetCurrent();

                virtual uint32_t __YYAPI AddRef() = 0;

                virtual uint32_t __YYAPI Release() = 0;

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
                    auto _pThreadWorkEntry = RefPtr<ThreadWorkEntryImpl<LambdaCallback>>::Create(ThreadWorkEntryStyle::None, std::move(_pfnLambdaCallback));
                    if (!_pThreadWorkEntry)
                        return E_OUTOFMEMORY;

                    return PushThreadWorkEntry(_pThreadWorkEntry);
                }

                /// <summary>
                /// ����һ���첽�� co_await ����
                /// </summary>
                /// <param name="_pfnLambdaCallback">��Ҫ�첽ִ�е� Lambda ���ʽ</param>
                /// <returns>awaitable</returns>
                template<typename LambdaCallback>
                auto __YYAPI AsyncTask(_In_ LambdaCallback&& _pfnLambdaCallback)
                {
                    struct WorkEntryType
                    {
                        // ���������ɺ����»ص��� TaskRunner
                        RefPtr<SequencedTaskRunner> pResumeTaskRunner;
                        // 0����ʾ��δ���ã�-1��ʾ������ɡ�
                        intptr_t hHandle;
                        // ��Ҫ�첽ִ�е� Callback��ע����ڽṹ��ĩβ�����ڱ������ظ�����ϲ�
                        LambdaCallback pfnLambdaCallback;

                        void operator()()
                        {
                            pfnLambdaCallback();

                            auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);

                            if (_hHandle)
                            {
                                // ��� pResumeTaskRunner == nullptr��Ŀ�겻�����κ�һ�� SequencedTaskRunner����ܿ������񲻹����Ƿ���Ҫ����
                                // ��� pResumeTaskRunner == SequencedTaskRunner::GetCurrent()����û�е������ PostTask��ͽ��������
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
                virtual HRESULT __YYAPI PushThreadWorkEntry(_In_ ThreadWorkEntry* _pWorkEntry) = 0;
            };

            // �����У�ӵ�й̶��̵߳�Task
            class DECLSPEC_NOVTABLE ThreadTaskRunner : public SequencedTaskRunner
            {
            public:

                /// <summary>
                /// ��ȡ�󶨵��߳�Id��
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;
                
                /// <summary>
                /// ��ȡ��ǰ�̰߳󶨵�TaskRunner��
                /// </summary>
                /// <returns>
                /// ����ڷ��� nullptr�����߳̿������� SequencedTaskRunner��û�й̶������̡߳�Ҳ���� RunUIMessageLoop ��δ���á�
                /// </returns>
                static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

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
        } // namespace Threading
    }     // namespace Base

    using namespace YY::Base::Threading;
} // namespace YY

#pragma pack(push, pop)
