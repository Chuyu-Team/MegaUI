#include "pch.h"
#include <MegaUI/Render/FontEnumMap.h>

#include <Media/Font.h>
#include <MegaUI/core/Property.h>

// 批注不一致警告
#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {        
        const EnumMap* __YYAPI GetSystemFontEnumMap()
        {
            static const EnumMap SystemFontEnumMap[] =
            {
                { "CaptionFont", (int32_t)SystemFont::CaptionFont },
                { "MenuFont", (int32_t)SystemFont::MenuFont },
                { "MessageFont", (int32_t)SystemFont::MessageFont },
                { "SmCaptionFont", (int32_t)SystemFont::SmCaptionFont },
                { "StatusFont", (int32_t)SystemFont::StatusFont },
                { "IconFont", (int32_t)SystemFont::IconFont },
                { }
            };
            return SystemFontEnumMap;
        }
    } // namespace MegaUI
} // namespace YY
