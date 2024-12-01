#pragma once
#include <dwrite.h>
#include <gdiplus.h>

#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Strings/String.h>
#include <Media/Graphics/GDIPlus/GDIPlusHelper.h>
#include <Base/Containers/Array.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class GDIPlusDrawContext;

            struct GdiPlusTextLayoutDrawContext
            {
                GDIPlusDrawContext* pDrawContext;
                Brush oBrush;
            };

            class GdiPlusTextLayout : public IDWriteTextLayout
            {
            private:
                uint32_t uRef = 1;

                DWRITE_TEXT_ALIGNMENT eTextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
                DWRITE_PARAGRAPH_ALIGNMENT eParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                DWRITE_WORD_WRAPPING eWordWrapping = DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP;
                DWRITE_READING_DIRECTION eReadingDirection = DWRITE_READING_DIRECTION::DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
                DWRITE_FLOW_DIRECTION eFlowDirection = DWRITE_FLOW_DIRECTION::DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
                uString szFontFamilyName;
                DWRITE_FONT_WEIGHT uFontWeigth = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
                DWRITE_FONT_STYLE eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
                DWRITE_FONT_STRETCH eFontStretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_UNDEFINED;
                FLOAT nFontSize = 0;

                uString szText;
                FLOAT nMaxWidth = UINT16_MAX;
                FLOAT nMaxHeight = UINT16_MAX;

                //Gdiplus::FontFamily oFontFamily;
                //Gdiplus::Font oFont;
                //Gdiplus::StringFormat oFormat;

                //Array<WordInfo> oWordInfos;

                void InvalidCache()
                {
                    //oWordInfos.Clear();
                }

                void BuildCache()
                {
                    //if (szText.GetSize() == 0)
                    //{
                    //    return;
                    //}

                    //if (oWordInfos.IsEmpty())
                    //{
                    //    return;
                    //}

                    //// GetTextExtentExPointW()
                    //auto _hDc = ::GetDC(NULL);
                    //LOGFONTW _Font;
                    //_Font.lfHeight = nFontSize;
                    //_Font.lfWidth = 0;
                    //_Font.lfEscapement = 0;
                    //_Font.lfOrientation = 0;
                    //_Font.lfWeight = uFontWeigth;
                    //_Font.lfItalic = eFontStyle == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC || eFontStyle == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE;
                    //_Font.lfUnderline = FALSE;
                    //_Font.lfStrikeOut = 0;
                    //_Font.lfCharSet = DEFAULT_CHARSET;
                    //_Font.lfOutPrecision = OUT_DEFAULT_PRECIS;
                    //_Font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
                    //_Font.lfQuality = CLEARTYPE_QUALITY;
                    //_Font.lfPitchAndFamily = 0;
                    //wcscat_s(_Font.lfFaceName, szFontFamilyName);
                    //auto _hFont = CreateFontIndirectW(&_Font);
                    //auto _hOldFont = SelectObject(_hDc, _hFont);

                    //for (auto _szText = szText.GetConstString(); *_szText;)
                    //{
                    //    auto _szNext = CharNextW(_szText);
                    //    uStringView _szWord(_szText, _szNext - _szText);
                    //    ABCFLOAT _Abc = {};
                    //    GetCharABCWidthsFloatW(_hDc, _szWord[0], _szWord[0], &_Abc);
                    //    _szText = _szNext;

                    //    oWordInfos.EmplacePtr(_szWord, _Abc);
                    //}
                    //SelectObject(_hDc, _hOldFont);
                    //ReleaseDC(nullptr, _hDc);
                }

                static Gdiplus::StringAlignment GetGdiplusFontAlignment(DWRITE_TEXT_ALIGNMENT _eTextAlignment)
                {
                    switch (_eTextAlignment)
                    {
                    case DWRITE_TEXT_ALIGNMENT_LEADING:
                        return Gdiplus::StringAlignment::StringAlignmentNear;
                    case DWRITE_TEXT_ALIGNMENT_TRAILING:
                        return Gdiplus::StringAlignment::StringAlignmentFar;
                    case DWRITE_TEXT_ALIGNMENT_CENTER:
                        return Gdiplus::StringAlignment::StringAlignmentCenter;
                    case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
                    default:
                        return Gdiplus::StringAlignment::StringAlignmentNear;
                    }
                }

                static Gdiplus::StringAlignment GetGdiplusLineAlignment(DWRITE_PARAGRAPH_ALIGNMENT _eParagraphAlignment)
                {
                    switch (_eParagraphAlignment)
                    {
                    case DWRITE_PARAGRAPH_ALIGNMENT_NEAR:
                        return Gdiplus::StringAlignment::StringAlignmentNear;
                    case DWRITE_PARAGRAPH_ALIGNMENT_FAR:
                        return Gdiplus::StringAlignment::StringAlignmentFar;
                    case DWRITE_PARAGRAPH_ALIGNMENT_CENTER:
                        return Gdiplus::StringAlignment::StringAlignmentCenter;
                    default:
                        return Gdiplus::StringAlignment::StringAlignmentNear;
                    }
                }

                static int32_t __YYAPI GetGdiplusFontStyle(_In_ DWRITE_FONT_WEIGHT _uFontWeigth, DWRITE_FONT_STYLE _eFontStyle, bool _bUnderline, bool _bStrikeOut)
                {
                    int32_t _fFontStyle = 0;

                    if (_uFontWeigth <= DWRITE_FONT_WEIGHT_MEDIUM)
                    {
                        // Regular
                        if (_eFontStyle == DWRITE_FONT_STYLE_ITALIC)
                        {
                            _fFontStyle = Gdiplus::FontStyleItalic;
                        }
                        else
                        {
                            _fFontStyle = Gdiplus::FontStyleRegular;
                        }
                    }
                    else
                    {
                        // Bold
                        if (_eFontStyle == DWRITE_FONT_STYLE_ITALIC)
                        {
                            _fFontStyle = Gdiplus::FontStyleBoldItalic;
                        }
                        else
                        {
                            _fFontStyle = Gdiplus::FontStyleBold;
                        }
                    }

                    if (_bUnderline)
                    {
                        _fFontStyle |= Gdiplus::FontStyleUnderline;
                    }

                    if (_bStrikeOut)
                    {
                        _fFontStyle |= Gdiplus::FontStyleStrikeout;
                    }

                    return _fFontStyle;
                }

            public:
                GdiPlusTextLayout(
                    _In_z_ WCHAR const* _szFontFamilyName,
                    _In_opt_ IDWriteFontCollection* _pFontCollection,
                    DWRITE_FONT_WEIGHT _uFontWeight,
                    DWRITE_FONT_STYLE _eFontStyle,
                    DWRITE_FONT_STRETCH _eFontStretch,
                    FLOAT _nFontSize,
                    uString _szText,
                    FLOAT _nMaxWidth,
                    FLOAT _nMaxHeight)
                    :szFontFamilyName(_szFontFamilyName)
                    , uFontWeigth(_uFontWeight)
                    , eFontStyle(_eFontStyle)
                    , eFontStretch(_eFontStretch)
                    , nFontSize(_nFontSize)
                    , szText(_szText)
                    , nMaxWidth(_nMaxWidth)
                    , nMaxHeight(_nMaxHeight)
                {
                }

                ////////////////////////////////////////////////////////////////
                // IUnknown

                HRESULT STDMETHODCALLTYPE QueryInterface(
                    /* [in] */ REFIID riid,
                    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
                {
                    if (!ppvObject)
                        return E_POINTER;
                    *ppvObject = nullptr;

                    if (riid == __uuidof(IDWriteTextLayout)
                        || riid == __uuidof(IDWriteTextFormat)
                        || riid == __uuidof(IUnknown))
                    {
                        *ppvObject = static_cast<IDWriteTextLayout*>(this);
                        AddRef();
                        return S_OK;
                    }
                    else
                    {
                        return E_NOINTERFACE;
                    }
                }

                ULONG STDMETHODCALLTYPE AddRef(void) override
                {
                    return YY::Sync::Increment(&uRef);
                }

                ULONG STDMETHODCALLTYPE Release(void) override
                {
                    const auto _uNewRef = YY::Sync::Decrement(&uRef);
                    if (_uNewRef == 0)
                    {
                        delete this;
                    }

                    return _uNewRef;
                }

                // IUnknown
                ////////////////////////////////////////////////////////////////

                ////////////////////////////////////////////////////////////////
                // IDWriteTextFormat

                HRESULT STDMETHODCALLTYPE SetTextAlignment(DWRITE_TEXT_ALIGNMENT _eTextAlignment) override
                {
                    if (eTextAlignment != _eTextAlignment)
                    {
                        eTextAlignment = _eTextAlignment;
                        InvalidCache();
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT _eParagraphAlignment) override
                {
                    if (eParagraphAlignment != _eParagraphAlignment)
                    {
                        eParagraphAlignment = _eParagraphAlignment;
                        InvalidCache();
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetWordWrapping(DWRITE_WORD_WRAPPING _eWordWrapping) override
                {
                    if (eWordWrapping != _eWordWrapping)
                    {
                        eWordWrapping = _eWordWrapping;
                        InvalidCache();
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetReadingDirection(DWRITE_READING_DIRECTION _eReadingDirection) override
                {
                    if (eReadingDirection != _eReadingDirection)
                    {
                        eReadingDirection = _eReadingDirection;
                        InvalidCache();
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetFlowDirection(DWRITE_FLOW_DIRECTION _eFlowDirection) override
                {
                    if (eFlowDirection != _eFlowDirection)
                    {
                        eFlowDirection = _eFlowDirection;
                        InvalidCache();
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetIncrementalTabStop(FLOAT _nIncrementalTabStop) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetTrimming(
                    _In_ DWRITE_TRIMMING const* trimmingOptions,
                    _In_opt_ IDWriteInlineObject* trimmingSign
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetLineSpacing(
                    DWRITE_LINE_SPACING_METHOD lineSpacingMethod,
                    FLOAT lineSpacing,
                    FLOAT baseline
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                DWRITE_TEXT_ALIGNMENT STDMETHODCALLTYPE GetTextAlignment() override
                {
                    return eTextAlignment;
                }

                DWRITE_PARAGRAPH_ALIGNMENT STDMETHODCALLTYPE GetParagraphAlignment() override
                {
                    return eParagraphAlignment;
                }

                DWRITE_WORD_WRAPPING STDMETHODCALLTYPE GetWordWrapping() override
                {
                    return eWordWrapping;
                }

                DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetReadingDirection() override
                {
                    return eReadingDirection;
                }

                DWRITE_FLOW_DIRECTION STDMETHODCALLTYPE GetFlowDirection() override
                {
                    return eFlowDirection;
                }

                FLOAT STDMETHODCALLTYPE GetIncrementalTabStop() override
                {
                    // TODO
                    return 0;
                }

                HRESULT STDMETHODCALLTYPE GetTrimming(
                    _Out_ DWRITE_TRIMMING* trimmingOptions,
                    _COM_Outptr_ IDWriteInlineObject** trimmingSign
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetLineSpacing(
                    _Out_ DWRITE_LINE_SPACING_METHOD* lineSpacingMethod,
                    _Out_ FLOAT* lineSpacing,
                    _Out_ FLOAT* baseline
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontCollection(
                    _COM_Outptr_ IDWriteFontCollection** fontCollection
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                UINT32 STDMETHODCALLTYPE GetFontFamilyNameLength() override
                {
                    return szFontFamilyName.GetSize();
                }

                HRESULT STDMETHODCALLTYPE GetFontFamilyName(
                    _Out_writes_z_(_uNameSize) WCHAR* _szFontFamilyName,
                    UINT32 _uNameSize
                    ) override
                {
                    if (!_szFontFamilyName)
                        return E_POINTER;

                    const auto _cchFontFamilyName = szFontFamilyName.GetSize();
                    if (_uNameSize <= _cchFontFamilyName)
                    {
                        return __HRESULT_FROM_WIN32(ERROR_MORE_DATA);
                    }

                    memcpy(_szFontFamilyName, szFontFamilyName.GetConstString(), _cchFontFamilyName * sizeof(_szFontFamilyName[0]));
                    _szFontFamilyName[_cchFontFamilyName] = L'\0';
                    return S_OK;
                }

                DWRITE_FONT_WEIGHT STDMETHODCALLTYPE GetFontWeight() override
                {
                    return uFontWeigth;
                }

                DWRITE_FONT_STYLE STDMETHODCALLTYPE GetFontStyle() override
                {
                    return eFontStyle;
                }

                DWRITE_FONT_STRETCH STDMETHODCALLTYPE GetFontStretch() override
                {
                    return eFontStretch;
                }

                FLOAT STDMETHODCALLTYPE GetFontSize() override
                {
                    return nFontSize;
                }

                UINT32 STDMETHODCALLTYPE GetLocaleNameLength() override
                {
                    // TODO
                    return 0;
                }

                HRESULT STDMETHODCALLTYPE GetLocaleName(
                    _Out_writes_z_(nameSize) WCHAR* localeName,
                    UINT32 nameSize
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                // IDWriteTextFormat
                ////////////////////////////////////////////////////////////////

                ////////////////////////////////////////////////////////////////
                // IDWriteTextLayout

                HRESULT STDMETHODCALLTYPE SetMaxWidth(FLOAT _nMaxWidth) override
                {
                    nMaxWidth = _nMaxWidth;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetMaxHeight(FLOAT _nMaxHeight) override
                {
                    nMaxHeight = _nMaxHeight;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SetFontCollection(
                    _In_ IDWriteFontCollection* fontCollection,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetFontFamilyName(
                    _In_z_ WCHAR const* fontFamilyName,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetFontWeight(
                    DWRITE_FONT_WEIGHT fontWeight,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetFontStyle(
                    DWRITE_FONT_STYLE fontStyle,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetFontStretch(
                    DWRITE_FONT_STRETCH fontStretch,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetFontSize(
                    FLOAT fontSize,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetUnderline(
                    BOOL hasUnderline,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetStrikethrough(
                    BOOL hasStrikethrough,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetDrawingEffect(
                    IUnknown* drawingEffect,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetInlineObject(
                    _In_ IDWriteInlineObject* inlineObject,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetTypography(
                    _In_ IDWriteTypography* typography,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE SetLocaleName(
                    _In_z_ WCHAR const* localeName,
                    DWRITE_TEXT_RANGE textRange
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                FLOAT STDMETHODCALLTYPE GetMaxWidth() override
                {
                    return nMaxWidth;
                }

                FLOAT STDMETHODCALLTYPE GetMaxHeight() override
                {
                    return nMaxHeight;
                }

                HRESULT STDMETHODCALLTYPE GetFontCollection(
                    UINT32 currentPosition,
                    _COM_Outptr_ IDWriteFontCollection** fontCollection,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontFamilyNameLength(
                    UINT32 currentPosition,
                    _Out_ UINT32* nameLength,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontFamilyName(
                    UINT32 currentPosition,
                    _Out_writes_z_(nameSize) WCHAR* fontFamilyName,
                    UINT32 nameSize,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    // TODO
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontWeight(
                    UINT32 currentPosition,
                    _Out_ DWRITE_FONT_WEIGHT* fontWeight,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontStyle(
                    UINT32 currentPosition,
                    _Out_ DWRITE_FONT_STYLE* fontStyle,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontStretch(
                    UINT32 currentPosition,
                    _Out_ DWRITE_FONT_STRETCH* fontStretch,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetFontSize(
                    UINT32 currentPosition,
                    _Out_ FLOAT* fontSize,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetUnderline(
                    UINT32 currentPosition,
                    _Out_ BOOL* hasUnderline,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetStrikethrough(
                    UINT32 currentPosition,
                    _Out_ BOOL* hasStrikethrough,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetDrawingEffect(
                    UINT32 currentPosition,
                    _COM_Outptr_ IUnknown** drawingEffect,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetInlineObject(
                    UINT32 currentPosition,
                    _COM_Outptr_ IDWriteInlineObject** inlineObject,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetTypography(
                    UINT32 currentPosition,
                    _COM_Outptr_ IDWriteTypography** typography,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetLocaleNameLength(
                    UINT32 currentPosition,
                    _Out_ UINT32* nameLength,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetLocaleName(
                    UINT32 currentPosition,
                    _Out_writes_z_(nameSize) WCHAR* localeName,
                    UINT32 nameSize,
                    _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE Draw(
                    _In_opt_ void* clientDrawingContext,
                    _In_ IDWriteTextRenderer* renderer,
                    FLOAT originX,
                    FLOAT originY
                    ) override
                {
                    auto _pDrawContext = reinterpret_cast<GdiPlusTextLayoutDrawContext*>(clientDrawingContext);

                    auto _pNativeBrush = _pDrawContext->pDrawContext->GetNativeBrush(_pDrawContext->oBrush);
                    if (!_pNativeBrush)
                        return E_FAIL;

                    Gdiplus::FontFamily _FontFamily(szFontFamilyName);
                    Gdiplus::Font _Font(&_FontFamily, nFontSize, GetGdiplusFontStyle(uFontWeigth, eFontStyle, false, false), Gdiplus::Unit::UnitPixel);

                    int32_t _fStringFormatFlags = Gdiplus::StringFormatFlagsMeasureTrailingSpaces;
                    if (eWordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP)
                        _fStringFormatFlags |= Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap;

                    Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                    _Format.SetFormatFlags(_fStringFormatFlags);
                    _Format.SetAlignment(GetGdiplusFontAlignment(eTextAlignment));
                    _Format.SetLineAlignment(GetGdiplusLineAlignment(eParagraphAlignment));

                    //if (HasFlags(_fTextAlign, ContentAlignStyle::EndEllipsis))
                    //    _Format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);

                    _pDrawContext->pDrawContext->oSurface.DrawString(
                        szText.GetConstString(), (INT)szText.GetSize(), &_Font, Gdiplus::RectF(originX, originY, nMaxWidth, nMaxHeight), &_Format, _pNativeBrush);
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE GetLineMetrics(
                    _Out_writes_opt_(maxLineCount) DWRITE_LINE_METRICS* lineMetrics,
                    UINT32 maxLineCount,
                    _Out_ UINT32* actualLineCount
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetMetrics(
                    _Out_ DWRITE_TEXT_METRICS* textMetrics
                    ) override
                {
                    *textMetrics = DWRITE_TEXT_METRICS{};
                    AutoGdiplusStartup::AutoStartup();
                    Gdiplus::Graphics _oTmpSurface((HWND)nullptr, FALSE);
                    Gdiplus::FontFamily _FontFamily(szFontFamilyName);
                    Gdiplus::Font _Font(&_FontFamily, nFontSize, GetGdiplusFontStyle(uFontWeigth, eFontStyle, false, false), Gdiplus::Unit::UnitPixel);

                    int32_t _fStringFormatFlags = Gdiplus::StringFormatFlagsMeasureTrailingSpaces;
                    if (eWordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP)
                        _fStringFormatFlags |= Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap;

                    Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                    _Format.SetFormatFlags(_fStringFormatFlags);
                    _Format.SetAlignment(GetGdiplusFontAlignment(eTextAlignment));
                    _Format.SetLineAlignment(GetGdiplusLineAlignment(eParagraphAlignment));

                    uStringView _szStr = szText;
                    auto _uMax = szText.GetSize();

                    Gdiplus::SizeF _Extent;
                    _oTmpSurface.MeasureString(
                        _szStr.GetConstString(),
                        (INT)_uMax,
                        &_Font,
                        Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                        &_Format,
                        &_Extent);

                    textMetrics->height = _Extent.Height;
                    textMetrics->width = textMetrics->widthIncludingTrailingWhitespace = _Extent.Width;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE GetOverhangMetrics(
                    _Out_ DWRITE_OVERHANG_METRICS* overhangs
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE GetClusterMetrics(
                    _Out_writes_opt_(maxClusterCount) DWRITE_CLUSTER_METRICS* clusterMetrics,
                    UINT32 maxClusterCount,
                    _Out_ UINT32* actualClusterCount
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE DetermineMinWidth(
                    _Out_ FLOAT* minWidth
                    ) override
                {
                    return E_FAIL;
                }

                HRESULT STDMETHODCALLTYPE HitTestPoint(
                    FLOAT pointX,
                    FLOAT pointY,
                    _Out_ BOOL* isTrailingHit,
                    _Out_ BOOL* isInside,
                    _Out_ DWRITE_HIT_TEST_METRICS* hitTestMetrics
                    ) override
                {
                    if (eWordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP)
                    {
                        return SingleLineHitTestPoint(pointX, pointY, isTrailingHit, isInside, hitTestMetrics);
                    }

                    return E_NOTIMPL;
                }

                HRESULT STDMETHODCALLTYPE HitTestTextPosition(
                    UINT32 _uTextPosition,
                    BOOL _bTrailingHit,
                    _Out_ FLOAT* _pnPointX,
                    _Out_ FLOAT* _pnPointY,
                    _Out_ DWRITE_HIT_TEST_METRICS* _poHitTestMetrics
                    ) override
                {
                    if (eWordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP)
                    {
                        return SingleLineHitTestTextPosition(_uTextPosition, _bTrailingHit, _pnPointX, _pnPointY, _poHitTestMetrics);
                    }

                    return E_NOTIMPL;
                }

                HRESULT STDMETHODCALLTYPE HitTestTextRange(
                    UINT32 textPosition,
                    UINT32 textLength,
                    FLOAT originX,
                    FLOAT originY,
                    _Out_writes_opt_(maxHitTestMetricsCount) DWRITE_HIT_TEST_METRICS* hitTestMetrics,
                    UINT32 maxHitTestMetricsCount,
                    _Out_ UINT32* actualHitTestMetricsCount
                    ) override
                {
                    if (eWordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP)
                    {
                        return SingleLineHitTestTextRange(textPosition, textLength, originX, originY, hitTestMetrics, maxHitTestMetricsCount, actualHitTestMetricsCount);
                    }
                    return E_NOTIMPL;
                }

            private:
                // UTF16平面的第一个字符
                static bool __fastcall IsHighSurrogateArea(wchar_t _ch)
                {
                    return (uint16_t(_ch) >= 0xD800u && uint16_t(_ch) <= 0xDBFFu);
                }

                // UTF16平面的第二个字符
                static bool __fastcall IsLowSurrogateArea(wchar_t _ch)
                {
                    return (uint16_t(_ch) >= 0xDC00u && uint16_t(_ch) <= 0xDFFFu);
                }

                static LPCWSTR __fastcall NextChar(_In_z_ LPCWSTR _szCharStart)
                {
                    if (!_szCharStart)
                        return nullptr;

                    const auto _ch = *_szCharStart;
                    if (_ch == 0)
                        return _szCharStart;

                    if (IsHighSurrogateArea(_ch) && IsLowSurrogateArea(_szCharStart[1]))
                    {
                        // 辅助平面,第一个代理字节时判断下一个字节是否是第二个代理字节，用于编码合法性检测。
                        ++_szCharStart;
                    }

                    return CharNextW(_szCharStart);
                }

                static LPCWSTR __fastcall PrevChar(_In_z_ LPCWSTR _szText, _In_z_ LPCWSTR _szCharStart)
                {
                    if (_szText == _szCharStart)
                        return _szText;

                    if (_szText == _szCharStart - 1)
                    {
                        return _szText;
                    }

                    if (IsLowSurrogateArea(*_szCharStart) && IsHighSurrogateArea(_szCharStart[-1]))
                    {
                        // 辅助平面,第二个代理字节时，尝试移动到代理第一个字节。
                        --_szCharStart;
                    }

                    auto _szPrev = CharPrevW(_szText, _szCharStart);
                    if (_szPrev == _szText)
                        return _szPrev;

                    if(IsLowSurrogateArea(*_szPrev) && IsHighSurrogateArea(_szPrev[-1]))
                    {
                        // 辅助平面,第二个代理字节时，尝试移动到代理第一个字节。
                        --_szPrev;
                    }

                    return _szPrev;
                }

                static size_t AdjustChar(uStringView _szText, size_t _uCurrentIndex, uint32_t* _puCharLength)
                {
                    if (_szText.GetSize() <= _uCurrentIndex)
                    {
                        *_puCharLength = 0;
                        return _szText.GetSize();
                    }

                    size_t _uStartIndex = _uCurrentIndex;

                    for (; _uStartIndex;)
                    {
                        WORD _fCharType;
                        if(!GetStringTypeW(CT_CTYPE3, _szText.GetConstString() + _uStartIndex, 1, &_fCharType))
                            break;

                        if (_fCharType & (C3_HIGHSURROGATE | C3_ALPHA))
                        {
                            break;
                        }
                        else if (_fCharType & (C3_LOWSURROGATE | C3_NONSPACING))
                        {
                            --_uStartIndex;
                        }
                        else
                        {
                            break;
                        }
                    }

                    size_t _uEndIndex = _uCurrentIndex + 1;
                    for (; _uEndIndex != _szText.GetSize();)
                    {
                        WORD _fCharType;
                        if (!GetStringTypeW(CT_CTYPE3, _szText.GetConstString() + _uEndIndex, 1, &_fCharType))
                            break;

                        if (_fCharType & (C3_HIGHSURROGATE | C3_ALPHA))
                        {
                            break;
                        }
                        else if (_fCharType & (C3_LOWSURROGATE | C3_NONSPACING))
                        {
                            ++_uEndIndex;
                        }
                        else
                        {
                            break;
                        }
                    }

                    *_puCharLength = _uEndIndex - _uStartIndex;
                    return _uStartIndex;
                }

                HRESULT STDMETHODCALLTYPE SingleLineHitTestPoint(
                    FLOAT pointX,
                    FLOAT pointY,
                    _Out_ BOOL* isTrailingHit,
                    _Out_ BOOL* isInside,
                    _Out_ DWRITE_HIT_TEST_METRICS* hitTestMetrics
                    )
                {
                    *isTrailingHit = FALSE;
                    *isInside = TRUE;
                    *hitTestMetrics = DWRITE_HIT_TEST_METRICS{};
                    if (pointX <= 0 || szText.GetSize() == 0)
                    {
                        hitTestMetrics->isText = TRUE;
                        hitTestMetrics->textPosition = 0;
                        hitTestMetrics->length = 0;
                        return S_OK;
                    }
                    AutoGdiplusStartup::AutoStartup();
                    Gdiplus::Graphics _oTmpSurface((HWND)nullptr, FALSE);
                    Gdiplus::FontFamily _FontFamily(szFontFamilyName);
                    Gdiplus::Font _Font(&_FontFamily, nFontSize, GetGdiplusFontStyle(uFontWeigth, eFontStyle, false, false), Gdiplus::Unit::UnitPixel);

                    int32_t _fStringFormatFlags = Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces;
                    Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                    _Format.SetFormatFlags(_fStringFormatFlags);
                    _Format.SetAlignment(GetGdiplusFontAlignment(eTextAlignment));
                    _Format.SetLineAlignment(GetGdiplusLineAlignment(eParagraphAlignment));

                    uStringView _szStr = szText;
                    auto _uMax = szText.GetSize();
                    Gdiplus::SizeF _ExtentMax;
                    size_t _uMin = 0;
                    Gdiplus::SizeF _ExtentMin;


                    Gdiplus::SizeF _Current;
                    _oTmpSurface.MeasureString(
                        _szStr.GetConstString(),
                        (INT)_uMax,
                        &_Font,
                        Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                        &_Format,
                        &_ExtentMax);

                    if (_ExtentMax.Width <= pointX)
                    {
                        *isTrailingHit = FALSE;
                        hitTestMetrics->isText = TRUE;
                        hitTestMetrics->textPosition = _uMax;
                        hitTestMetrics->length = 0;
                        return S_OK;
                    }

                    auto _uMidle = _uMax;
                    Gdiplus::SizeF _ExtentMidle = _ExtentMax;

                    for (;;)
                    {
                        if (_ExtentMidle.Width == pointX)
                        {
                            _uMin = _uMidle;
                            _ExtentMin = _ExtentMidle;
                            _uMax = _uMidle;
                            _ExtentMax = _ExtentMidle;
                            break;
                        }
                        else if (_ExtentMidle.Width > pointX)
                        {
                            _uMax = _uMidle;
                            _ExtentMax = _ExtentMidle;
                        }
                        else /*if (_ExtentMidle.Width < pointX)*/
                        {
                            _uMin = _uMidle;
                            _ExtentMin = _ExtentMidle;
                        }

                        uint32_t _cchLastCharLength = 1;
                        _uMidle = AdjustChar(_szStr, (_uMax + _uMin) / 2, &_cchLastCharLength);

                        if (_uMidle == _uMin)
                        {
                            _uMidle += _cchLastCharLength;
                        }

                        if (_uMidle >= _uMax)
                        {
                            // _uMin - _uMax之间只有一个字符！
                            break;
                        }

                        _oTmpSurface.MeasureString(
                            _szStr.GetConstString(),
                            (INT)_uMidle,
                            &_Font,
                            Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                            &_Format,
                            &_ExtentMidle);
                    }

                    hitTestMetrics->isText = TRUE;
                    hitTestMetrics->textPosition = _uMin;
                    hitTestMetrics->length = _uMax - _uMin;
                    if (_ExtentMin.Width < pointX)
                    {
                        if ((_ExtentMax.Width + _ExtentMin.Width) / 2 <= pointX)
                        {
                            *isTrailingHit = TRUE;
                        }
                    }
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SingleLineHitTestTextPosition(
                    UINT32 _uTextPosition,
                    BOOL _bTrailingHit,
                    _Out_ FLOAT* _pnPointX,
                    _Out_ FLOAT* _pnPointY,
                    _Out_ DWRITE_HIT_TEST_METRICS* _poHitTestMetrics
                    )
                {
                    *_pnPointX = 0;
                    *_pnPointY = 0;
                    *_poHitTestMetrics = DWRITE_HIT_TEST_METRICS{};

                    uStringView _szPreString;
                    uStringView _szCurrentChar;

                    uint32_t _uCharLength = 0;
                    _uTextPosition = AdjustChar(szText, _uTextPosition, &_uCharLength);

                    if (_uTextPosition != 0)
                    {
                        _szPreString.SetString(szText.GetConstString(), _uTextPosition);
                    }

                    if (_uTextPosition < szText.GetSize())
                    {
                        _szCurrentChar.SetString(szText.GetConstString() + _uTextPosition, _uCharLength);
                    }
                    /*auto _Status = s_AutoGdiplusStartup.TryGdiplusStartup();
                    if (_Status != Gdiplus::Status::Ok)
                        throw Exception();*/
                    AutoGdiplusStartup::AutoStartup();
                    Gdiplus::Graphics _oTmpSurface((HWND)nullptr, FALSE);
                    Gdiplus::FontFamily _FontFamily(szFontFamilyName);
                    Gdiplus::Font _Font(&_FontFamily, nFontSize, GetGdiplusFontStyle(uFontWeigth, eFontStyle, false, false), Gdiplus::Unit::UnitPixel);

                    int32_t _fStringFormatFlags = Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces;

                    Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                    _Format.SetFormatFlags(_fStringFormatFlags);
                    _Format.SetAlignment(GetGdiplusFontAlignment(eTextAlignment));
                    _Format.SetLineAlignment(GetGdiplusLineAlignment(eParagraphAlignment));

                    Gdiplus::SizeF _Extent;
                    if (_szPreString.GetSize())
                    {
                        _oTmpSurface.MeasureString(
                            _szPreString.GetConstString(),
                            (INT)_szPreString.GetSize(),
                            &_Font,
                            Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                            &_Format,
                            &_Extent);

                        *_pnPointX = _Extent.Width;
                        *_pnPointY = 0;
                    }

                    if (_szCurrentChar.GetSize())
                    {
                        _poHitTestMetrics->left = _Extent.Width;

                        _oTmpSurface.MeasureString(
                            _szCurrentChar.GetConstString(),
                            (INT)_szCurrentChar.GetSize(),
                            &_Font,
                            Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                            &_Format,
                            &_Extent);

                        _poHitTestMetrics->width = _Extent.Width;
                    }

                    _poHitTestMetrics->textPosition = _uTextPosition;
                    _poHitTestMetrics->length = _szCurrentChar.GetSize();
                    _poHitTestMetrics->isText = FALSE;
                    _poHitTestMetrics->height = _Extent.Height;
                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE SingleLineHitTestTextRange(
                    UINT32 textPosition,
                    UINT32 textLength,
                    FLOAT originX,
                    FLOAT originY,
                    _Out_writes_opt_(maxHitTestMetricsCount) DWRITE_HIT_TEST_METRICS* hitTestMetrics,
                    UINT32 maxHitTestMetricsCount,
                    _Out_ UINT32* actualHitTestMetricsCount
                    )
                {
                    *actualHitTestMetricsCount = 0;
                    if (textLength == 0 || textPosition >= szText.GetSize())
                    {
                        return S_OK;
                    }

                    auto _uTextPositionRight = textPosition + textLength;
                    if (_uTextPositionRight > szText.GetSize())
                    {
                        _uTextPositionRight = szText.GetSize();
                    }

                    if (textPosition == _uTextPositionRight)
                    {
                        return S_OK;
                    }

                    if (hitTestMetrics == nullptr || maxHitTestMetricsCount < 1)
                    {
                        *actualHitTestMetricsCount = 1;
                        return E_NOT_SUFFICIENT_BUFFER;
                    }
                    AutoGdiplusStartup::AutoStartup();
                    Gdiplus::Graphics _oTmpSurface((HWND)nullptr, FALSE);
                    Gdiplus::FontFamily _FontFamily(szFontFamilyName);
                    Gdiplus::Font _Font(&_FontFamily, nFontSize, GetGdiplusFontStyle(uFontWeigth, eFontStyle, false, false), Gdiplus::Unit::UnitPixel);

                    int32_t _fStringFormatFlags = Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces;

                    Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                    _Format.SetFormatFlags(_fStringFormatFlags);
                    _Format.SetAlignment(GetGdiplusFontAlignment(eTextAlignment));
                    _Format.SetLineAlignment(GetGdiplusLineAlignment(eParagraphAlignment));

                    Gdiplus::SizeF _ExtentLeft;
                    Gdiplus::SizeF _ExtentRigth;
                    if (textPosition)
                    {
                        _oTmpSurface.MeasureString(
                            szText.GetConstString(),
                            (INT)textPosition,
                            &_Font,
                            Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                            &_Format,
                            &_ExtentLeft);
                    }

                    _oTmpSurface.MeasureString(
                        szText.GetConstString(),
                        (INT)_uTextPositionRight,
                        &_Font,
                        Gdiplus::SizeF(nMaxWidth, nMaxHeight),
                        &_Format,
                        &_ExtentRigth);


                    DWRITE_HIT_TEST_METRICS&_HitTestMetrics = *hitTestMetrics;
                    _HitTestMetrics = DWRITE_HIT_TEST_METRICS{};
                    _HitTestMetrics.textPosition = textPosition;
                    _HitTestMetrics.length = textLength;
                    _HitTestMetrics.isText = TRUE;

                    _HitTestMetrics.left = originX + _ExtentLeft.Width;
                    _HitTestMetrics.top = originY;
                    _HitTestMetrics.width = _ExtentRigth.Width - _ExtentLeft.Width;
                    _HitTestMetrics.height = _ExtentRigth.Height;
                    *actualHitTestMetricsCount = 1;
                    return S_OK;
                }
            };
        }
    }
}
#pragma pack(pop)
