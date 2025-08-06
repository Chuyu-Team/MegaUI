#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <D2d1.h>
#endif

#include <YY/Base/YY.h>


#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
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

#ifdef _WIN32
            inline constexpr Point(POINTS _Other)
                : X((float)_Other.x)
                , Y((float)_Other.y)
            {
            }
#endif

            inline bool operator==(Point _Other) const
            {
                return X == _Other.X && Y == _Other.Y;
            }
            
            inline bool operator!=(Point _Other) const
            {
                return X != _Other.X || Y != _Other.Y;
            }

            inline Point& operator+=(const Point& _Other)
            {
                X += _Other.X;
                Y += _Other.Y;
                return *this;
            }

            inline Point& operator-=(const Point& _Other)
            {
                X -= _Other.X;
                Y -= _Other.Y;
                return *this;
            }

#ifdef _WIN32
            __YYAPI operator D2D1_POINT_2F&() const
            {
                return *(D2D1_POINT_2F*)this;
            }
#endif
        };
    } // namespace Media

    using namespace YY::Media;
} // namespace YY

#pragma pack(pop)
