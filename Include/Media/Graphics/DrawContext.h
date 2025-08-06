#pragma once
#include <dwrite.h>

#include <Media/Point.h>
#include <Media/Brushes/Brush.h>
#include <Media/Size.h>
#include <Media/Rect.h>
#include <YY/Base/Strings/String.h>
#include <Media/Font.h>
#include <Media/Pens/Pen.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            enum class ContentAlignStyle
            {
                // 横向对齐方式
                Left = 0x00000000,
                Center = 0x00000001,
                Right = 0x00000002,

                // 纵向对齐方式
                Top = 0x00000000,
                Middle = 0x00000004,
                Bottom = 0x00000008,

                // 允许换行，一般文字排版使用
                Wrap = 0x00000010,
                // 将显示不下的字符统一显示为 "..."
                EndEllipsis = 0x00000020,
            };

            YY_APPLY_ENUM_CALSS_BIT_OPERATOR(ContentAlignStyle);

            class DrawContext
            {
            public:
                DrawContext()
                {
                }

                virtual ~DrawContext()
                {
                }

                DrawContext(const DrawContext&) = delete;

                DrawContext& operator=(const DrawContext&) = delete;
                
                virtual Size __YYAPI GetPixelSize() = 0;

                /// <summary>
                /// 设置像素大小，注意：此函数必须在 BeginDraw之前调用。
                /// </summary>
                /// <param name="_PixelSize"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI SetPixelSize(
                    _In_ const Size& _PixelSize) = 0;

                /// <summary>
                /// 开始准备绘制
                /// </summary>
                /// <param name="_pPaint">需要重绘的区域。合理设置这可以提升绘制性能。</param>
                /// <returns></returns>
                virtual HRESULT __YYAPI BeginDraw(_In_opt_ const Rect* _pPaint) = 0;

                /// <summary>
                /// 结束绘制，提交到绘制表面。
                /// </summary>
                virtual HRESULT __YYAPI EndDraw() = 0;

                virtual void __YYAPI PushAxisAlignedClip(
                    _In_ const Rect& _ClipRect) = 0;

                virtual void __YYAPI PopAxisAlignedClip() = 0;

                /// <summary>
                ///     DrawLine -
                ///     Draws a line with the specified pen.
                ///     Note that this API does not accept a Brush, as there is no area to fill.
                /// </summary>
                /// <param name="_oPoint0"> The start Point for the line. </param>
                /// <param name="_oPoint1"> The end Point for the line. </param>
                /// <param name="_oPen"> The Pen with which to stroke the line. </param>
                virtual void __YYAPI DrawLine(
                    _In_ Point _oPoint0,
                    _In_ Point _oPoint1,
                    _In_ Pen _oPen
                    ) = 0;

                /// <summary>
                /// 绘制矩形，注意 _oPen与_oBrush不能同时为空。
                /// </summary>
                /// <param name="_oRect"></param>
                /// <param name="_oPen">绘制边框的画笔。可选，为空时不绘制边框。</param>
                /// <param name="_oBrush">填充矩形内部的画刷。可选，为空时不填充矩形内部。</param>
                virtual void __YYAPI DrawRectangle(
                    _In_ const Rect& _oRect,
                    _In_opt_ Pen _oPen,
                    _In_opt_ Brush _oBrush
                    ) = 0;


                virtual
                void
                __YYAPI
                DrawString(
                    _In_ uString _szText,
                    _In_ const Font& _FontInfo,
                    _In_ Brush _oBrush,
                    _In_ const Rect& _LayoutRect,
                    _In_ ContentAlignStyle _fTextAlign) = 0;

                virtual
                void
                __YYAPI
                MeasureString(
                    _In_ uStringView _szText,
                    _In_ const Font& _FontInfo,
                    _In_ const Size& _LayoutSize,
                    _In_ ContentAlignStyle _fTextAlign,
                    _Out_ Size* _pExtent) = 0;

                virtual RefPtr<IDWriteTextLayout> __YYAPI CreateTextLayout(
                    _In_ uString _szText,
                    _In_ const Font& _FontInfo,
                    _In_ const Size& _LayoutSize,
                    _In_ ContentAlignStyle _fTextAlign
                    )
                {
                    return nullptr;
                };

                virtual
                void
                __YYAPI
                DrawString2(
                    _In_ const Point& _Origin,
                    _In_ RefPtr<IDWriteTextLayout> _pTextLayout,
                    _In_ Brush _oBrush)
                {
                }
            };

            class DrawContextFactory
            {
            public:
                static _Ret_notnull_ DrawContextFactory* __YYAPI GetDefaultDrawContextFactory();
                
#ifdef _WIN32
                static _Ret_notnull_ DrawContextFactory* __YYAPI GetGdiPlusDrawContextFactory();

                static _Ret_notnull_ DrawContextFactory* __YYAPI GetD2D1_0DrawContextFactory();

                static _Ret_notnull_ DrawContextFactory* __YYAPI GetD2D1_1DrawContextFactory();
#endif

#ifdef _WIN32
                /// <summary>
                /// 判断微软组合引擎是否可用。
                /// </summary>
                /// <returns></returns>
                virtual bool __YYAPI IsMicrosoftCompositionEngineSupport() = 0;

                virtual HRESULT __YYAPI CreateDrawTarget(_In_ HWND _hWnd, _Outptr_ DrawContext** _ppDrawContext) = 0;
#endif
            };


            template<typename DrawContextType>
            class DrawContextFactoryImpl : public DrawContextFactory
            {
            public:
#ifdef _WIN32
                bool __YYAPI IsMicrosoftCompositionEngineSupport() override
                {
                    return DrawContextType::IsMicrosoftCompositionEngineSupport();
                }

                HRESULT __YYAPI CreateDrawTarget(_In_ HWND _hWnd, _Outptr_ DrawContext** _ppDrawContext) override
                {
                    return DrawContextType::CreateDrawTarget(_hWnd, _ppDrawContext);
                }
#endif
            };

        } // namespace Graphics
    }     // namespace Media

    using namespace YY::Media::Graphics;
} // namespace YY

#pragma pack(pop)
