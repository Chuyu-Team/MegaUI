#pragma once
#include <dwrite.h>

#include <Base/YY.h>
#include <Media/Graphics/DrawContext.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            namespace DWrite
            {
                _Ret_maybenull_ IDWriteFactory* __YYAPI GetDWriteFactory();

                HRESULT
                __YYAPI CreateTextFormat(
                    _In_ const Font& _FontInfo,
                    _In_ ContentAlignStyle _fTextAlign,
                    _In_z_ uchar_t const* _szLocaleName,
                    _COM_Outptr_ IDWriteTextFormat** _ppTextFormat);

                HRESULT
                __YYAPI CreateTextLayout(
                    _In_ uStringView _szText,
                    _In_ IDWriteTextFormat* _pTextFormat,
                    _In_ FontStyle _fTextStyle,
                    _In_ Size _Maxbound,
                    _COM_Outptr_ IDWriteTextLayout** _ppTextLayout);

                void
                __YYAPI
                DrawString(
                    _In_ ID2D1RenderTarget* pRenderTarget,
                    _In_ uStringView _szText,
                    _In_ const Font& _FontInfo,
                    _In_ ID2D1Brush *_pDefaultFillBrush,
                    _In_ const Rect& _LayoutRect,
                    _In_ ContentAlignStyle _fTextAlign);
             
                void
                __YYAPI
                MeasureString(
                    _In_ uStringView _szText,
                    _In_ const Font& _FontInfo,
                    _In_ const Size& _LayoutSize,
                    _In_ ContentAlignStyle _fTextAlign,
                    _Out_ Size* _pExtent);
            }
        }
    } // namespace Media
} // namespace YY

#pragma pack(pop)
