#pragma once

/*
本文定义了 MegaUI 基本的数据类型，这样做是为了更好的进行跨平台移植工作

我们优先使用 stdint.h 的内容，因为它属于 C++ 标准，更便于人们理解
*/

#include <stdint.h>

#define __MEGA_UI_PACKING 4

#ifndef __MEGA_UI_API
#ifdef _WIN32
#define __MEGA_UI_API __fastcall
#else
#define __MEGA_UI_API
#endif
#endif //!__MEGA_UI_API


namespace YY
{
    namespace MegaUI
    {
        using ::int8_t;
        using ::int16_t;
        using ::int32_t;
        using ::int64_t;

        using ::uint8_t;
        using ::uint16_t;
        using ::uint32_t;
        using ::uint64_t;

        using ::intmax_t;
        using ::uintmax_t;

        using ::intptr_t;
        typedef size_t uintptr_t;

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

        // ANSI字符
        typedef char achar_t;


#if defined(__cpp_lib_char8_t) || defined(__cpp_char8_t)
        // UTF8 字符
        typedef char8_t u8char_t;
#else
        // UTF8 字符
        typedef uint8_t u8char_t;
#endif

#ifdef _WIN32
        // UTF16字符，Windows平台 wchar_t，就是UTF16
        typedef wchar_t u16char_t;

        // UTF32字符
        typedef char32_t u32char_t;

        // 当前平台的推荐Unicode字符
        typedef u16char_t uchar_t;
#else
        // UTF16字符
        typedef char16_t u16char_t;

        // UTF32字符
        typedef wchar_t u32char_t;

        // 当前平台的推荐字符
        typedef u8char_t uchar_t;

        static_assert(sizeof(wchar_t) == 4, "wchar_t UTF32")
#endif
        
        typedef _Null_terminated_ achar_t* raw_astring_t;
        typedef _Null_terminated_ const achar_t* raw_const_astring_t;

        typedef _Null_terminated_ u8char_t* raw_u8string;
        typedef _Null_terminated_ const u8char_t* raw_const_u8string_t;

        typedef _Null_terminated_ u16char_t* raw_u18string_t;
        typedef _Null_terminated_ const u16char_t* raw_const_u16string_t;

        typedef _Null_terminated_ u32char_t* raw_u32string_t;
        typedef _Null_terminated_ const u32char_t* raw_const_u32string_t;

        // 当前平台的推荐的Unicode字符串类型
        typedef _Null_terminated_ uchar_t* raw_ustring_t;
        // 当前平台的推荐的只读Unicode字符串类型
        typedef _Null_terminated_ const uchar_t* raw_const_ustring_t;

#define _U8S(S) u8 ## S

#ifdef _WIN32
#define _U16S(S) L ## S
#else
#define _U16S(S) u ## S
#endif


#define _U32S(S) U ## S

#ifdef _WIN32
#define _S(S) _U16S(S)
#else
#define _S(S) _U8S(S)
#endif

        // 字节
        typedef unsigned char byte_t;


        typedef _Return_type_success_(return >= 0) long HRESULT;
    } // namespace MegaUI
} // namespace YY