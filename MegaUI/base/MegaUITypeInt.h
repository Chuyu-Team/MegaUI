#pragma once

/*
本文定义了 MegaUI 基本的数据类型，这样做是为了更好的进行跨平台移植工作

我们优先使用 stdint.h 的内容，因为它属于 C++ 标准，更便于人们理解
*/

#include <stdint.h>

namespace YY
{
    namespace MegaUI
    {
        typedef ::int8_t int8_t;
        typedef ::int16_t int16_t;
        typedef ::int32_t int32_t;
        typedef ::int64_t int64_t;

        typedef ::uint8_t uint8_t;
        typedef ::uint16_t uint16_t;
        typedef ::uint32_t uint32_t;
        typedef ::uint64_t uint64_t;

        typedef ::intmax_t intmax_t;
        typedef ::uintmax_t uintmax_t;

        typedef ::intptr_t int_t;
        typedef ::size_t uint_t;


        constexpr auto int8_min = INT8_MIN;
        constexpr auto int8_max = INT8_MAX;

        constexpr auto int16_min = INT16_MIN;
        constexpr auto int16_max = INT16_MAX;

        constexpr auto int32_min = INT32_MIN;
        constexpr auto int32_max = INT32_MAX;

        constexpr auto int64_min = INT64_MIN;
        constexpr auto int64_max = INT64_MAX;

        constexpr auto uint8_max = UINT8_MAX;
        constexpr auto uint16_max = UINT16_MAX;
        constexpr auto uint32_max = UINT32_MAX;
        constexpr auto uint64_max = UINT64_MAX;

        constexpr auto intmax_min = INTMAX_MIN;
        constexpr auto intmax_max = INTMAX_MAX;
        constexpr auto uintmax_max = UINTMAX_MAX;

        constexpr auto int_min = INTPTR_MIN;
        constexpr auto int_max = INTPTR_MAX;
        constexpr auto uint_max = UINTPTR_MAX;

#ifdef __cpp_lib_char8_t
        // UTF8 字符
        typedef char8_t u8char;
#else
        // UTF8 字符
        typedef uint8_t u8char;
#endif

        
#ifdef _WIN32
        // UTF16字符
        typedef wchar_t u16char;
#else
        // UTF16字符
        typedef char16_t u16char;
#endif

        // UTF32字符
        typedef char32_t u32char;


#ifdef _WIN32
        // 当前平台的推荐Unicode字符
        typedef u16char uchar_t;
#else
        // 当前平台的推荐字符
        typedef u8char uchar_t;
#endif
        
        typedef _Null_terminated_ char* raw_string;
        typedef _Null_terminated_ const char* raw_const_string;

        typedef _Null_terminated_ u8char* raw_u8string;
        typedef _Null_terminated_ const u8char* raw_const_u8string;

        typedef _Null_terminated_ u16char* raw_u18string;
        typedef _Null_terminated_ const u16char* raw_const_u16string;

        typedef _Null_terminated_ u32char* raw_u32string;
        typedef _Null_terminated_ const u32char* raw_const_u32string;

        // 当前平台的推荐的Unicode字符串类型
        typedef _Null_terminated_ uchar_t* raw_ustring;
        // 当前平台的推荐的只读Unicode字符串类型
        typedef _Null_terminated_ const uchar_t* raw_const_ustring;
    }
}