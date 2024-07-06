#pragma once

#include <Base/YY.h>
#include <Base/Sync/SRWLock.h>
#include <Base/Threading/ProcessThreads.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Sync
{
    class CriticalSection
    {
    private:
        SRWLock oSRWLock;
        ThreadId uOwnerThreadId = 0;
        uint32_t uLockRef = 0;

    public:
        constexpr CriticalSection() noexcept = default;

        CriticalSection(const CriticalSection&) = delete;
        CriticalSection& operator=(const CriticalSection&) = delete;

        _Acquires_exclusive_lock_(*this)
        void __YYAPI Lock() noexcept;

        _When_(return != 0, _Acquires_exclusive_lock_(*this))
        bool __YYAPI TryLock() noexcept;

        _Releases_exclusive_lock_(*this)
        void __YYAPI Unlock() noexcept;
    };
}

#pragma pack(pop)
