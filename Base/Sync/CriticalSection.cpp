#include "CriticalSection.h"

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
            void __YYAPI CriticalSection::Lock() noexcept
            {
                const auto _uCurrentThreadId = Threading::GetCurrentThreadId();
                if (uOwnerThreadId == _uCurrentThreadId)
                {
                    ++uLockRef;
                    return;
                }

                oSRWLock.Lock();
                uOwnerThreadId = _uCurrentThreadId;
                uLockRef = 0;
            }

            bool __YYAPI CriticalSection::TryLock() noexcept
            {
                const auto _uCurrentThreadId = Threading::GetCurrentThreadId();
                if (uOwnerThreadId == _uCurrentThreadId)
                {
                    ++uLockRef;
                    return true;
                }

                if (oSRWLock.TryLock())
                {
                    uOwnerThreadId = _uCurrentThreadId;
                    uLockRef = 0;
                    return true;
                }

                return false;
            }

            void __YYAPI CriticalSection::Unlock() noexcept
            {
                --uLockRef;
                if (uLockRef == 0)
                {
                    uOwnerThreadId = 0;
                    oSRWLock.Unlock();
                }
            }
        }
    }
}
