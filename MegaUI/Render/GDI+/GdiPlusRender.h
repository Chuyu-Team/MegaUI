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
            DynamicArray<Gdiplus::Rect, false, false> vecClip;
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

            static Gdiplus::Rect __MEGA_UI_API ToGdiPlusRect(const Rect& _Rect)
            {
                Gdiplus::Rect _GdiPlusRect(_Rect.left, _Rect.top, _Rect.right - _Rect.left, _Rect.bottom - _Rect.top);
                return _GdiPlusRect;
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
                    return __HRESULT_FROM_WIN32(GetLastError());
                
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
                    Gdiplus::RectF PaintF(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
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
                Gdiplus::Rect _NewClipRect;

                if (auto _uSize = vecClip.GetSize())
                {
                    _NewClipRect = ToGdiPlusRect(_ClipRect);
                    _NewClipRect.Intersect(vecClip[_uSize - 1]);
                }
                else
                {
                    _NewClipRect = ToGdiPlusRect(_ClipRect & ps.rcPaint);
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
                    ToGdiPlusRect(_Rect));
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
        };
    }
} // namespace YY

#pragma pack(pop)
