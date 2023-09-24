#pragma once
#include <Base/Threading/TaskRunner.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            extern thread_local SequencedTaskRunner* g_pTaskRunner;

            uint32_t __YYAPI GenerateNewTaskRunnerId();


        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
