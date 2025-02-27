﻿#include "pch.h"
#include "GDIPlusDrawContext.h"
#include "GdiPlusTextLayout.hpp"

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

#pragma comment(lib, "gdiplus.lib")

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            static int32_t __YYAPI GetFontStyle(_In_ const Font& _FontInfo)
            {
                int32_t _fFontStyle = 0;

                if (_FontInfo.uWeight <= FontWeight::Medium)
                {
                    // Regular
                    if (HasFlags(_FontInfo.fStyle, FontStyle::Italic))
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
                    if (HasFlags(_FontInfo.fStyle, FontStyle::Italic))
                    {
                        _fFontStyle = Gdiplus::FontStyleBoldItalic;
                    }
                    else
                    {
                        _fFontStyle = Gdiplus::FontStyleBold;
                    }
                }

                if (HasFlags(_FontInfo.fStyle, FontStyle::Underline))
                {
                    _fFontStyle |= Gdiplus::FontStyleUnderline;
                }

                if (HasFlags(_FontInfo.fStyle, FontStyle::StrikeOut))
                {
                    _fFontStyle |= Gdiplus::FontStyleStrikeout;
                }

                return _fFontStyle;
            }

            //
            static Gdiplus::StringAlignment __YYAPI GetFontAlignment(_In_ ContentAlignStyle _fTextAlign)
            {
                if (HasFlags(_fTextAlign, ContentAlignStyle::Right))
                {
                    return Gdiplus::StringAlignmentFar;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Center))
                {
                    return Gdiplus::StringAlignmentCenter;
                }
                else
                {
                    return Gdiplus::StringAlignmentNear;
                }
            }

            static Gdiplus::StringAlignment __YYAPI GetFontLineAlignment(_In_ ContentAlignStyle _fTextAlign)
            {
                if (HasFlags(_fTextAlign, ContentAlignStyle::Bottom))
                {
                    return Gdiplus::StringAlignmentFar;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Middle))
                {
                    return Gdiplus::StringAlignmentCenter;
                }
                else
                {
                    return Gdiplus::StringAlignmentNear;
                }
            }

            GDIPlusDrawContext::GDIPlusDrawContext()
                : hWnd(NULL)
                , oPixelSize {}
                , uThreadId(0)
                , hDC(NULL)
                , bFirstPaint(true)
            {    
            }

            GDIPlusDrawContext::~GDIPlusDrawContext()
            {
                oSurface.Reset();
                oSurfaceBitmap.Reset();
            }

            DrawContextFactory* __YYAPI GDIPlusDrawContext::GetDrawContextFactory()
            {
                static DrawContextFactoryImpl<GDIPlusDrawContext> s_DrawContextFactory;
                return &s_DrawContextFactory;
            }

            bool __YYAPI GDIPlusDrawContext::IsSupport()
            {
                return true;
            }

            bool __YYAPI GDIPlusDrawContext::IsMicrosoftCompositionEngineSupport()
            {
                return false;
            }

            HRESULT __YYAPI GDIPlusDrawContext::CreateDrawTarget(HWND _hWnd, DrawContext** _ppDrawContext)
            {
                if (!_ppDrawContext)
                    return E_INVALIDARG;
                *_ppDrawContext = nullptr;

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return HRESULT_From_LSTATUS(GetLastError());

                auto _pDrawContext = new (std::nothrow) GDIPlusDrawContext();
                if (!_pDrawContext)
                    return E_OUTOFMEMORY;

                _pDrawContext->hWnd = _hWnd;
                _pDrawContext->oPixelSize.cx = _ClientRect.right - _ClientRect.left;
                _pDrawContext->oPixelSize.cy = _ClientRect.bottom - _ClientRect.top;
                *_ppDrawContext = _pDrawContext;
                return S_OK;
            }

            Size __YYAPI GDIPlusDrawContext::GetPixelSize()
            {
                return Size(oPixelSize.cx, oPixelSize.cy);
            }

            HRESULT __YYAPI GDIPlusDrawContext::SetPixelSize(const Size& _PixelSize)
            {
                oPixelSize.cx = _PixelSize.Width;
                oPixelSize.cy = _PixelSize.Height;
                return S_OK;
            }

            HRESULT __YYAPI GDIPlusDrawContext::BeginDraw(const Rect* _pPaint)
            {
                if (bFirstPaint)
                    _pPaint = nullptr;

                if (_pPaint && _pPaint->Left <= 0 && _pPaint->Top <= 0 && _pPaint->Left >= oPixelSize.cx && _pPaint->Bottom >= oPixelSize.cy)
                    _pPaint = nullptr;

                oPaint.Left = 0;
                oPaint.Top = 0;
                oPaint.Right = oPixelSize.cx;
                oPaint.Bottom = oPixelSize.cy;

                if (AutoGdiplusStartup::AutoStartup() != Gdiplus::Ok)
                {
                    return E_UNEXPECTED;
                }

                vecClip.Clear();

                hDC = ::GetDC(hWnd);

                UpdateTargetPixelSize(_pPaint != nullptr);

                auto _pSurface = GetSurface();
                if (!_pSurface)
                    return E_OUTOFMEMORY;

                _pSurface->Clear(Gdiplus::Color());
                _pSurface->ResetClip();
                
                if (_pPaint)
                {
                    oPaint &= *_pPaint;

                    _pSurface->SetClip(oPaint);
                }
                return S_OK;
            }

            HRESULT __YYAPI GDIPlusDrawContext::EndDraw()
            {
                if (!hDC)
                    return E_FAIL;
                
                bFirstPaint = false;

                {
                    Gdiplus::RectF PaintF(
                        Gdiplus::REAL(oPaint.Left),
                        Gdiplus::REAL(oPaint.Top),
                        Gdiplus::REAL(oPaint.GetWidth()),
                        Gdiplus::REAL(oPaint.GetHeight()));
                    
                    // ExcludeClipRect(hDC, oPaint.Left, oPaint.Top, oPaint.Right, oPaint.Bottom);

                    Gdiplus::Graphics DCSurface(hDC);
                    DCSurface.DrawImage(
                        &oSurfaceBitmap,
                        PaintF,
                        PaintF.X, PaintF.Y, PaintF.Width, PaintF.Height,
                        Gdiplus::UnitPixel);
                }

                ::ReleaseDC(hWnd, hDC);
                return S_OK;
            }

            void __YYAPI GDIPlusDrawContext::PushAxisAlignedClip(const Rect& _ClipRect)
            {
                Gdiplus::RectF _NewClipRect;

                if (auto _uSize = vecClip.GetSize())
                {
                    _NewClipRect = _ClipRect;
                    _NewClipRect.Intersect(vecClip[_uSize - 1]);
                }
                else
                {
                    _NewClipRect = _ClipRect & oPaint;
                }

                oSurface.SetClip(_NewClipRect);
                vecClip.Add(_NewClipRect);
            }

            void __YYAPI GDIPlusDrawContext::PopAxisAlignedClip()
            {
                auto _uSize = vecClip.GetSize();
                if (_uSize == 0)
                {
                    // 不对称？？？
                    return;
                }

                --_uSize;
                vecClip.Resize(_uSize);

                if (_uSize)
                {
                    oSurface.SetClip(vecClip[_uSize - 1]);
                }
                else
                {
                    oSurface.ResetClip();
                }
            }

            void __YYAPI GDIPlusDrawContext::DrawLine(Point _oPoint0, Point _oPoint1, Pen _oPen)
            {
                // todo
            }

            void __YYAPI GDIPlusDrawContext::DrawRectangle(const Rect& _oRect, Pen _oPen, Brush _oBrush)
            {
                if (_oBrush != nullptr)
                {
                    auto _pNativeBrush = GetNativeBrush(_oBrush);
                    if (_pNativeBrush != nullptr)
                    {
                        oSurface.FillRectangle(
                            _pNativeBrush,
                            _oRect);
                    }
                }

                if (_oPen != nullptr)
                {
                    auto _pNativePen = GetNativePen(_oPen);
                    if (_pNativePen != nullptr)
                    {
                        oSurface.DrawRectangle(_pNativePen, _oRect);
                    }
                }
            }

            void __YYAPI GDIPlusDrawContext::DrawString(uString _szText, const Font& _FontInfo, Brush _oBrush, const Rect& _LayoutRect, ContentAlignStyle _fTextAlign)
            {
                if (_oBrush == nullptr || _szText.GetSize() == 0 || _LayoutRect.IsEmpty() || oSurface.IsNull())
                    return;

                if (_szText.GetSize() > (size_t)(std::numeric_limits<INT>::max)())
                    throw Exception();
                auto _pNativeBrush = GetNativeBrush(_oBrush);
                if (!_pNativeBrush)
                    return;

                Gdiplus::FontFamily _FontFamily(_FontInfo.szFace);
                Gdiplus::Font _Font(&_FontFamily, _FontInfo.iSize, GetFontStyle(_FontInfo), Gdiplus::Unit::UnitPixel);

                int32_t _fStringFormatFlags = 0;
                if (!HasFlags(_fTextAlign, ContentAlignStyle::Wrap))
                    _fStringFormatFlags |= Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap;

                Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                _Format.SetFormatFlags(_fStringFormatFlags);
                _Format.SetAlignment(GetFontAlignment(_fTextAlign));
                _Format.SetLineAlignment(GetFontLineAlignment(_fTextAlign));

                if (HasFlags(_fTextAlign, ContentAlignStyle::EndEllipsis))
                    _Format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);

                oSurface.DrawString(_szText.GetConstString(), (INT)_szText.GetSize(), &_Font, _LayoutRect, &_Format, _pNativeBrush);
            }

            void __YYAPI GDIPlusDrawContext::MeasureString(uStringView _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign, Size* _pExtent)
            {
                _pExtent->Width = 0;
                _pExtent->Height = 0;

                if (_szText.GetSize() > (size_t)(std::numeric_limits<INT>::max)())
                    throw Exception();
                auto _Status = AutoGdiplusStartup::AutoStartup();
                if (_Status != Gdiplus::Status::Ok)
                    throw Exception();

                Gdiplus::FontFamily _FontFamily(_FontInfo.szFace);
                Gdiplus::Font _Font(&_FontFamily, _FontInfo.iSize, GetFontStyle(_FontInfo), Gdiplus::Unit::UnitPixel);

                int32_t _fStringFormatFlags = 0;
                if (!HasFlags(_fTextAlign, ContentAlignStyle::Wrap))
                    _fStringFormatFlags |= Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap;

                Gdiplus::StringFormat _Format(Gdiplus::StringFormat::GenericTypographic());
                _Format.SetFormatFlags(_fStringFormatFlags);
                _Format.SetAlignment(GetFontAlignment(_fTextAlign));
                _Format.SetLineAlignment(GetFontLineAlignment(_fTextAlign));

                if (HasFlags(_fTextAlign, ContentAlignStyle::EndEllipsis))
                    _Format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);


                // GdiplusRef<Gdiplus::Brush> TTT(Gdiplus::Color(0));
                _Status = Gdiplus::Status::GenericError;

                HDC _hDC = NULL;
                do
                {
                    GdiplusStatic<Gdiplus::Graphics> _oTmpSurface;
                    Gdiplus::Graphics* _pSurface = nullptr;
                    if (uThreadId == GetCurrentThreadId())
                    {
                        _pSurface = GetSurface();
                        if (!_pSurface)
                        {
                            break;
                        }
                    }
                    else
                    {
                        _hDC = ::GetDC(NULL);
                        _oTmpSurface.Reset(_hDC);
                        if (_oTmpSurface.IsNull())
                            break;
                        _pSurface = &_oTmpSurface;
                    }

                    Gdiplus::SizeF _Extent;

                    _Status = _pSurface->MeasureString(
                        _szText.GetConstString(),
                        (INT)_szText.GetSize(),
                        &_Font,
                        _LayoutSize,
                        &_Format,
                        &_Extent);
                    if (_Status != Gdiplus::Status::Ok)
                        break;

                    _pExtent->Width = _Extent.Width;
                    _pExtent->Height = _Extent.Height;
                } while (false);
                
                if (_hDC)
                    ::ReleaseDC(NULL, _hDC);

                if (_Status != Gdiplus::Status::Ok)
                    throw Exception(); 
            }

            RefPtr<IDWriteTextLayout>__YYAPI GDIPlusDrawContext::CreateTextLayout(uString _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign)
            {
                DWRITE_FONT_STYLE _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
                if (HasFlags(_FontInfo.fStyle, FontStyle::Italic))
                    _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;

                auto _pGdiPlusTextLayout = new GdiPlusTextLayout(
                    _FontInfo.szFace,
                    nullptr,
                    DWRITE_FONT_WEIGHT(_FontInfo.uWeight),
                    _eFontStyle,
                    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
                    (int)_FontInfo.iSize,
                    _szText,
                    _LayoutSize.Width,
                    _LayoutSize.Height);

                /*RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = _pDWriteFactory->CreateTextFormat(
                    _FontInfo.szFace,
                    nullptr,
                    DWRITE_FONT_WEIGHT(_FontInfo.uWeight),
                    _eFontStyle,
                    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
                    _FontInfo.iSize,
                    _szLocaleName,
                    _pTextFormat.ReleaseAndGetAddressOf());*/
                //if (FAILED(_hr))
                //    return _hr;

                // static_cast<IDWriteTextFormat*>(_pGdiPlusTextLayout)->SetFontFamilyName(_FontInfo.szFace);

                DWRITE_TEXT_ALIGNMENT _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
                if (HasFlags(_fTextAlign, ContentAlignStyle::Right))
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Center))
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
                }

                _pGdiPlusTextLayout->SetTextAlignment(_TextAlignment);

                DWRITE_PARAGRAPH_ALIGNMENT _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                if (HasFlags(_fTextAlign, ContentAlignStyle::Bottom))
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
                else if (HasFlags(_fTextAlign, ContentAlignStyle::Middle))
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
                }
                _pGdiPlusTextLayout->SetParagraphAlignment(_ParagraphAlignment);

                auto _WordWrapping = HasFlags(_fTextAlign, ContentAlignStyle::Wrap) ? DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP;
                _pGdiPlusTextLayout->SetWordWrapping(_WordWrapping);

                if (HasFlags(_fTextAlign, ContentAlignStyle::EndEllipsis))
                {
                    //RefPtr<IDWriteInlineObject> pDWriteInlineObject;
                    //auto _hr = _pDWriteFactory->CreateEllipsisTrimmingSign(_pTextFormat, pDWriteInlineObject.ReleaseAndGetAddressOf());
                    //if (SUCCEEDED(_hr))
                    //{
                    //    DWRITE_TRIMMING trim;
                    //    trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
                    //    trim.delimiter = 1;
                    //    trim.delimiterCount = 3;
                    //    _pTextFormat->SetTrimming(&trim, pDWriteInlineObject);
                    //}
                }

                /*RefPtr<IDWriteTextLayout> _pTextLayout;
                auto _hr = _pDWriteFactory->CreateTextLayout(
                    _szText.GetConstString(),
                    (UINT32)_szText.GetSize(),
                    _pTextFormat,
                    _Maxbound.Width,
                    _Maxbound.Height,
                    _pTextLayout.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;*/

                //if (HasFlags(_fTextStyle, FontStyle::Underline))
                //{
                //    _hr = _pTextLayout->SetUnderline(TRUE, DWRITE_TEXT_RANGE{ 0, (UINT32)_szText.GetSize() });
                //    if (FAILED(_hr))
                //        return _hr;
                //}

                //if (HasFlags((FontStyle)_fTextStyle, FontStyle::StrikeOut))
                //{
                //    _hr = _pTextLayout->SetStrikethrough(TRUE, DWRITE_TEXT_RANGE{ 0, (UINT32)_szText.GetSize() });
                //    if (FAILED(_hr))
                //        return _hr;
                //}

                //*_ppTextLayout = _pTextLayout.Detach();
                return RefPtr<IDWriteTextLayout>::FromPtr(_pGdiPlusTextLayout);
            }

            void __YYAPI GDIPlusDrawContext::DrawString2(const Point& _Origin, RefPtr<IDWriteTextLayout> _pTextLayout, Brush _oBrush)
            {
                GdiPlusTextLayoutDrawContext _oContext = { this, _oBrush };
                _pTextLayout->Draw(&_oContext, nullptr, _Origin.X, _Origin.Y);
            }

            Gdiplus::Graphics* __YYAPI GDIPlusDrawContext::GetSurface()
            {
                if (oSurface.IsNull())
                {
                    if (oSurfaceBitmap.IsNull())
                    {
                        oSurfaceBitmap.Reset((INT)oPixelSize.cx, (INT)oPixelSize.cy, (Gdiplus::PixelFormat)PixelFormat32bppARGB);
                        if (oSurfaceBitmap.IsNull())
                            return nullptr;
                    }

                    oSurface.Reset(&oSurfaceBitmap);
                    if (oSurface.IsNull())
                        return nullptr;

                    uThreadId = GetCurrentThreadId();
                }

                return &oSurface;
            }

            HRESULT __YYAPI GDIPlusDrawContext::UpdateTargetPixelSize(bool _bCopyToNewTarget)
            {
                if (oSurfaceBitmap.IsNull())
                {
                    return S_FALSE;
                }

                if (oSurfaceBitmap.GetWidth() >= oPixelSize.cx && oSurfaceBitmap.GetHeight() >= oPixelSize.cy)
                {
                    return S_FALSE;
                }
                
                oSurface.Reset();

#if 0
                // GDI系统保留有副本，暂时不进行复制。
                if (_bCopyToNewTarget)
                {
                    GdiplusStatic<Gdiplus::Bitmap> _oTmpBitmap(std::move(oSurfaceBitmap));

                    auto _pSurface = GetSurface();

                    Gdiplus::RectF PaintF(
                        Gdiplus::REAL(0),
                        Gdiplus::REAL(0),
                        Gdiplus::REAL(oSurfaceBitmap.GetWidth()),
                        Gdiplus::REAL(oSurfaceBitmap.GetHeight()));

                    _pSurface->DrawImage(
                        &_oTmpBitmap,
                        PaintF,
                        PaintF.X, PaintF.Y, PaintF.Width, PaintF.Height,
                        Gdiplus::UnitPixel);

                }
                else
#endif
                {
                    oSurfaceBitmap.Reset();
                }
                return S_OK;
            }

            RefPtr<GdiplusRef<Gdiplus::Brush>> __YYAPI GDIPlusDrawContext::GetNativeBrush(Brush _oBrush)
            {
                RefPtr<GdiplusRef<Gdiplus::Brush>> _oTmp;
                do
                {
                    auto _pCurrentMetadata = _oBrush.GetResourceMetadata();
                    if (!_pCurrentMetadata)
                    {
                        break;
                    }

                    if (SolidColorBrush::GetStaticResourceMetadata() == _pCurrentMetadata)
                    {
                        auto _pSolidBrush = new GdiplusRef<Gdiplus::SolidBrush>(Gdiplus::Color(_oBrush.TryCast<SolidColorBrush>().GetColor()));
                        _oTmp.Attach((GdiplusRef<Gdiplus::Brush>*)_pSolidBrush);
                        break;
                    }
                } while (false);

                if (_oTmp->GetLastStatus() != Gdiplus::Ok)
                    _oTmp = nullptr;
                return _oTmp;
            }

            RefPtr<GdiplusRef<Gdiplus::Pen>> __YYAPI GDIPlusDrawContext::GetNativePen(Pen _oPen)
            {
                RefPtr<GdiplusRef<Gdiplus::Pen>> _oTmp;
                do
                {
                    if (_oPen == nullptr)
                        break;

                    auto _oBrush = GetNativeBrush(_oPen.GetBrush());
                    if (_oBrush == nullptr)
                        break;

                    _oTmp.Attach(new GdiplusRef<Gdiplus::Pen>(_oBrush, _oPen.GetThickness()));
                } while (false);
                

                if (_oTmp && _oTmp->GetLastStatus() != Gdiplus::Ok)
                    _oTmp = nullptr;
                return _oTmp;
            }
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
