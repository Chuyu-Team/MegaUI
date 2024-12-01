#pragma once
#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        enum class Encoding
        {
            // 使用默认代码页的ANSI，具体取决与运行环境
            ANSI = 0,

            GB2312 = 936,
            // 小端编码方式的 UTF16
            UTF16LE = 1200,
            // 大端编码方式的 UTF16
            UTF16BE = 1201,
            // 默认UTF16值，自动根据平台选择大小端
            UTF16 = UTF16LE,

            // 小端编码方式的 UTF32
            UTF32LE = 12000,
            // 大端编码方式的 UTF32
            UTF32BE = 12001,
            // 默认UTF32值，自动根据平台选择大小端
            UTF32 = UTF32LE,

            UTF8 = 65001,
            // 本机wchar_t编码
            UTFW = sizeof(wchar_t) == 2 ? UTF16 : UTF32,
            // 最佳Unciode编码
            UTFN = sizeof(uchar_t) == 1 ? UTF8 : UTFW,

        };

        template<class T>
        struct DetaultEncoding;

        template<>
        struct DetaultEncoding<achar_t>
        {
            static constexpr Encoding eEncoding = Encoding::ANSI;
        };

#if defined(__cpp_lib_char8_t) || defined(__cpp_char8_t)
        template<>
        struct DetaultEncoding<char8_t>
        {
            static constexpr Encoding eEncoding = Encoding::UTF8;
        };
#endif
        
        template<>
        struct DetaultEncoding<u16char_t>
        {
            static constexpr Encoding eEncoding = Encoding::UTF16;
        };

        template<>
        struct DetaultEncoding<u32char_t>
        {
            static constexpr Encoding eEncoding = Encoding::UTF32;
        };

        inline bool __YYAPI IsANSI(Encoding _eEncoding)
        {
            switch (_eEncoding)
            {
            case Encoding::UTF8:
            case Encoding::UTF16LE:
            case Encoding::UTF16BE:
            case Encoding::UTF32LE:
            case Encoding::UTF32BE:
                return false;
            default:
                return true;
            }
        }
    }
}
