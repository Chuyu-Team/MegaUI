#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <GdiPlus.h>

#include <Base/YY.h>
#include <Multimedia/Size.h>
#include <Multimedia/Point.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Multimedia
    {
        class Rect
        {
        public:
            float Left;
            float Top;
            float Right;
            float Bottom;

            __inline constexpr Rect()
                : Left(0)
                , Top(0)
                , Right(0)
                , Bottom(0)
            {
            }

            __inline constexpr Rect(float _Left, float _Top, float _Right, float _Bottom)
                : Left(_Left)
                , Top(_Top)
                , Right(_Right)
                , Bottom(_Bottom)
            {
            }

            __inline constexpr Rect(const Rect& _Other)
                : Left(_Other.Left)
                , Top(_Other.Top)
                , Right(_Other.Right)
                , Bottom(_Other.Bottom)
            {
            }

            __inline constexpr Rect(const RECT& _Other)
                : Left((float)_Other.left)
                , Top((float)_Other.top)
                , Right((float)_Other.right)
                , Bottom((float)_Other.bottom)
            {
            }

            __inline constexpr Rect(const Point& _Point, const Size& _Size)
                : Left(_Point.X)
                , Top(_Point.Y)
                , Right(_Point.X + _Size.Width)
                , Bottom(_Point.Y + _Size.Height)
            {
            }

            __inline Rect& __YYAPI operator=(const Rect& _Other)
            {
                Left = _Other.Left;
                Top = _Other.Top;
                Right = _Other.Right;
                Bottom = _Other.Bottom;
                return *this;
            }

            __inline Rect& __YYAPI operator|=(const Rect& _Other)
            {
                // UnionRect
                if (Left > _Other.Left)
                    Left = _Other.Left;
                
                if (Top > _Other.Top)
                    Top = _Other.Top;

                if (Right < _Other.Right)
                    Right = _Other.Right;

                if (Bottom < _Other.Bottom)
                    Bottom = _Other.Bottom;

                return *this;
            }

            __inline Rect& __YYAPI operator&=(const Rect& _Other)
            {
                // IntersectRect
                if (Left >= _Other.Right || Top > _Other.Bottom || Right <= _Other.Left || Bottom <= _Other.Top)
                {
                    // 空集合
                    Left = 0;
                    Top = 0;
                    Right = 0;
                    Bottom = 0;
                }
                else
                {
                    if (Left < _Other.Left)
                        Left = _Other.Left;

                    if (Top < _Other.Top)
                        Top = _Other.Top;

                    if (Right > _Other.Right)
                        Right = _Other.Right;

                    if (Bottom > _Other.Bottom)
                        Bottom = _Other.Bottom;
                }
                return *this;
            }

            /*__inline Rect& __YYAPI operator-=(const RECT& _Other)
            {
                SubtractRect(this, this, &_Other);
                return *this;
            }*/

            __inline bool __YYAPI operator==(const Rect& _Other) const
            {
                return Left == _Other.Left && Top == _Other.Top && Right == _Other.Right && Bottom == _Other.Bottom;
            }
            
            __inline bool __YYAPI operator!=(const Rect& _Other) const
            {
                return Left != _Other.Left || Top != _Other.Top || Right != _Other.Right || Bottom != _Other.Bottom;
            }

            __inline bool __YYAPI operator==(Point _Point) const
            {
                return Left == _Point.X && Top == _Point.Y;
            }

            __inline bool __YYAPI operator==(Size _Size) const
            {
                return GetWidth() == _Size.Width && GetHeight() == _Size.Height;
            }

            __inline Rect& __YYAPI operator+=(Point _Point)
            {
                Left += _Point.X;
                Right += _Point.X;
                Top += _Point.Y;
                Bottom += _Point.Y;
                return *this;
            }

            void __YYAPI DeflateRect(_In_ const Rect& _Other)
            {
                Left += _Other.Left;
                if (Left < Right)
                {
                    Right -= _Other.Right;

                    if (Right < Left)
                        Right = Left;
                }
                else
                {
                    Left = Right;
                }

                Top += _Other.Top;
                if (Top < Bottom)
                {
                    Bottom -= _Other.Bottom;

                    if (Bottom < Top)
                        Bottom = Top;
                }
                else
                {
                    Top = Bottom;
                }
            }

            bool __YYAPI PointInRect(Point _Point)
            {
                // return ::PtInRect(this, _Point) != FALSE;
                return Left <= _Point.X && Right >= _Point.X && Top <= _Point.Y && Bottom >= _Point.Y;
            }

            const Point& __YYAPI GetPoint() const
            {
                return *(Point*)this;
            }

            void __YYAPI SetPoint(Point _Point)
            {
                Right += _Point.X - Left;
                Left = _Point.X;

                Bottom += _Point.Y - Top;
                Top = _Point.Y;
            }

            Size __YYAPI GetSize() const
            {
                return Size(GetWidth(), GetHeight());
            }

            void __YYAPI SetSize(Size _Size)
            {
                Right = Left + _Size.Width;
                Bottom = Top + _Size.Height;
            }

            float __YYAPI GetWidth() const
            {
                return Right - Left;
            }

            float __YYAPI GetHeight() const
            {
                return Bottom - Top;
            }

            bool __YYAPI IsEmpty() const
            {
                return Left >= Right || Top >= Bottom;
            }

            void Clear()
            {
                Left = 0;
                Right = 0;
                Top = 0;
                Bottom = 0;
            }

            Point __YYAPI GetCenterPoint() const
            {
                return Point(Left + GetWidth() / 2, Top + GetHeight() / 2);
            }

            __YYAPI operator D2D_RECT_F&() const
            {
                return *(D2D_RECT_F*)this;
            }

            __YYAPI operator Gdiplus::RectF() const
            {
                Gdiplus::RectF _Rect(Left, Top, GetWidth(), GetHeight());
                return _Rect;
            }
        };


        __inline Rect __YYAPI operator|(const Rect& _Left, const Rect& _Rigth)
        {
            Rect _Tmp = _Left;
            _Tmp |= _Rigth;
            return _Tmp;
        }

        __inline Rect __YYAPI operator&(const Rect& _Left, const Rect& _Rigth)
        {
            Rect _Tmp = _Left;
            _Tmp &= _Rigth;
            return _Tmp;
        }
        //
        //__inline Rect __YYAPI operator-(const Rect& _Left, const Rect& _Rigth)
        //{
        //    Rect _Tmp;
        //    SubtractRect(&_Tmp, &_Left, &_Rigth);
        //    return _Tmp;
        //}
    } // namespace Multimedia
    
    using namespace YY::Multimedia;
} // namespace YY

#pragma pack(pop)
