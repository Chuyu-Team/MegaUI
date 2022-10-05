#pragma once
#include <Windows.h>
#include <d2d1.h>

#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Size
        {
        public:
            float Width;
            float Height;

            constexpr Size()
                : Width(0)
                , Height(0)
            {
            }
            
            constexpr Size(float _Width, float _Height)
                : Width(_Width)
                , Height(_Height)
            {
            }

            Size(const SIZE& _Size)
                : Width((float)_Size.cx)
                , Height((float)_Size.cy)
            {
            }

            inline bool __MEGA_UI_API operator == (Size _Other)
            {
                return Width == _Other.Width && Height == _Other.Height;
            }
            
            inline bool __MEGA_UI_API operator != (Size _Other)
            {
                return Width != _Other.Width || Height != _Other.Height;
            }

            __MEGA_UI_API operator Gdiplus::SizeF&() const
            {
                return *(Gdiplus::SizeF*)this;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
