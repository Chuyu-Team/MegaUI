#include "pch.h"
#include "D2D1_0DrawContext.h"

#include <Media/Graphics/D2D/DWriteHelper.h>

#pragma warning(disable : 28251)

#pragma comment(lib, "d2d1.lib")

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
#define INVALID_FACTORY (ID2D1Factory*)-1

            static ID2D1Factory* s_pD2DFactory;

            ID2D1Factory* D2D1_0DrawContext::GetD2D1Factory()
            {
                if (auto _pTmp = s_pD2DFactory)
                {
                    return _pTmp == INVALID_FACTORY ? nullptr : _pTmp;
                }

                ID2D1Factory* _pD2DFactory = nullptr;
#ifdef _DEBUG
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, D2D1_FACTORY_OPTIONS {D2D1_DEBUG_LEVEL_INFORMATION}, &_pD2DFactory);
#else
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
#endif
                if (FAILED(_hr) || _pD2DFactory == nullptr)
                {
                    YY::Base::Sync::CompareExchangePoint(&s_pD2DFactory, INVALID_FACTORY, (ID2D1Factory*)nullptr);
                    return nullptr;
                }

                auto _pLast = YY::Base::Sync::CompareExchangePoint(&s_pD2DFactory, _pD2DFactory, (ID2D1Factory*)nullptr);
                if (_pLast != nullptr)
                {
                    // 其他线程已经初始化，保存最后状态，这有助于释放时。
                    _pD2DFactory->Release();
                    return _pLast == INVALID_FACTORY ? nullptr : _pLast;
                }

                // 注册释放
                atexit(
                    []()
                    {
                        auto _pLast = YY::Base::Sync::ExchangePoint(&s_pD2DFactory, INVALID_FACTORY);
                        if (_pLast && _pLast != INVALID_FACTORY)
                            _pLast->Release();
                    });

                return _pD2DFactory;
            }

            HRESULT D2D1_0DrawContext::CreateDrawTarget(HWND _hWnd, DrawContext** _ppDrawContext)
            {
                if (!_ppDrawContext)
                    return E_INVALIDARG;
                *_ppDrawContext = nullptr;

                const auto _pD2D1Factory = GetD2D1Factory();
                if (!_pD2D1Factory)
                {
                    return E_NOINTERFACE;
                }

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return __HRESULT_FROM_WIN32(GetLastError());

                std::unique_ptr<D2D1_0DrawContext> _pDrawContext(new (std::nothrow) D2D1_0DrawContext());
                if (!_pDrawContext.get())
                    return E_OUTOFMEMORY;

                _pDrawContext->hWnd = _hWnd;
                _pDrawContext->PixelSize.width = _ClientRect.right - _ClientRect.left;
                _pDrawContext->PixelSize.height = _ClientRect.bottom - _ClientRect.top;

                auto _hr = _pDrawContext->TryInitializeRenderTarget();
                if (FAILED(_hr))
                {
                    return _hr;
                }

                *_ppDrawContext = _pDrawContext.release();
                return S_OK;
            }

            Size D2D1_0DrawContext::GetPixelSize()
            {
                return Size(PixelSize.width, PixelSize.height);
            }

            HRESULT D2D1_0DrawContext::SetPixelSize(const Size& _PixelSize)
            {
                const D2D1_SIZE_U _oPixelSize = {_PixelSize.Width, _PixelSize.Height};
                if (_oPixelSize.width == PixelSize.width && _oPixelSize.height == PixelSize.height)
                    return S_OK;

                PixelSize = _oPixelSize;
                return S_OK;
            }

            HRESULT D2D1_0DrawContext::BeginDraw(_In_opt_ const Rect* _pPaint)
            {
                // 无效区域跟整个绘制大小一样就不用无效区域了。
                if (_pPaint && _pPaint->Left == 0 && _pPaint->Top == 0 && _pPaint->Left == PixelSize.width && _pPaint->Bottom == PixelSize.height)
                    _pPaint = nullptr;

                RefPtr<ID2D1Bitmap> _pBackupBitmap;
                if (_pPaint)
                {
                    auto _hr = UpdateTargetPixelSize(_pPaint ? _pBackupBitmap.ReleaseAndGetAddressOf() : nullptr);
                    if (FAILED(_hr))
                        return _hr;
                }

                auto _hr = TryInitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;

                pRenderTarget->BeginDraw();

                if (_pBackupBitmap)
                {
                    auto _oPixelSize = _pBackupBitmap->GetPixelSize();
                    D2D1_RECT_F _oBufferBitmapRectangle = {0, 0, _oPixelSize.width, _oPixelSize.height};
                    pRenderTarget->DrawBitmap(_pBackupBitmap, _oBufferBitmapRectangle, 1.0, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, _oBufferBitmapRectangle);
                    _pBackupBitmap = nullptr;
                }

                if (_pPaint)
                {
                    oPaint = *_pPaint;
                    PushAxisAlignedClip(*_pPaint);
                }
                else
                {
                    oPaint.Reset();
                }

                pRenderTarget->Clear();
                return S_OK;
            }

            HRESULT D2D1_0DrawContext::EndDraw()
            {
                if (!pRenderTarget)
                    return E_UNEXPECTED;

                if (oPaint.HasValue())
                {
                    PopAxisAlignedClip();
                    oPaint.Reset();
                }

                auto _hr = pRenderTarget->EndDraw();

                if (FAILED(_hr))
                {
                    // 需要重新呈现
                    if (_hr == D2DERR_RECREATE_TARGET)
                    {
                        _hr = S_OK;
                        pRenderTarget = nullptr;
                    }
                }

                return _hr;
            }

            void D2D1_0DrawContext::PushAxisAlignedClip(const Rect& _ClipRect)
            {
                if (!pRenderTarget)
                    return;

                pRenderTarget->PushAxisAlignedClip(_ClipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            }

            void D2D1_0DrawContext::PopAxisAlignedClip()
            {
                if (!pRenderTarget)
                    return;

                pRenderTarget->PopAxisAlignedClip();
            }

            void D2D1_0DrawContext::DrawLine(Point _oPoint0, Point _oPoint1, Pen _oPen)
            {
                if (!pRenderTarget)
                    return;

                // ToDo
            }

            void D2D1_0DrawContext::DrawRectangle(const Rect& _oRect, Pen _oPen, Brush _oBrush)
            {
                if (!pRenderTarget)
                    return;

                D2D1_RECT_F _oD2DRect = _oRect;

                // 没有刷子则表示无需填充内部
                if (_oBrush != nullptr)
                {
                    auto _pD2dBursh = GetNativeBrush(_oBrush);
                    if (_pD2dBursh != nullptr)
                    {
                        pRenderTarget->FillRectangle(_oD2DRect, _pD2dBursh);
                    }
                }
                
                // 没有 Pen 或者 粗细小于 0则表示无需绘制边框。
                if (_oPen != nullptr && _oPen.GetThickness() <= 0)
                {
                    auto _pD2dBursh = GetNativeBrush(_oPen.GetBrush());
                    if (_pD2dBursh != nullptr)
                    {
                        pRenderTarget->DrawRectangle(_oD2DRect, _pD2dBursh, _oPen.GetThickness());
                    }
                }
            }

            void D2D1_0DrawContext::DrawString(uString _szText, const Font& _FontInfo, Brush _oBrush, const Rect& _LayoutRect, ContentAlignStyle _fTextAlign)
            {
                if (!pRenderTarget)
                    return;

                auto _oNativeBrush = GetNativeBrush(_oBrush);
                if (!_oNativeBrush)
                    return;

                DWrite::DrawString(pRenderTarget, _szText, _FontInfo, _oNativeBrush, _LayoutRect, _fTextAlign);
            }

            void D2D1_0DrawContext::MeasureString(uStringView _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign, Size* _pExtent)
            {
                DWrite::MeasureString(_szText, _FontInfo, _LayoutSize, _fTextAlign, _pExtent);
            }

            HRESULT D2D1_0DrawContext::TryInitializeRenderTarget()
            {
                if (pRenderTarget != nullptr)
                    return S_OK;

                const auto _pD2D1Factory = GetD2D1Factory();
                if (!_pD2D1Factory)
                {
                    return E_NOINTERFACE;
                }

                auto _hr = _pD2D1Factory->CreateHwndRenderTarget(
                    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.0f, 96.0f),
                    D2D1::HwndRenderTargetProperties(hWnd, PixelSize),
                    pRenderTarget.ReleaseAndGetAddressOf());

                if (FAILED(_hr))
                    return _hr;

                if (!pRenderTarget)
                {
                    return E_UNEXPECTED;
                }

                return S_OK;
            }

            HRESULT __YYAPI D2D1_0DrawContext::UpdateTargetPixelSize(ID2D1Bitmap** _ppBackupBitmap)
            {
                if (_ppBackupBitmap)
                    *_ppBackupBitmap = nullptr;

                if (pRenderTarget)
                {
                    auto _oPrevPixelSize = pRenderTarget->GetPixelSize();
                    if (_oPrevPixelSize.width != PixelSize.width || _oPrevPixelSize.height != PixelSize.height)
                    {
                        RefPtr<ID2D1Bitmap> _pBackupBitmap;
                        HRESULT _hr;

                        if (_ppBackupBitmap)
                        {
                            D2D1_SIZE_U _oTmpBufferPixelSize = {min(_oPrevPixelSize.width, PixelSize.width), min(_oPrevPixelSize.height, PixelSize.height)};
                            _hr = pRenderTarget->CreateBitmap(
                                _oPrevPixelSize,
                                D2D1::BitmapProperties(pRenderTarget->GetPixelFormat()),
                                _pBackupBitmap.ReleaseAndGetAddressOf());
                            if (FAILED(_hr))
                            {
                                return _hr;
                            }

                            D2D1_RECT_U _oSrcRect = {0, 0, _oPrevPixelSize.width, _oPrevPixelSize.height};
                            _hr = _pBackupBitmap->CopyFromRenderTarget(nullptr, pRenderTarget, &_oSrcRect);
                            if (FAILED(_hr))
                            {
                                return _hr;
                            }
                        }

                        _hr = pRenderTarget->Resize(PixelSize);
                        if (FAILED(_hr))
                        {
                            return _hr;
                        }

                        if (_ppBackupBitmap)
                            *_ppBackupBitmap = _pBackupBitmap.Detach();
                    }
                }

                return S_OK;
            }
            
            RefPtr<ID2D1Brush> __YYAPI D2D1_0DrawContext::GetNativeBrush(Brush _oBrush)
            {
                RefPtr<ID2D1Brush> _pBrush;
                do
                {
                    auto _pCurrentMetadata = _oBrush.GetResourceMetadata();
                    if (!_pCurrentMetadata)
                    {
                        break;
                    }

                    if (SolidColorBrush::GetStaticResourceMetadata() == _pCurrentMetadata)
                    {
                        ID2D1SolidColorBrush* _pSolidColorBrush = nullptr;
                        pRenderTarget->CreateSolidColorBrush(_oBrush.TryCast<SolidColorBrush>().GetColor(), &_pSolidColorBrush);
                        _pBrush.Attach(_pSolidColorBrush);
                        break;
                    }
                } while (false);

                return _pBrush;
            }
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
