#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <d2d1.h>
#include <GdiPlus.h>
#endif

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        class Size
        {
        public:
            float Width;
            float Height;

            constexpr Size()
                : Width(0.0f)
                , Height(0.0f)
            {
            }
            
            constexpr Size(float _Width, float _Height)
                : Width(_Width)
                , Height(_Height)
            {
            }

#ifdef _WIN32
            Size(const SIZE& _Size)
                : Width((float)_Size.cx)
                , Height((float)_Size.cy)
            {
            }
#endif

            inline bool __YYAPI IsEmpty() const
            {
                return Width == 0.0f || Height == 0.0f;
            }

            inline bool __YYAPI operator==(Size _Other) const
            {
                return Width == _Other.Width && Height == _Other.Height;
            }
            
            inline bool __YYAPI operator!=(Size _Other) const
            {
                return Width != _Other.Width || Height != _Other.Height;
            }

#ifdef _WIN32
            __YYAPI operator Gdiplus::SizeF&() const
            {
                return *(Gdiplus::SizeF*)this;
            }
#endif
        };
    } // namespace Media

    using namespace YY::Media;
} // namespace YY

#pragma pack(pop)
