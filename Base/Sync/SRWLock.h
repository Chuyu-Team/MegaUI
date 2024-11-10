#pragma once

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
        #ifdef _WIN32
            class SRWLock
            {
            private:
                SRWLOCK oSRWLock = RTL_SRWLOCK_INIT;

            public:
                constexpr SRWLock() noexcept = default;

                SRWLock(const SRWLock&) = delete;
                SRWLock& operator=(const SRWLock&) = delete;

                _Acquires_exclusive_lock_(this->oSRWLock)
                void __YYAPI Lock() noexcept
                {
                    AcquireSRWLockExclusive(&oSRWLock);
                }

                _When_(return != 0, _Acquires_exclusive_lock_(this->oSRWLock))
                bool __YYAPI TryLock() noexcept
                {
                    return TryAcquireSRWLockExclusive(&oSRWLock);
                }

                _Releases_exclusive_lock_(this->oSRWLock)
                void __YYAPI Unlock() noexcept
                {
                    ReleaseSRWLockExclusive(&oSRWLock);
                }

                _When_(return != 0, _Acquires_shared_lock_(this->oSRWLock))
                bool __YYAPI TryLockShared() noexcept
                {
                    return TryAcquireSRWLockShared(&oSRWLock);
                }

                _Acquires_shared_lock_(this->oSRWLock)
                void __YYAPI LockShared() noexcept
                {
                    AcquireSRWLockShared(&oSRWLock);
                }
        
                _Releases_shared_lock_(this->oSRWLock)
                void __YYAPI UnlockShared() noexcept
                {
                    ReleaseSRWLockShared(&oSRWLock);
                }
            };

        #else
            class SRWLock
            {
            private:
                uintptr_t uInternalValue = 0;

            public:
                constexpr SRWLock() noexcept = default;

                SRWLock(const SRWLock&) = delete;
                SRWLock& operator=(const SRWLock&) = delete;
        
                _Acquires_exclusive_lock_(this->oSRWLock)
                void __YYAPI Lock() noexcept;

                _When_(return != 0, _Acquires_exclusive_lock_(this->oSRWLock))
                bool __YYAPI TryLock() noexcept;

                _Releases_exclusive_lock_(this->oSRWLock)
                void __YYAPI Unlock() noexcept;

                _When_(return != 0, _Acquires_shared_lock_(this->oSRWLock))
                bool __YYAPI TryLockShared() noexcept;

                _Acquires_shared_lock_(this->oSRWLock)
                void __YYAPI LockShared() noexcept;
        
                _Releases_shared_lock_(this->oSRWLock)
                void __YYAPI UnlockShared() noexcept;
            };
        #endif
        }
    }
}

#pragma pack(pop)
