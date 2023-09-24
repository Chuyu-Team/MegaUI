#include "pch.h"
#include <MegaUI/Render/FontEnumMap.h>

#include <Media/Font.h>
#include <MegaUI/Core/Property.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {        
        const EnumMap* __YYAPI GetSystemFontEnumMap()
        {
            static const EnumMap SystemFontEnumMap[] =
            {
                { u8"CaptionFont", (int32_t)SystemFont::CaptionFont },
                { u8"MenuFont", (int32_t)SystemFont::MenuFont },
                { u8"MessageFont", (int32_t)SystemFont::MessageFont },
                { u8"SmCaptionFont", (int32_t)SystemFont::SmCaptionFont },
                { u8"StatusFont", (int32_t)SystemFont::StatusFont },
                { u8"IconFont", (int32_t)SystemFont::IconFont },
                { }
            };
            return SystemFontEnumMap;
        }
    } // namespace MegaUI
} // namespace YY
