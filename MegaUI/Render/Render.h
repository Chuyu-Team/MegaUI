#pragma once
#include "..\base\MegaUITypeInt.h"
#include "..\base\Rect.h"
#include "..\base\Color.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Render
        {
        public:
            Render() = default;
            virtual ~Render() = default;

            Render(const Render&) = delete;
            Render& __fastcall operator=(const Render&) = delete;

            virtual
            HRESULT
            __fastcall
            BeginDraw(Rect* _pNeedPaintRect) =0;

            virtual
            HRESULT
            __fastcall
            EndDraw() = 0;

            virtual
            void
            __fastcall
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) = 0;

            virtual
            void
            __fastcall
            PopAxisAlignedClip() = 0;

            virtual
            void
            __fastcall
            FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) = 0;
                
            virtual
            HRESULT
            __fastcall
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) = 0;

            virtual
            HRESULT
            __fastcall
            SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) = 0;

            virtual
            D2D1_SIZE_U __fastcall GetPixelSize() = 0;
        };

        HRESULT __fastcall CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender);
        
#define LIGHT 0.5
#define VERYLIGHT 0.8
#define DARK -0.3
#define VERYDARK -0.75

        /// <summary>
        /// 调整亮度
        /// </summary>
        /// <param name="cr"></param>
        /// <param name="fIllum">1 >= fIllum >= -1</param>
        /// <returns></returns>
        inline Color __fastcall AdjustBrightness(Color cr, double fIllum)
        {
            double r = cr.Red, g = cr.Green, b = cr.Blue;

            if (fIllum > 0.0)
            {
                r += (255.0 - r) * fIllum;
                g += (255.0 - g) * fIllum;
                b += (255.0 - b) * fIllum;
            }
            else
            {
                r += r * fIllum;
                g += g * fIllum;
                b += b * fIllum;
            }

            return Color(cr.Alpha, (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }

    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
