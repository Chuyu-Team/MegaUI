#pragma once
#include <Windows.h>
#include <d2d1.h>

#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Rect : public RECT
        {
        public:
            __inline constexpr Rect()
                : RECT {}
            {
            }

            __inline constexpr Rect(const RECT& _Other)
                : RECT {_Other}
            {
            }

            __inline constexpr Rect(const POINT& _Point, const SIZE& _Size)
                : RECT {_Point.x, _Point.y, _Point.x + _Size.cx, _Point.y + _Size.cy}
            {
            }

            __inline constexpr Rect(int32_t _Left, int32_t _Top, int32_t _Right, int32_t _Bottom)
                : RECT {_Left, _Top, _Right, _Bottom}
            {
            }

            __inline Rect& __fastcall operator=(const RECT& _Other)
            {
                left = _Other.left;
                top = _Other.top;
                right = _Other.right;
                bottom = _Other.bottom;
                return *this;
            }

            __inline Rect& __fastcall operator|=(const RECT& _Other)
            {
                UnionRect(this, this, &_Other);
                return *this;
            }

            __inline Rect& __fastcall operator&=(const RECT& _Other)
            {
                IntersectRect(this, this, &_Other);
                return *this;
            }

            __inline Rect& __fastcall operator-=(const RECT& _Other)
            {
                SubtractRect(this, this, &_Other);
                return *this;
            }

            __inline bool __fastcall operator==(const RECT& _Other)
            {
                return EqualRect(this, &_Other);
            }
            
            __inline bool __fastcall operator!=(const RECT& _Other)
            {
                return EqualRect(this, &_Other) == FALSE;
            }

            __inline bool __fastcall operator==(POINT _Point)
            {
                return left == _Point.x && top == _Point.y;
            }

            __inline bool __fastcall operator==(SIZE _Size)
            {
                return right - left == _Size.cx && bottom - top == _Size.cy;
            }

            void __fastcall DeflateRect(_In_ const RECT& _Other)
            {
                left += _Other.left;
                if (left < right)
                {
                    right -= _Other.right;

                    if (right < left)
                        right = left;
                }
                else
                {
                    left = right;
                }

                top += _Other.top;
                if (top < bottom)
                {
                    bottom -= _Other.bottom;

                    if (bottom < top)
                        bottom = top;
                }
                else
                {
                    top = bottom;
                }
            }

            bool __fastcall PointInRect(POINT _Point)
            {
                return ::PtInRect(this, _Point) != FALSE;
            }

            void __fastcall SetPoint(POINT _Point)
            {
                right += _Point.x - left;
                left = _Point.x;

                bottom += _Point.y - top;
                top = _Point.y;
            }

            void __fastcall SetSize(SIZE _Size)
            {
                right = left + _Size.cx;
                bottom = top + _Size.cy;
            }

            int32_t __fastcall GetWidth()
            {
                return right - left;
            }

            int32_t __fastcall GetHeight()
            {
                return bottom - top;
            }

            __fastcall operator D2D_RECT_F() const
            {
                D2D_RECT_F _RectF;
                _RectF.left = left;
                _RectF.top = top;
                _RectF.right = right;
                _RectF.bottom = bottom;
                return _RectF;
            }
        };


        __inline Rect __fastcall operator|(const Rect& _Left, const Rect& _Rigth)
        {
            Rect _Tmp;
            UnionRect(&_Tmp, &_Left, &_Rigth);
            return _Tmp;
        }

        __inline Rect __fastcall operator&(const Rect& _Left, const Rect& _Rigth)
        {
            Rect _Tmp;
            IntersectRect(&_Tmp, &_Left, &_Rigth);
            return _Tmp;
        }
        
        __inline Rect __fastcall operator-(const Rect& _Left, const Rect& _Rigth)
        {
            Rect _Tmp;
            SubtractRect(&_Tmp, &_Left, &_Rigth);
            return _Tmp;
        }
    }
} // namespace YY

#pragma pack(pop)
