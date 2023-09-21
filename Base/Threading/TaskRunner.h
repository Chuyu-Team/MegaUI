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
                // ���У�û�д˱�־������С�
                Sequenced = 0x00000001,
                // ӵ�й̶��̣߳�û�д˱�׼��ʾʵ��ָ���߳̿�����ʱ�仯��
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
                virtual HRESULT __YYAPI Async(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) = 0;

                /// <summary>
                /// �������첽ִ�С�
                /// </summary>
                /// <param name="pfnLambdaCallback">��Ҫ�첽ִ�е�Lambda���ʽ��</param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Async(_In_ std::function<void()>&& _pfnLambdaCallback) = 0;

                /// <summary>
                /// ������ͬ��ִ�С�
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

            // ��˳��ִ�е�Task����һ���󶨹̶������̣߳�ֻ��֤�����У�
            class DECLSPEC_NOVTABLE SequencedTaskRunner : public TaskRunner
            {
            public:
                // ���̳߳ش���һ�� TaskRunner��ע������ִ��ʱֻ��֤����֤ʼ����ͬһ���̣߳�����֤�����С�
                static RefPtr<SequencedTaskRunner> __YYAPI Create();
            };

            // ӵ�й̶��̵߳�Task
            class DECLSPEC_NOVTABLE ThreadTaskRunner : public SequencedTaskRunner
            {
            public:
                static RefPtr<ThreadTaskRunner> __YYAPI GetCurrent();

                /// <summary>
                /// ��ȡ�󶨵��߳�Id��
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;

                virtual uintptr_t __YYAPI RunMessageLoop() = 0;
            };

            // ����ִ�е�Task������֤����˳��ִ��
            class DECLSPEC_NOVTABLE ParallelTaskRunner : public TaskRunner
            {
            public:
            };

        } // namespace Threading
    }     // namespace Base

    using namespace YY::Base::Threading;
} // namespace YY

#pragma pack(push, pop)
