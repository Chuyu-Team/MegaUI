#include "pch.h"
#include "D2D1_1DrawContext.h"

#include <Base/Sync/Interlocked.h>
#include <Media/Graphics/D2D/DWriteHelper.h>

#pragma warning(disable : 28251)

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dcomp.lib")

#if __ENABLE_FLIP_SEQUENTIAL
constexpr UINT uSwapChainBufferCount = 2;
#else
constexpr UINT uSwapChainBufferCount = 1;
#endif

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
#define INVALID_FACTORY (ID2D1Factory1*)-1

            static ID2D1Factory1* s_pD2DFactory;

            ID2D1Factory1* __YYAPI D2D1_1DrawContext::GetD2D1Factory1()
            {
                if (auto _pTmp = s_pD2DFactory)
                {
                    return _pTmp == INVALID_FACTORY ? nullptr : _pTmp;
                }

                ID2D1Factory1* _pD2DFactory = nullptr;
#ifdef _DEBUG
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, D2D1_FACTORY_OPTIONS {D2D1_DEBUG_LEVEL_INFORMATION}, &_pD2DFactory);
#else
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
#endif
                if (FAILED(_hr) || _pD2DFactory == nullptr)
                {
                    YY::Base::Sync::CompareExchangePoint(&s_pD2DFactory, INVALID_FACTORY, (ID2D1Factory1*)nullptr);
                    return nullptr;
                }

                auto _pLast = YY::Base::Sync::CompareExchangePoint(&s_pD2DFactory, _pD2DFactory, (ID2D1Factory1*)nullptr);
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

            HRESULT __YYAPI D2D1_1DrawContext::CreateDrawTarget(HWND _hWnd, DrawContext** _ppDrawContext)
            {
                if (!_ppDrawContext)
                    return E_INVALIDARG;
                *_ppDrawContext = nullptr;

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return __HRESULT_FROM_WIN32(GetLastError());

                const D2D1_SIZE_U _oPixelSize = {_ClientRect.right - _ClientRect.left, _ClientRect.bottom - _ClientRect.top};

                auto _pD2D1_1DrawContext = new (std::nothrow) D2D1_1DrawContext(_hWnd, _oPixelSize);
                if (!_pD2D1_1DrawContext)
                    return E_OUTOFMEMORY;

                auto _hr = _pD2D1_1DrawContext->TryInitializeRenderTarget();
                if (FAILED(_hr))
                {
                    delete _pD2D1_1DrawContext;
                    return _hr;
                }

                *_ppDrawContext = _pD2D1_1DrawContext;
                return S_OK;
            }

            HRESULT __YYAPI D2D1_1DrawContext::TryInitializeRenderTarget()
            {
                if (pDeviceContext && pD2DTargetBitmap)
                    return S_OK;

                auto _pD2DFactory = GetD2D1Factory1();
                if (!_pD2DFactory)
                    return E_NOINTERFACE;

                // This array defines the set of DirectX hardware feature levels this app  supports.
                // The ordering is important and you should  preserve it.
                // Don't forget to declare your app's minimum required feature level in its
                // description.  All apps are assumed to support 9.1 unless otherwise stated.
                static const D3D_FEATURE_LEVEL _FeatureLevels[] =
                {
                    D3D_FEATURE_LEVEL_11_1,
                    D3D_FEATURE_LEVEL_11_0,
                    D3D_FEATURE_LEVEL_10_1,
                    D3D_FEATURE_LEVEL_10_0,
                    D3D_FEATURE_LEVEL_9_3,
                    D3D_FEATURE_LEVEL_9_2,
                    D3D_FEATURE_LEVEL_9_1
                };

                // Create the DX11 API device object, and get a corresponding context.
                RefPtr<ID3D11Device> _pD3DDevice;
                
#ifdef _DEBUG
                constexpr DWORD _fD3D11Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
                constexpr DWORD _fD3D11Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif


                auto _hr = D3D11CreateDevice(
                    nullptr, // specify null to use the default adapter
                    D3D_DRIVER_TYPE_HARDWARE,
                    0,
                    _fD3D11Flags,
                    _FeatureLevels,                   // list of feature levels this app can support
                    _countof(_FeatureLevels),         // number of possible feature levels
                    D3D11_SDK_VERSION,
                    _pD3DDevice.ReleaseAndGetAddressOf(),       // returns the Direct3D device created
                    &FeatureLevel,                              // returns feature level of device created
                    nullptr/*_pD3DDeviceContext.ReleaseAndGetAddressOf()*/ // returns the device immediate context
                    );
                if (FAILED(_hr))
                    return _hr;

                RefPtr<IDXGIDevice> _pDxgiDevice;
                // Obtain the underlying DXGI device of the Direct3D11 device.
                _hr = _pD3DDevice->QueryInterface(_pDxgiDevice.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                // Obtain the Direct2D device for 2-D rendering.
                _hr = _pD2DFactory->CreateDevice(_pDxgiDevice, pD2DDevice.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                // Get Direct2D device's corresponding device context object.
                _hr = pD2DDevice->CreateDeviceContext(
                    D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                    pDeviceContext.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                pDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);

                RefPtr<IDXGIAdapter> _pDxgiAdapter;
                _hr = _pDxgiDevice->GetAdapter(_pDxgiAdapter.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                RefPtr<IDXGIFactory2> _pDxgiFactory;
                _hr = _pDxgiAdapter->GetParent(IID_PPV_ARGS(_pDxgiFactory.ReleaseAndGetAddressOf()));
                if (FAILED(_hr))
                    return _hr;
#if 0
                DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
                // 设置刷新率60FPS
                fullscreenDesc.RefreshRate.Numerator = 60;
                fullscreenDesc.RefreshRate.Denominator = 1;
                // 扫描方案
                fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                // 缩放方案
                fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                // 全屏显示
#endif

                DXGI_SWAP_CHAIN_DESC1 _SwapChainDesc = {};
                _SwapChainDesc.Width = PixelSize.width;
                _SwapChainDesc.Height = PixelSize.height;
                _SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                _SwapChainDesc.Stereo = FALSE;
                _SwapChainDesc.SampleDesc.Count = 1;
                _SwapChainDesc.SampleDesc.Quality = 0;
                _SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                _SwapChainDesc.BufferCount = uSwapChainBufferCount;
#if __ENABLE_FLIP_SEQUENTIAL
                _SwapChainDesc.Scaling = __ENABLE_COMPOSITION_ENGINE ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE;
                _SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#else
                _SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                _SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
#endif

#if __ENABLE_COMPOSITION_ENGINE
                _SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
                _hr = _pDxgiFactory->CreateSwapChainForComposition(_pDxgiDevice, &_SwapChainDesc, nullptr, pSwapChain.ReleaseAndGetAddressOf());
#else
                _SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
                _hr = _pDxgiFactory->CreateSwapChainForHwnd(
                    _pD3DDevice,
                    hWnd,
                    &_SwapChainDesc,
                    nullptr,
                    nullptr,
                    pSwapChain.ReleaseAndGetAddressOf());
#endif

                if (FAILED(_hr))
                    return _hr;

                // Direct2D needs the dxgi version of the backbuffer surface pointer.
                RefPtr<IDXGISurface> _pDxgiBackBuffer;
                _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(_pDxgiBackBuffer.ReleaseAndGetAddressOf()));
                if (FAILED(_hr))
                    return _hr;

                D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);

                // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                    _pDxgiBackBuffer,
                    &_BitmapProperties,
                    pD2DTargetBitmap.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                pDeviceContext->SetTarget(pD2DTargetBitmap);
                
#if __ENABLE_COMPOSITION_ENGINE
                _hr = DCompositionCreateDevice(_pDxgiDevice, IID_PPV_ARGS(pDcompDevice.ReleaseAndGetAddressOf()));
                if (FAILED(_hr))
                    return _hr;

                pDcompDevice->CreateTargetForHwnd(hWnd, TRUE, pTarget.ReleaseAndGetAddressOf());
                RefPtr<IDCompositionVisual> _pVisual;
                pDcompDevice->CreateVisual(_pVisual.ReleaseAndGetAddressOf());
                _pVisual->SetContent(pSwapChain);
                pTarget->SetRoot(_pVisual);
                pDcompDevice->Commit();
#endif
                return S_OK;
            }

            HRESULT __YYAPI D2D1_1DrawContext::UpdateTargetPixelSize(ID2D1Bitmap** _ppBackupBitmap)
            {
                if (_ppBackupBitmap)
                    *_ppBackupBitmap = nullptr;
                
                if (!pDeviceContext)
                    return S_OK;

                auto _oPrevPixelSize = pDeviceContext->GetPixelSize();
                if (_oPrevPixelSize.width == PixelSize.width && _oPrevPixelSize.height == PixelSize.height)
                    return S_OK;
                bFirstPaint = true;

                RefPtr<ID2D1Bitmap> pD2DBitmapBackup;
                D2D1_POINT_2U _oDestPoint = {};
                D2D1_RECT_U _oSrcRect = {0, 0, min(_oPrevPixelSize.width, PixelSize.width), min(_oPrevPixelSize.height, PixelSize.height)};
                            
                do
                {
                    // 需要支持脏矩阵，临时保存原来的内容。
                    auto _oPixelFormat = pDeviceContext->GetPixelFormat();

                    if (_ppBackupBitmap)
                    {
                        pDeviceContext->CreateBitmap(
                            _oPrevPixelSize,
                            D2D1::BitmapProperties(_oPixelFormat),
                            pD2DBitmapBackup.ReleaseAndGetAddressOf());

                        if (__ENABLE_FLIP_SEQUENTIAL)
                        {
                            // RefPtr<IDXGISurface> pDxgiBackBuffer;
                            // auto _hr = pSwapChain->GetBuffer(uSwapChainBufferCount - 1, IID_PPV_ARGS(pDxgiBackBuffer.ReleaseAndGetAddressOf()));
                            // if (FAILED(_hr))
                            //     return _hr;

                            // D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                            //     D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                            //     D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);
                            // RefPtr<ID2D1Bitmap1> pD2DTargetBitmap;

                            //// Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                            //_hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                            //    pDxgiBackBuffer,
                            //    &_BitmapProperties,
                            //    pD2DTargetBitmap.ReleaseAndGetAddressOf());

                            pD2DBitmapBackup->CopyFromRenderTarget(&_oDestPoint, pDeviceContext, &_oSrcRect);
                        }
                        else
                        {
                            pD2DBitmapBackup->CopyFromBitmap(&_oDestPoint, pD2DTargetBitmap, &_oSrcRect);
                        }
                    }
                    
                    pDeviceContext->SetTarget(nullptr);
                    pD2DTargetBitmap = nullptr;

                    auto _hr = pSwapChain->ResizeBuffers(uSwapChainBufferCount, PixelSize.width, PixelSize.height, _oPixelFormat.format, 0);
                    if (FAILED(_hr))
                        return _hr;

                    // Direct2D needs the dxgi version of the backbuffer surface pointer.
                    RefPtr<IDXGISurface> pDxgiBackBuffer;
                    _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(pDxgiBackBuffer.ReleaseAndGetAddressOf()));
                    if (FAILED(_hr))
                        return _hr;

                    D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        _oPixelFormat, 0, 0);

                    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                    _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                        pDxgiBackBuffer,
                        &_BitmapProperties,
                        pD2DTargetBitmap.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    pDeviceContext->SetTarget(pD2DTargetBitmap);

                } while (false);

                if (_ppBackupBitmap)
                    *_ppBackupBitmap = pD2DBitmapBackup.Detach();
                return S_OK;
            }
            
            Size __YYAPI D2D1_1DrawContext::GetPixelSize()
            {
                return Size(PixelSize.width, PixelSize.height);
            }

            HRESULT __YYAPI D2D1_1DrawContext::SetPixelSize(const Size& _PixelSize)
            {
                const D2D1_SIZE_U _oPixelSize = {_PixelSize.Width, _PixelSize.Height};
                PixelSize = _oPixelSize;
                return S_OK;
            }

            HRESULT __YYAPI D2D1_1DrawContext::BeginDraw(const Rect* _pPaint)
            {
                oPaint.Reset();

                // 无效区域恰好为绘制区域大小其实跟没有无效区域是一样的
                if (_pPaint && _pPaint->Left == 0 && _pPaint->Top == 0 && _pPaint->Left == PixelSize.width && _pPaint->Bottom == PixelSize.height)
                    _pPaint = nullptr;

                
                RefPtr<ID2D1Bitmap> _pD2DBitmapBackup;
                auto _hr = UpdateTargetPixelSize(_pPaint && bFirstPaint == false ? _pD2DBitmapBackup.ReleaseAndGetAddressOf() : nullptr);
                if (FAILED(_hr))
                    return _hr;

                _hr = TryInitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;
                
                pDeviceContext->BeginDraw();

                if (_pD2DBitmapBackup)
                {
                    auto _oPixelSize = _pD2DBitmapBackup->GetPixelSize();
                    D2D1_RECT_F _oBufferBitmapRectangle = {0, 0, _oPixelSize.width, _oPixelSize.height};
                    pDeviceContext->DrawBitmap(_pD2DBitmapBackup, _oBufferBitmapRectangle, 1.0, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, _oBufferBitmapRectangle);
                    _pD2DBitmapBackup = nullptr;
                }

                if (_pPaint)
                {
                    oPaint = RECT{(LONG)_pPaint->Left, (LONG)_pPaint->Top, (LONG)_pPaint->Right, (LONG)_pPaint->Bottom};
                    PushAxisAlignedClip(*_pPaint);
                }


                pDeviceContext->Clear();
                return S_OK;
            }

            HRESULT __YYAPI D2D1_1DrawContext::EndDraw()
            {
                if (!pDeviceContext)
                    return E_UNEXPECTED;

                DXGI_PRESENT_PARAMETERS _oPresentParameters = {};
                if (oPaint.HasValue())
                {
                    if (bFirstPaint == false)
                    {
                        _oPresentParameters.DirtyRectsCount = 1;
                        _oPresentParameters.pDirtyRects = oPaint.GetValuePtr();
                    }
                    PopAxisAlignedClip();
                }

                HRESULT _hr = pDeviceContext->EndDraw();

                // 需要重新呈现
                if (FAILED(_hr))
                {
                    if (_hr == D2DERR_RECREATE_TARGET)
                    {
                        pDeviceContext = nullptr;
                        pD2DDevice = nullptr;
                        pD2DTargetBitmap = nullptr;
                        pSwapChain = nullptr;
#if __ENABLE_COMPOSITION_ENGINE
                        pTarget = nullptr;
                        pDcompDevice = nullptr;
#endif
                    }
                    else
                    {
                        pDeviceContext->Flush();
                    }

                    return _hr;
                }

                _hr = pSwapChain->Present1(0, 0, &_oPresentParameters);
                if (SUCCEEDED(_hr))
                {
                    bFirstPaint = false;
                }
                return _hr;
            }

            void __YYAPI D2D1_1DrawContext::PushAxisAlignedClip(const Rect& _ClipRect)
            {
                pDeviceContext->PushAxisAlignedClip(_ClipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            }

            void __YYAPI D2D1_1DrawContext::PopAxisAlignedClip()
            {
                pDeviceContext->PopAxisAlignedClip();
            }

            void __YYAPI D2D1_1DrawContext::DrawLine(Point _oPoint0, Point _oPoint1, Pen _oPen)
            {
                // ToDo
            }

            void __YYAPI D2D1_1DrawContext::DrawRectangle(const Rect& _oRect, Pen _oPen, Brush _oBrush)
            {
                if (!pDeviceContext)
                    return;

                D2D1_RECT_F _oD2DRect = _oRect;

                // 没有刷子则表示无需填充内部
                if (_oBrush != nullptr)
                {
                    auto _pD2dBursh = GetNativeBrush(_oBrush);
                    if (_pD2dBursh != nullptr)
                    {
                        pDeviceContext->FillRectangle(_oD2DRect, _pD2dBursh);
                    }
                }

                // 没有 Pen 或者 粗细小于 0则表示无需绘制边框。
                if (_oPen != nullptr && _oPen.GetThickness() <= 0)
                {
                    auto _pD2dBursh = GetNativeBrush(_oPen.GetBrush());
                    if (_pD2dBursh != nullptr)
                    {
                        pDeviceContext->DrawRectangle(_oD2DRect, _pD2dBursh, _oPen.GetThickness());
                    }
                }
            }

            void __YYAPI D2D1_1DrawContext::DrawString(uString _szText, const Font& _FontInfo, Brush _oBrush, const Rect& _LayoutRect, ContentAlignStyle _fTextAlign)
            {
                if (!pDeviceContext.Get())
                    return;
                
                auto _oNativeBrush = GetNativeBrush(_oBrush);
                if (!_oNativeBrush.Get())
                    return;

                DWrite::DrawString(pDeviceContext.Get(), _szText, _FontInfo, _oNativeBrush.Get(), _LayoutRect, _fTextAlign);
            }

            void __YYAPI D2D1_1DrawContext::MeasureString(uStringView _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign, Size* _pExtent)
            {
                DWrite::MeasureString(_szText, _FontInfo, _LayoutSize, _fTextAlign, _pExtent);
            }
            
            RefPtr<ID2D1Brush> __YYAPI D2D1_1DrawContext::GetNativeBrush(Brush _oBrush)
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
                        pDeviceContext->CreateSolidColorBrush(_oBrush.TryCast<SolidColorBrush>().GetColor(), &_pSolidColorBrush);
                        _pBrush.Attach(_pSolidColorBrush);
                        break;
                    }
                } while (false);

                return _pBrush;
            }

        } // namespace Graphics
    }     // namespace Media
} // namespace YY
