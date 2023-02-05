#pragma once
#include <dwrite.h>

#include <MegaUI/base/MegaUITypeInt.h>
#include <MegaUI/base/Rect.h>
#include <Multimedia/Graphics/Color.h>
#include <Base/Containers/StringView.h>
#include <MegaUI/Render/Font.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        namespace ContentAlign
        {
            // 横向对齐方式
            constexpr auto Left = 0x00000000;
            constexpr auto Center = 0x00000001;
            constexpr auto Right = 0x00000002;

            // 纵向对齐方式
            constexpr auto Top = 0x00000000;
            constexpr auto Middle = 0x00000004;
            constexpr auto Bottom = 0x00000008;

            // 允许换行，一般文字排版使用
            constexpr auto Wrap = 0x00000010;
            // 将显示不下的字符统一显示为 "..."
            constexpr auto EndEllipsis = 0x00000020;
        }

        // 用于保存 Element 的位置以及是否缓存绘制信息
        struct ElementRenderNode
        {
            enum
            {
                // 控件位置发送变化
                InvalidatePosition = 0x00000001u,
                // 控件大小发生变化
                InvalidateExtent = 0x00000002u,
                // Node 中的显示内容发生变化。
                InvalidateContent = 0x00000004u,
            };
            // 相对于窗口的坐标系
            Rect Bounds;

            // Invalidate 位组合
            uint32_t uInvalidateMarks;
        };

        class Render
        {
        public:
            Render() = default;
            virtual ~Render() = default;

            Render(const Render&) = delete;
            Render& __YYAPI operator=(const Render&) = delete;

            virtual
            HRESULT
            __YYAPI
            BeginDraw(Rect* _pNeedPaintRect) =0;

            virtual
            HRESULT
            __YYAPI
            EndDraw() = 0;

            virtual
            void
            __YYAPI
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) = 0;

            virtual
            void
            __YYAPI
            PopAxisAlignedClip() = 0;

            virtual
            void
            __YYAPI
            FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) = 0;
                
            virtual
            HRESULT
            __YYAPI
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) = 0;

            virtual
            HRESULT
            __YYAPI
            SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) = 0;

            virtual
            D2D1_SIZE_U __YYAPI GetPixelSize() = 0;
            
            virtual
            void
            __YYAPI
            DrawString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ Color _crTextColor,
                _In_ const Rect& _LayoutRect,
                _In_ int32_t _fTextAlign
                ) = 0;

            virtual
            void
            __YYAPI
            MeasureString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Size& _LayoutSize,
                _In_ int32_t _fTextAlign,
                _Out_ Size* _pExtent) = 0;
        };

        HRESULT __YYAPI CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender);
        
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
        inline Color __YYAPI AdjustBrightness(Color cr, double fIllum)
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
        
        float __YYAPI DevicePixelToPixel(float _iRelativePixel, int32_t _DPI);

        float __YYAPI PointToPixel(float _iFontPoint, int32_t _DPI);

        Rect __YYAPI DevicePixelToPixel(Rect _iRelativePixelRect, int32_t _DPI);

        Size __YYAPI DevicePixelToPixel(Size _iRelativePixelSize, int32_t _DPI);

        float __YYAPI UpdatePixel(float _iOldPixel, int32_t _OldDPI, int32_t _NewDPI);

    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
