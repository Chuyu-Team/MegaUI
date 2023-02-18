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
                // ���У�û�д˱�־������С�
                Sequenced = 0x00000001,
                // ӵ�й̶��̣߳�û�д˱�׼��ʾʵ��ָ���߳̿�����ʱ�仯��
                FixedThread = 0x00000002,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(TaskRunnerStyle);

            class DECLSPEC_NOVTABLE ITaskRunner
            {
            public:
                virtual uint32_t __YYAPI AddRef() = 0;

                virtual uint32_t __YYAPI Release() = 0;

                /// <summary>
                /// ���� ITaskRunner ��Ψһ Id��ע�⣬�ⲻ���߳�Id��
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetId() = 0;

                virtual TaskRunnerStyle __YYAPI GetStyle() = 0;

                /// <summary>
                /// �������첽ִ�С�
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Async(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) = 0;

                /// <summary>
                /// ������ͬ��ִ�С�
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Sync(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) = 0;
            };

            // ��˳��ִ�е�Task����һ���󶨹̶������̣߳�ֻ��֤�����У�
            class DECLSPEC_NOVTABLE ISequencedTaskRunner : public ITaskRunner
            {
            public:
            };
            
            // ӵ�й̶��̵߳�Task
            class DECLSPEC_NOVTABLE IThreadTaskRunner : public ISequencedTaskRunner
            {
            public:
                /// <summary>
                /// ��ȡ�󶨵��߳�Id��
                /// </summary>
                /// <returns></returns>
                virtual uint32_t __YYAPI GetThreadId() = 0;
            };


            // ����ִ�е�Task������֤����˳��ִ��
            class DECLSPEC_NOVTABLE IParallelTaskRunner : public ITaskRunner
            {
            public:

            };

        } // namespace Async
    } // namespace Base
} // namespace YY
