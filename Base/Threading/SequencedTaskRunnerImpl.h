#pragma once
#include <Base/Threading/TaskRunnerImpl.h>
#include <Base/Sync/InterlockedQueue.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            class SequencedTaskRunnerImpl : public SequencedTaskRunner
            {
                enum class RunnerFlagIndex
                {
                    QueuePushLock = 0,
                    // 正在等待消息循环,
                    WaitForMessage,
                    // 阻止WaitForMessage
                    CanWaitForMessage,
                };
            private:
                uint32_t uRef;
                uint32_t uTaskRunnerId;
                uint32_t uThreadId;
                // uWeakCount    uPushLock
                // [31  ~  1]       0
                union
                {
                    struct
                    {
                        uint32_t uPushLock : 1;
                        uint32_t uWeakCount : 31;
                    };
                    uint32_t uWeakCountAndPushLock;
                };
                InterlockedQueue<ThreadWorkEntry> oThreadWorkList;

            public:
                SequencedTaskRunnerImpl();

                SequencedTaskRunnerImpl(const SequencedTaskRunnerImpl&) = delete;

                SequencedTaskRunnerImpl& operator=(const SequencedTaskRunnerImpl&) = delete;

                /////////////////////////////////////////////////////
                // TaskRunner

                virtual uint32_t __YYAPI AddRef() override;

                virtual uint32_t __YYAPI Release() override;

                virtual uint32_t __YYAPI GetId() override;

                virtual TaskRunnerStyle __YYAPI GetStyle() override;

                virtual HRESULT __YYAPI Async(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) override;

                virtual HRESULT __YYAPI Async(_In_ std::function<void()>&& _pfnLambdaCallback) override;

                virtual HRESULT __YYAPI Sync(_In_ TaskRunnerSimpleCallback _pfnCallback, _In_opt_ void* _pUserData) override;

            private:
                void __YYAPI PushThreadWorkEntry(ThreadWorkEntry* _pWorkEntry);

                void __YYAPI DoWorkList();
            };
        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
