#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <d2d1.h>
#include <GdiPlus.h>
#endif

#include <YY/Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
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

            uint32_t ColorRGBA;

            uint32_t ColorRGB : 24;

            constexpr static Color __YYAPI MakeARGB(_In_ uint8_t _Alpha, _In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue)
            {
                Color _Tmp(_Alpha, _Red, _Green, _Blue);
                return _Tmp;
            }

            constexpr static Color __YYAPI MakeRGB(_In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue)
            {
                Color _Tmp(0xFFu, _Red, _Green, _Blue);
                return _Tmp;
            }

            constexpr static Color __YYAPI MakeRGBA(_In_ uint8_t _Red, _In_ uint8_t _Green, _In_ uint8_t _Blue, _In_ float _AlphaF)
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

            constexpr Color(_In_ uint32_t _ColorRGBA = 0u)
                : ColorRGBA(_ColorRGBA)
            {
            }

#ifdef _WIN32
            __YYAPI operator DXGI_RGBA() const
            {
                DXGI_RGBA _Color;
                _Color.r = float(Red) / 255.0f;
                _Color.g = float(Green) / 255.0f;
                _Color.b = float(Blue) / 255.0f;
                _Color.a = float(Alpha) / 255.0f;
                return _Color;
            }
#endif

#ifdef _WIN32
            __YYAPI operator Gdiplus::Color() const
            {
                return Gdiplus::Color(Alpha, Red, Green, Blue);
            }
#endif
            bool __YYAPI operator==(Color _Other) const
            {
                return ColorRGBA == _Other.ColorRGBA;
            }
        };
    } // namespace Media

    using namespace YY::Media;
} // namespace YY

#pragma pack(pop)
