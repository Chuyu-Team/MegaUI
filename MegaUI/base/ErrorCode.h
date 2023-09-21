#pragma once

#ifdef _WIN32
#include <winerror.h>
#endif

namespace YY
{
    namespace Base
    {
        constexpr inline _Translates_Win32_to_HRESULT_(x) HRESULT HRESULT_From_LSTATUS(_In_ LSTATUS _lStatus)
        {
#ifdef _WIN32
            return __HRESULT_FROM_WIN32(_lStatus);
#else
            return ((HRESULT)(_lStatus) <= 0 ? ((HRESULT)(_lStatus)) : ((HRESULT)(((_lStatus)&0x0000FFFF) | (/*FACILITY_WIN32*/ 7 << 16) | 0x80000000)));
#endif
        }

#ifndef _WIN32
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

        constexpr HRESULT S_OK = 0;
        constexpr HRESULT S_FALSE = 0;

        constexpr HRESULT E_INVALIDARG = 1;
        constexpr HRESULT E_OUTOFMEMORY = 2;
        constexpr HRESULT E_POINTER = 3;
        constexpr HRESULT E_UNEXPECTED = 4;
        constexpr HRESULT E_BOUNDS = 5;
        constexpr HRESULT E_NOINTERFACE = 5;
        constexpr HRESULT E_NOTIMPL = 5;
        constexpr HRESULT E_FAIL = 5;
        constexpr HRESULT E_PENDING = 5;
        constexpr HRESULT E_ABORT = 5;

        constexpr LSTATUS ERROR_CANCELLED = 5;
#endif
    } // namespace Base
} // namespace YY
