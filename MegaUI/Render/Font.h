#pragma once

#include <MegaUI/base/MegaUITypeInt.h>
#include <MegaUI/base/StringBase.h>
#include <MegaUI/base/Color.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        struct EnumMap;

        namespace FontWeight
        {
            constexpr auto Thin = 100;
            constexpr auto ExtraLight = 200;
            constexpr auto UltraLight = 200;
            constexpr auto Light = 300;
            constexpr auto SemiLight = 350;
            constexpr auto Normal = 400;
            constexpr auto Regular = 400;
            constexpr auto Medium = 500;
            constexpr auto DemiBold = 600;
            constexpr auto SemiBold = 600;
            constexpr auto Bold = 700;
            constexpr auto ExtraBold = 800;
            constexpr auto UltraBold = 800;
            constexpr auto Black = 900;
            constexpr auto Heavy = 900;
            constexpr auto ExtraBlack = 950;
            constexpr auto UltraBlack = 950;
        } // namespace FontWeight

        namespace FontStyle
        {
            constexpr auto None = 0x00000000;
            constexpr auto Italic = 0x00000001;
            constexpr auto Underline = 0x00000002;
            constexpr auto StrikeOut = 0x00000004;
        }

        // 保存字体的基本信息
        struct Font
        {
            // 字体名称
            uString szFace;
            // 字体大小
            float iSize = 0;
            // 字体的粗细，FontWeight
            uint32_t uWeight = 0;
            // FontStyle 的位组合
            uint32_t fStyle = 0;
        };

        enum class SystemFont : uint32_t
        {
            CaptionFont,
            MenuFont,
            MessageFont,
            SmCaptionFont,
            StatusFont,
            IconFont,
            SystemFontCount,
        };

        HRESULT __YYAPI GetSystemFont(_In_ SystemFont _eSystemFont, _Out_ Font* _pFont);


        _Ret_notnull_ const EnumMap* __YYAPI GetSystemFontEnumMap();
    }
} // namespace YY

#pragma pack(pop)
