#pragma once
#include <dwrite.h>

#include "..\base\MegaUITypeInt.h"
#include "..\base\Rect.h"
#include "..\base\Color.h"
#include <MegaUI/base/StringView.h>
#include <MegaUI/base/Font.h>

#pragma pack(push, __MEGA_UI_PACKING)

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
            Render& __MEGA_UI_API operator=(const Render&) = delete;

            virtual
            HRESULT
            __MEGA_UI_API
            BeginDraw(Rect* _pNeedPaintRect) =0;

            virtual
            HRESULT
            __MEGA_UI_API
            EndDraw() = 0;

            virtual
            void
            __MEGA_UI_API
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) = 0;

            virtual
            void
            __MEGA_UI_API
            PopAxisAlignedClip() = 0;

            virtual
            void
            __MEGA_UI_API
            FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) = 0;
                
            virtual
            HRESULT
            __MEGA_UI_API
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) = 0;

            virtual
            HRESULT
            __MEGA_UI_API
            SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) = 0;

            virtual
            D2D1_SIZE_U __MEGA_UI_API GetPixelSize() = 0;
            
            /// <summary>
            /// Create a text format object used for text layout.
            /// </summary>
            /// <param name="_szFontFamilyName">Name of the font family</param>
            /// <param name="_pFontCollection">Font collection. NULL indicates the system font collection.</param>
            /// <param name="_eFontWeight">Font weight</param>
            /// <param name="_eFontStyle">Font style</param>
            /// <param name="_eFontStretch">Font stretch</param>
            /// <param name="_uFontSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
            /// <param name="_szLocaleName">Locale name</param>
            /// <param name="_ppTextFormat">Contains newly created text format object, or NULL in case of failure.</param>
            /// <returns>
            /// Standard HRESULT error code.
            /// </returns>
            /// <remarks>
            /// If fontCollection is nullptr, the system font collection is used, grouped by typographic family name
            /// (DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE) without downloadable fonts.
            /// </remarks>
            virtual
            HRESULT
            __MEGA_UI_API CreateTextFormat(
                _In_z_ uchar_t const* _szFontFamilyName,
                _In_opt_ IDWriteFontCollection* _pFontCollection,
                _In_ const Font& _FontInfo,
                _In_z_ uchar_t const* _szLocaleName,
                _COM_Outptr_ IDWriteTextFormat** _ppTextFormat
                ) = 0;

            /// <summary>
            /// CreateTextLayout takes a string, format, and associated constraints
            /// and produces an object representing the fully analyzed
            /// and formatted result.
            /// </summary>
            /// <param name="_szText">The string to layout.</param>
            /// <param name="_pTextFormat">The format to apply to the string.</param>
            /// <param name="_uMaxWidth">Width of the layout box.</param>
            /// <param name="_uMaxHeight">Height of the layout box.</param>
            /// <param name="_ppTextLayout">The resultant object.</param>
            /// <returns>
            /// Standard HRESULT error code.
            /// </returns>
            virtual
            HRESULT
            __MEGA_UI_API CreateTextLayout(
                _In_ uStringView _szText,
                _In_ IDWriteTextFormat* _pTextFormat,
                _In_ Size _Maxbound,
                _COM_Outptr_ IDWriteTextLayout** _ppTextLayout
                ) =0;

            /// <summary>
            /// Draw a text layout object. If the layout is not subsequently changed, this can
            /// be more efficient than DrawText when drawing the same layout repeatedly.
            /// </summary>
            /// <param name="options">The specified text options. If D2D1_DRAW_TEXT_OPTIONS_CLIP
            /// is used, the text is clipped to the layout bounds. These bounds are derived from
            /// the origin and the layout bounds of the corresponding IDWriteTextLayout object.
            /// </param>
            virtual
            void
            __MEGA_UI_API DrawTextLayout(
                _In_ Point _Origin,
                _In_ IDWriteTextLayout* _pTextLayout,
                _In_ ID2D1Brush* _pDefaultFillBrush,
                _In_ D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE
                ) = 0;

            virtual
            void
            __MEGA_UI_API
            DrawString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Rect& _LayoutRect,
                _In_ int32_t _fTextAlign
                ) = 0;
        };

        HRESULT __MEGA_UI_API CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender);
        
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
        inline Color __MEGA_UI_API AdjustBrightness(Color cr, double fIllum)
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
