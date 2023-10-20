#pragma once

#ifdef _WIN32
#include <winerror.h>
#endif

#include <Base/YY.h>

namespace YY::Base
{
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
    constexpr HRESULT E_NOT_SET = 5;
       
    constexpr LSTATUS ERROR_SUCCESS = 0;
    constexpr LSTATUS ERROR_CANCELLED = 5;
    constexpr LSTATUS ERROR_BAD_FORMAT = 5;
#endif
    constexpr inline _Translates_Win32_to_HRESULT_(x) HRESULT HRESULT_From_LSTATUS(_In_ LSTATUS _lStatus) noexcept
    {
        if (_lStatus == ERROR_SUCCESS)
            return S_OK;
#ifdef _WIN32
        return __HRESULT_FROM_WIN32(_lStatus);
#else
        return ((HRESULT)(_lStatus) <= 0 ? ((HRESULT)(_lStatus)) : ((HRESULT)(((_lStatus)&0x0000FFFF) | (/*FACILITY_WIN32*/ 7 << 16) | 0x80000000)));
#endif
    }


#ifdef _WIN32
    inline LSTATUS WINAPI DosErrorFormNtStatus(long _Status) noexcept
    {
        if (_Status == 0)
            return ERROR_SUCCESS;

        static void* g_pfnRtlNtStatusToDosError = nullptr;

        if (g_pfnRtlNtStatusToDosError == nullptr)
        {
            auto _pfn = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError");
            if (_pfn)
            {
                g_pfnRtlNtStatusToDosError = _pfn;
            }
            else
            {
                g_pfnRtlNtStatusToDosError = (void*)-1;
            }
        }

        if (g_pfnRtlNtStatusToDosError == (void*)-1)
        {
            // 找不到这函数就转换到一个通用错误。
            return E_FAIL;
        }

        return ((decltype(DosErrorFormNtStatus)*)g_pfnRtlNtStatusToDosError)(_Status);
    }
#endif
} // namespace YY::Base

namespace YY
{
    using namespace YY::Base;
}
