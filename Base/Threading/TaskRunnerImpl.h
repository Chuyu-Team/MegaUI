#pragma once
#include <Base/Threading/TaskRunner.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            uint32_t __YYAPI GenerateNewTaskRunnerId();

            enum class ThreadWorkEntryStyle
            {
                None = 0,
                // ����ͬ�����С�
                Sync = 0x00000001,
                // ��C���� pfnCallback + pUserData ģʽ
                SimpleCallback = 0x00000002,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(ThreadWorkEntryStyle);

            struct ThreadWorkEntry
            {
                ThreadWorkEntryStyle fStyle;

                // ���������������ܱ�ȡ����
                HRESULT hr;

                union
                {
                    struct
                    {
                        TaskRunnerSimpleCallback pfnCallback;
                        void* pUserData;
                    } SimpleCallback;

                    std::function<void()> pfnLambdaCallback;
                };

                ThreadWorkEntry(TaskRunnerSimpleCallback _pfnCallback, void* _pUserData, bool _bSync = false);

                ThreadWorkEntry(std::function<void()>&& _pfnLambdaCallback, bool _bSync = false);

                ThreadWorkEntry(const ThreadWorkEntry&) = delete;

                ~ThreadWorkEntry();

                void __YYAPI DoWorkThenFree();
            };

        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
