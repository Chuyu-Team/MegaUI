#pragma once
#include "MegaUITypeInt.h"
namespace YY
{
    namespace MegaUI
    {
        enum class Encoding
        {
            // 使用默认代码页的ANSI，具体取决与运行环境
            ANSI_DEFAULT = 0,

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

            // 本机wchar_t编码
            UTFW = sizeof(wchar_t) == 2 ? UTF16 : UTF32,


            UTF8 = 65001,
        };

        __inline bool __fastcall IsANSI(Encoding eEncoding)
        {
            switch (eEncoding)
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