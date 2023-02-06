#pragma once
#include <Windows.h>

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Multimedia
    {
        class Point
        {
        public:
            float X;
            float Y;

            constexpr Point()
                : X(0)
                , Y(0)
            {
            }

            constexpr Point(float _X, float _Y)
                : X(_X)
                , Y(_Y)
            {
            }

            constexpr Point(const Point& _Other)
                : X(_Other.X)
                , Y(_Other.Y)
            {
            }

            inline constexpr Point(POINTS _Other)
                : X((float)_Other.x)
                , Y((float)_Other.y)
            {
            }

            inline bool operator==(Point _Other) const
            {
                return X == _Other.X && Y == _Other.Y;
            }
            
            inline bool operator!=(Point _Other) const
            {
                return X != _Other.X || Y != _Other.Y;
            }

            __YYAPI operator D2D1_POINT_2F&() const
            {
                return *(D2D1_POINT_2F*)this;
            }
        };
    } // namespace Multimedia

    using namespace YY::Multimedia;
} // namespace YY

#pragma pack(pop)
