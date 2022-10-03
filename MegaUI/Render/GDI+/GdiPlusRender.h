#pragma once
#include <Windows.h>
#include <GdiPlus.h>

#pragma comment(lib, "gdiplus.lib")

#include "../../base/MegaUITypeInt.h"
#include "../Render.h"
#include "../../base/alloc.h"
#include "../../base/DynamicArray.h"
#include "Brush/D2D1SolidColorBrushForGdiPlus.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // 此版本适用于Windows XP等没有 D2D的Windows系统
        class GdiPlusRender : public Render
        {
        private:
            HWND hWnd;
            D2D1_SIZE_U PixelSize;
            Gdiplus::GdiplusStartupInput GdiplusStartupInput;
            ULONG_PTR GdiplusToken;
            HDC hDC;
            PAINTSTRUCT ps;
            Gdiplus::Bitmap* pSurfaceBitmap;
            Gdiplus::Graphics* pSurface;
            DynamicArray<Gdiplus::RectF, false, false> vecClip;
        public:
            GdiPlusRender(HWND _hWnd, const D2D1_SIZE_U& _PixelSize)
                : hWnd(_hWnd)
                , PixelSize(_PixelSize)
                , GdiplusStartupInput {}
                , GdiplusToken(0)
                , hDC(nullptr)
                , ps {}
                , pSurfaceBitmap(nullptr)
                , pSurface(nullptr)
            {
            }

            ~GdiPlusRender()
            {
                if (pSurface)
                    delete pSurface;

                if (pSurfaceBitmap)
                    delete pSurfaceBitmap;

                if (GdiplusToken)
                {
                    Gdiplus::GdiplusShutdown(GdiplusToken); 
                }
            }

            HRESULT __MEGA_UI_API Init()
            {
                return Gdiplus::GdiplusStartup(&GdiplusToken, &GdiplusStartupInput, NULL) == Gdiplus::Status::Ok ? S_OK : E_FAIL;
            }

            static HRESULT __MEGA_UI_API CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender)
            {
                if (_ppRender == nullptr)
                    return E_INVALIDARG;

                *_ppRender = nullptr;

                if (_hWnd == NULL)
                    return E_INVALIDARG;

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return E_UNEXPECTED;
                
                D2D1_SIZE_U _PixelSize = D2D1::SizeU(
                    static_cast<UINT>(_ClientRect.right - _ClientRect.left),
                    static_cast<UINT>(_ClientRect.bottom - _ClientRect.top));

                auto _pGdiPlusRender = HNew<GdiPlusRender>(_hWnd, _PixelSize);
                if (!_pGdiPlusRender)
                    return E_OUTOFMEMORY;

                auto _hr = _pGdiPlusRender->Init();
                if (FAILED(_hr))
                {
                    HFree(_pGdiPlusRender);
                    return _hr;
                }
                
                *_ppRender = _pGdiPlusRender;
                return S_OK;
            }

            virtual HRESULT __MEGA_UI_API BeginDraw(Rect* _pNeedPaintRect) override
            {
                vecClip.Clear();

                hDC = BeginPaint(hWnd, &ps);

                if (_pNeedPaintRect)
                {
                    *_pNeedPaintRect = ps.rcPaint;
                }

                if (pSurfaceBitmap)
                {
                    if (pSurfaceBitmap->GetWidth() < PixelSize.width || pSurfaceBitmap->GetHeight() < PixelSize.height)
                    {
                        delete pSurface;
                        pSurface = nullptr;
                        delete pSurfaceBitmap;
                        pSurfaceBitmap = nullptr;
                    }
                }

                if (!pSurfaceBitmap)
                {
                    pSurfaceBitmap = new Gdiplus::Bitmap(PixelSize.width, PixelSize.height);
                    if (!pSurfaceBitmap)
                        return E_OUTOFMEMORY;
                }

                if (!pSurface)
                {
                    pSurface = new Gdiplus::Graphics(pSurfaceBitmap);
                    if (!pSurface)
                        return E_OUTOFMEMORY;
                }
                else
                {
                    pSurface->Clear(Gdiplus::Color());
                    pSurface->ResetClip();
                }
                return S_OK;
            }

            virtual HRESULT __MEGA_UI_API EndDraw() override
            {
                if (!hDC)
                    return E_FAIL;
                {
                    Gdiplus::RectF PaintF(
                        Gdiplus::REAL(ps.rcPaint.left),
                        Gdiplus::REAL(ps.rcPaint.top),
                        Gdiplus::REAL(ps.rcPaint.right - ps.rcPaint.left),
                        Gdiplus::REAL(ps.rcPaint.bottom - ps.rcPaint.top));
                    Gdiplus::Graphics DCSurface(hDC);
                    DCSurface.DrawImage(
                        pSurfaceBitmap,
                        PaintF,
                        PaintF.X, PaintF.Y, PaintF.Width, PaintF.Height,
                        Gdiplus::UnitPixel);
                }

                EndPaint(hWnd, &ps);
                return S_OK;
            }

            virtual
            void
            __MEGA_UI_API
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) override
            {
                Gdiplus::RectF _NewClipRect;

                if (auto _uSize = vecClip.GetSize())
                {
                    _NewClipRect = _ClipRect;
                    _NewClipRect.Intersect(vecClip[_uSize - 1]);
                }
                else
                {
                    _NewClipRect = _ClipRect & ps.rcPaint;
                }

                pSurface->SetClip(_NewClipRect);
                vecClip.Add(_NewClipRect);
            }

            virtual
            void
            __MEGA_UI_API
            PopAxisAlignedClip() override
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
                    pSurface->SetClip(vecClip[_uSize - 1]);
                }
                else
                {
                    pSurface->ResetClip();
                }
            }
            
            virtual void __MEGA_UI_API FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) override
            {
                pSurface->FillRectangle(
                    ((D2D1SolidColorBrushForGdiPlus*)_pBrush)->GetBrush(),
                    _Rect);
            }

            virtual HRESULT __MEGA_UI_API CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) override
            {
                if (!_ppSolidColorBrush)
                    return E_INVALIDARG;
                *_ppSolidColorBrush = nullptr;

                auto _pSolidColorBrush = HNew<D2D1SolidColorBrushForGdiPlus>(_Color);
                if (!_pSolidColorBrush)
                    return E_OUTOFMEMORY;

                *_ppSolidColorBrush = _pSolidColorBrush;
                return S_OK;
            }

            virtual HRESULT __MEGA_UI_API SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize)
            {
                PixelSize = _PixelSize;
                return S_OK;
            }

            virtual D2D1_SIZE_U __MEGA_UI_API GetPixelSize()
            {
                return PixelSize;
            }
            
            virtual
            HRESULT
            __MEGA_UI_API CreateTextFormat(
                _In_z_ uchar_t const* _szFontFamilyName,
                _In_opt_ IDWriteFontCollection* _pFontCollection,
                _In_ const Font& _FontInfo,
                _In_z_ uchar_t const* _szLocaleName,
                _COM_Outptr_ IDWriteTextFormat** _ppTextFormat
                ) override
            {
                if (_ppTextFormat)
                    *_ppTextFormat = nullptr;
                return E_NOTIMPL;
            }

            virtual
            HRESULT
            __MEGA_UI_API CreateTextLayout(
                _In_ uStringView _szText,
                _In_ IDWriteTextFormat* _pTextFormat,
                _In_ Size _MaxBound,
                _COM_Outptr_ IDWriteTextLayout** _ppTextLayout
                ) override
            {
                if (_ppTextLayout)
                    *_ppTextLayout = nullptr;
                return E_NOTIMPL;
            }

            virtual
            void
            __MEGA_UI_API DrawTextLayout(
                _In_ Point _Origin,
                _In_ IDWriteTextLayout* _pTextLayout,
                _In_ ID2D1Brush* _pDefaultFillBrush,
                _In_ D2D1_DRAW_TEXT_OPTIONS _eOptions
                ) override
            {
                return;
            }

            static float __MEGA_UI_API GetFontSize(_In_ const Font& _FontInfo)
            {
                return _FontInfo.iSize * 72.0f / 96.0f;
            }

            static int32_t __MEGA_UI_API GetFontStyle(_In_ const Font& _FontInfo)
            {
                int32_t _fFontStyle = 0;

                if (_FontInfo.uWeight <= FontWeight::Medium)
                {
                    // Regular
                    if (_FontInfo.fStyle & FontStyle::Italic)
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
                    if (_FontInfo.fStyle & FontStyle::Italic)
                    {
                        _fFontStyle = Gdiplus::FontStyleBoldItalic;
                    }
                    else
                    {
                        _fFontStyle = Gdiplus::FontStyleBold;
                    }
                }

                if (_FontInfo.fStyle & FontStyle::Underline)
                {
                    _fFontStyle |= Gdiplus::FontStyleUnderline;
                }

                if (_FontInfo.fStyle & FontStyle::StrikeOut)
                {
                    _fFontStyle |= Gdiplus::FontStyleStrikeout;
                }

                return _fFontStyle;
            }

            // 
            static Gdiplus::StringAlignment __MEGA_UI_API GetFontAlignment(_In_ int32_t _fTextAlign)
            {
                if (_fTextAlign & ContentAlign::Right)
                {
                    return Gdiplus::StringAlignmentFar;
                }
                else if (_fTextAlign & ContentAlign::Center)
                {
                    return Gdiplus::StringAlignmentCenter;
                }
                else
                {
                    return Gdiplus::StringAlignmentNear;
                }
            }

            static Gdiplus::StringAlignment __MEGA_UI_API GetFontLineAlignment(_In_ int32_t _fTextAlign)
            {
                if (_fTextAlign & ContentAlign::Bottom)
                {
                    return Gdiplus::StringAlignmentFar;
                }
                else if (_fTextAlign & ContentAlign::Middle)
                {
                    return Gdiplus::StringAlignmentCenter;
                }
                else
                {
                    return Gdiplus::StringAlignmentNear;
                }
            }

            virtual
            void
            __MEGA_UI_API
            DrawString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Rect& _LayoutRect,
                _In_ int32_t _fTextAlign
                ) override
            {
                if (_szText.GetSize() == 0 || _LayoutRect.IsEmpty())
                    return;

                if (_szText.GetSize() > int32_max)
                    throw Exception();

                Gdiplus::FontFamily _FontFamily(_FontInfo.szFace);
                Gdiplus::Font _Font(&_FontFamily, _FontInfo.iSize, GetFontStyle(_FontInfo), Gdiplus::Unit::UnitPixel);

                int32_t _fStringFormatFlags = 0;
                if ((_fTextAlign & ContentAlign::Wrap) == 0)
                    _fStringFormatFlags |= Gdiplus::StringFormatFlags::StringFormatFlagsNoWrap;

                Gdiplus::StringFormat _Format(_fStringFormatFlags);
                _Format.SetAlignment(GetFontAlignment(_fTextAlign));
                _Format.SetLineAlignment(GetFontLineAlignment(_fTextAlign));

                if (_fTextAlign & ContentAlign::EndEllipsis)
                    _Format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);

                Gdiplus::SolidBrush _Brush(Gdiplus::Color(_FontInfo.Color.Alpha, _FontInfo.Color.Red, _FontInfo.Color.Green, _FontInfo.Color.Blue));
                


                pSurface->DrawString(_szText.GetConstString(), (INT)_szText.GetSize(), &_Font, _LayoutRect, &_Format, &_Brush);
            }
        };
    }
} // namespace YY

#pragma pack(pop)
