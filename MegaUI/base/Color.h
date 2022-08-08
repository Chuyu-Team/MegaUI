#pragma once
#include <Windows.h>
#include <d2d1.h>

#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        union Color
        {
            struct
            {
                uint8_t Red;
                uint8_t Green;
                uint8_t Blue;
                uint8_t Alpha;
            };

            COLORREF ColorRGBA;

            COLORREF ColorRGB : 24;

            DXGI_RGBA __fastcall GetFloatColorRGBA() const
            {
                DXGI_RGBA _Color;
                _Color.r = Red;
                _Color.g = Green;
                _Color.b = Blue;
                _Color.a = float(Alpha) / 255.0f;
                return _Color;
            }

            __fastcall operator DXGI_RGBA() const
            {
                return GetFloatColorRGBA();
            }
        };
    }
} // namespace YY