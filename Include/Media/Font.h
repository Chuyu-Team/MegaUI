#pragma once
#include <YY/Base/Strings/String.h>

namespace YY
{
    namespace Media
    {
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

        enum class FontStyle
        {
            None = 0x00000000,
            Italic = 0x00000001,
            Underline = 0x00000002,
            StrikeOut = 0x00000004,
        };

        YY_APPLY_ENUM_CALSS_BIT_OPERATOR(FontStyle);

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
            FontStyle fStyle = FontStyle::None;
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
    } // namespace Media

    using namespace YY::Media;
} // namespace YY
