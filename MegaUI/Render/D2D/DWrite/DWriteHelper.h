#pragma once
#include <dwrite.h>

#include <Base/Memory/RefPtr.h>
#include <Multimedia/Font.h>

#include <MegaUI/Render/Render.h>

#pragma comment(lib, "Dwrite.lib")

namespace YY
{
    namespace MegaUI
    {
        class DWriteHelper
        {
        private:
            IDWriteFactory* pDWriteFactory = nullptr;

        public:
            DWriteHelper()
            {
            }

            ~DWriteHelper()
            {
                if (pDWriteFactory)
                    pDWriteFactory->Release();
            }

            _Ret_maybenull_ IDWriteFactory* __YYAPI TryGetDWriteFactory()
            {
                if (pDWriteFactory)
                    return pDWriteFactory;

                auto _hr = DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&pDWriteFactory));

                return pDWriteFactory;
            }

            HRESULT
            __YYAPI CreateTextFormat(
                _In_ const Font& _FontInfo,
                _In_ ContentAlignStyle _fTextAlign,
                _In_z_ uchar_t const* _szLocaleName,
                _COM_Outptr_ IDWriteTextFormat** _ppTextFormat
                )
            {
                if (!_ppTextFormat)
                    return E_INVALIDARG;
                *_ppTextFormat = nullptr;

                auto _pDWriteFactory = TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                DWRITE_FONT_STYLE _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
                if (HasFlags(_FontInfo.fStyle, FontStyle::Italic))
                    _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;
                
                RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = _pDWriteFactory->CreateTextFormat(
                    _FontInfo.szFace,
                    nullptr,
                    DWRITE_FONT_WEIGHT(_FontInfo.uWeight),
                    _eFontStyle,
                    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
                    _FontInfo.iSize,
                    _szLocaleName,
                    &_pTextFormat);
                if (FAILED(_hr))
                    return _hr;

                DWRITE_TEXT_ALIGNMENT _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
                if (HasFlags(_fTextAlign, ContentAlignStyle::Right))
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Center))
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
                }

                _hr = _pTextFormat->SetTextAlignment(_TextAlignment);
                if (FAILED(_hr))
                    return _hr;

                DWRITE_PARAGRAPH_ALIGNMENT _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                if (HasFlags(_fTextAlign, ContentAlignStyle::Bottom))
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Middle))
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
                }
                _hr = _pTextFormat->SetParagraphAlignment(_ParagraphAlignment);
                if (FAILED(_hr))
                    return _hr;

                auto _WordWrapping = HasFlags(_fTextAlign, ContentAlignStyle::Wrap) ? DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP;
                _hr = _pTextFormat->SetWordWrapping(_WordWrapping);
                if (FAILED(_hr))
                    return _hr;

                if (HasFlags(_fTextAlign, ContentAlignStyle::EndEllipsis))
                {
                    RefPtr<IDWriteInlineObject> pDWriteInlineObject;
                    auto _hr = _pDWriteFactory->CreateEllipsisTrimmingSign(_pTextFormat, &pDWriteInlineObject);
                    if (SUCCEEDED(_hr))
                    {
                        DWRITE_TRIMMING trim;
                        trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
                        trim.delimiter = 1;
                        trim.delimiterCount = 3;
                        _pTextFormat->SetTrimming(&trim, pDWriteInlineObject);
                    }
                }

                *_ppTextFormat = _pTextFormat.Detach();
                return S_OK;
            }

            HRESULT
            __YYAPI CreateTextLayout(
                _In_ uStringView _szText,
                _In_ IDWriteTextFormat* _pTextFormat,
                _In_ FontStyle _fTextStyle,
                _In_ Size _Maxbound,
                _COM_Outptr_ IDWriteTextLayout** _ppTextLayout
                )
            {
                if (!_ppTextLayout)
                    return E_INVALIDARG;
                *_ppTextLayout = nullptr;
                
                if (_szText.GetSize() > uint32_max)
                    return E_UNEXPECTED;

                auto _pDWriteFactory = TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                RefPtr<IDWriteTextLayout> _pTextLayout;
                auto _hr = _pDWriteFactory->CreateTextLayout(
                    _szText.GetConstString(),
                    (UINT32)_szText.GetSize(),
                    _pTextFormat,
                    _Maxbound.Width,
                    _Maxbound.Height,
                    &_pTextLayout);
                if (FAILED(_hr))
                    return _hr;

                if (HasFlags(_fTextStyle, FontStyle::Underline))
                {
                    _hr = _pTextLayout->SetUnderline(TRUE, DWRITE_TEXT_RANGE {0, (UINT32)_szText.GetSize()});
                    if (FAILED(_hr))
                        return _hr;
                }

                if (HasFlags((FontStyle)_fTextStyle, FontStyle::StrikeOut))
                {
                    _hr = _pTextLayout->SetStrikethrough(TRUE, DWRITE_TEXT_RANGE {0, (UINT32)_szText.GetSize()});
                    if (FAILED(_hr))
                        return _hr;
                }

                *_ppTextLayout = _pTextLayout.Detach();
                return S_OK;
            }

            void
            __YYAPI
            DrawString(
                _In_ ID2D1RenderTarget* pRenderTarget,
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ Color _crTextColor,
                _In_ const Rect& _LayoutRect,
                _In_ ContentAlignStyle _fTextAlign
                )
            {
                if (_szText.GetSize() == 0 || _LayoutRect.IsEmpty() || pRenderTarget == nullptr)
                    return;

                RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = CreateTextFormat(_FontInfo, _fTextAlign, L"", &_pTextFormat);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                RefPtr<IDWriteTextLayout> _pTextLayout;
                _hr = CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutRect.GetSize(), &_pTextLayout);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                RefPtr<ID2D1SolidColorBrush> _pDefaultFillBrush;
                _hr = pRenderTarget->CreateSolidColorBrush(_crTextColor, &_pDefaultFillBrush);
                if (FAILED(_hr))
                    return;

                pRenderTarget->DrawTextLayout(
                    _LayoutRect.GetPoint(),
                    _pTextLayout,
                    _pDefaultFillBrush,
                    D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);

                return;
            }
             
            void
            __YYAPI
            MeasureString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Size& _LayoutSize,
                _In_ ContentAlignStyle _fTextAlign,
                _Out_ Size* _pExtent
                )
            {
                _pExtent->Width = 0;
                _pExtent->Height = 0;

                RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = CreateTextFormat(_FontInfo, _fTextAlign & ~ContentAlignStyle::EndEllipsis, L"", &_pTextFormat);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                RefPtr<IDWriteTextLayout> _pTextLayout;
                _hr = CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutSize, &_pTextLayout);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                DWRITE_TEXT_METRICS TextMetrics;
                _hr = _pTextLayout->GetMetrics(&TextMetrics);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                _pExtent->Width = TextMetrics.widthIncludingTrailingWhitespace;
                _pExtent->Height = TextMetrics.height;
            }
        };
    }
} // namespace YY