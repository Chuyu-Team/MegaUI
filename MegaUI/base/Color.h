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

            constexpr static Color __fastcall MakeARGB(_In_ uint8_t _Alpha, _In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue)
            {
                Color _Tmp(_Alpha, _Red, _Green, _Blue);
                return _Tmp;
            }

            constexpr static Color __fastcall MakeRGB(_In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue)
            {
                Color _Tmp(0xFFu, _Red, _Green, _Blue);
                return _Tmp;
            }

            constexpr static Color __fastcall MakeRGBA(_In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue, _In_ float _AlphaF)
            {
                Color _Tmp(uint8_t(_AlphaF * 0xFFu), _Red, _Green, _Blue);
                return _Tmp;
            }

            constexpr Color(_In_ uint8_t _Alpha, _In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue)
                : Red(_Red)
                , Green(_Green)
                , Blue(_Blue)
                , Alpha(_Alpha)
            {
            }

            constexpr Color(_In_ COLORREF _ColorRGBA = 0u)
                : ColorRGBA(_ColorRGBA)
            {
            }

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