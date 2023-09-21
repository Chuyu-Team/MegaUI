#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
#ifdef _WIN32
            using ::WaitOnAddress;
            using ::WakeByAddressSingle;
            using ::WakeByAddressAll;
#else // !_WIN32
            bool
            __YYAPI
            WaitOnAddress(
                _In_reads_bytes_(AddressSize) volatile void* Address,
                _In_reads_bytes_(AddressSize) void* CompareAddress,
                _In_ size_t AddressSize,
                _In_opt_ uint32_t dwMilliseconds
                );

            void
            __YYAPI
            WakeByAddressSingle(
                    _In_ void* Address
                );

            void
            __YYAPI
            WakeByAddressAll(
                _In_ void* Address
                );
#endif // _WIN32
        }
    } // namespace Base

    using namespace YY::Base::Sync;
} // namespace YY
