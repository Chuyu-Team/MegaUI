#include "pch.h"
#include "DWriteHelper.h"

#include <YY/Base/Sync/Interlocked.h>
#include <YY/Base/Memory/RefPtr.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

#pragma comment(lib, "dwrite.lib")

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            namespace DWrite
            {
#define INVALID_FACTORY (IDWriteFactory*)-1

                static IDWriteFactory* s_pDWriteFactory;

                IDWriteFactory* __YYAPI GetDWriteFactory()
                {
                    if (auto _pTmp = s_pDWriteFactory)
                    {
                        return _pTmp == INVALID_FACTORY ? nullptr : _pTmp;
                    }

                    IDWriteFactory* _pDWriteFactory = nullptr;
                    auto _hr = DWriteCreateFactory(
                        DWRITE_FACTORY_TYPE_SHARED,
                        __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&_pDWriteFactory));

                    if (FAILED(_hr) || _pDWriteFactory == nullptr)
                    {
                        YY::Base::Sync::CompareExchangePoint(&s_pDWriteFactory, INVALID_FACTORY, (IDWriteFactory*)nullptr);
                        return nullptr;
                    }

                    auto _pLast = YY::Base::Sync::CompareExchangePoint(&s_pDWriteFactory, _pDWriteFactory, (IDWriteFactory*)nullptr);
                    if (_pLast != nullptr)
                    {
                        // 其他线程已经初始化，保存最后状态，这有助于释放时。
                        _pDWriteFactory->Release();
                        return _pLast == INVALID_FACTORY ? nullptr : _pLast;
                    }

                    // 注册释放
                    atexit(
                        []()
                        {
                            auto _pLast = YY::Base::Sync::ExchangePoint(&s_pDWriteFactory, INVALID_FACTORY);
                            if (_pLast && _pLast != INVALID_FACTORY)
                                _pLast->Release();
                        });

                    return _pDWriteFactory;
                }

                HRESULT __YYAPI CreateTextFormat(const Font& _FontInfo, ContentAlignStyle _fTextAlign, uchar_t const* _szLocaleName, IDWriteTextFormat** _ppTextFormat)
                {
                    if (!_ppTextFormat)
                        return E_INVALIDARG;
                    *_ppTextFormat = nullptr;

                    auto _pDWriteFactory = GetDWriteFactory();
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
                        (int)_FontInfo.iSize,
                        _szLocaleName,
                        _pTextFormat.ReleaseAndGetAddressOf());
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
                        auto _hr = _pDWriteFactory->CreateEllipsisTrimmingSign(_pTextFormat, pDWriteInlineObject.ReleaseAndGetAddressOf());
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

                HRESULT __YYAPI CreateTextLayout(uStringView _szText, IDWriteTextFormat* _pTextFormat, FontStyle _fTextStyle, Size _Maxbound, IDWriteTextLayout** _ppTextLayout)
                {
                    if (!_ppTextLayout)
                        return E_INVALIDARG;
                    *_ppTextLayout = nullptr;

                    if (_szText.GetSize() > (std::numeric_limits<UINT32>::max)())
                        return E_UNEXPECTED;

                    auto _pDWriteFactory = GetDWriteFactory();
                    if (!_pDWriteFactory)
                        return E_NOINTERFACE;

                    RefPtr<IDWriteTextLayout> _pTextLayout;
                    auto _hr = _pDWriteFactory->CreateTextLayout(
                        _szText.GetConstString(),
                        (UINT32)_szText.GetSize(),
                        _pTextFormat,
                        _Maxbound.Width,
                        _Maxbound.Height,
                        _pTextLayout.ReleaseAndGetAddressOf());
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

                void __YYAPI DrawString(ID2D1RenderTarget* pRenderTarget, uStringView _szText, const Font& _FontInfo, ID2D1Brush* _pDefaultFillBrush, const Rect& _LayoutRect, ContentAlignStyle _fTextAlign)
                {
                    if (_szText.GetSize() == 0 || _LayoutRect.IsEmpty() || pRenderTarget == nullptr)
                        return;

                    RefPtr<IDWriteTextFormat> _pTextFormat;
                    auto _hr = CreateTextFormat(_FontInfo, _fTextAlign, L"", _pTextFormat.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                    {
                        throw Exception(_hr);
                    }

                    RefPtr<IDWriteTextLayout> _pTextLayout;
                    _hr = CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutRect.GetSize(), _pTextLayout.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                    {
                        throw Exception(_hr);
                    }

                    pRenderTarget->DrawTextLayout(
                        _LayoutRect.GetPoint(),
                        _pTextLayout,
                        _pDefaultFillBrush,
                        D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);

                    return;
                }

                void __YYAPI MeasureString(uStringView _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign, Size* _pExtent)
                {
                    _pExtent->Width = 0;
                    _pExtent->Height = 0;

                    RefPtr<IDWriteTextFormat> _pTextFormat;
                    auto _hr = CreateTextFormat(_FontInfo, _fTextAlign & ~ContentAlignStyle::EndEllipsis, L"", _pTextFormat.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                    {
                        throw Exception(_hr);
                    }

                    RefPtr<IDWriteTextLayout> _pTextLayout;
                    _hr = CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutSize, _pTextLayout.ReleaseAndGetAddressOf());
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
            } // namespace DWrite
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
