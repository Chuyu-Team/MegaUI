#include "pch.h"
#include "D2D1_1DrawContext.h"

#include <Base/Sync/Interlocked.h>
#include <Media/Graphics/D2D/DWriteHelper.h>
#include <Base/Memory/UniquePtr.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dcomp.lib")

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
#define INVALID_FACTORY (ID2D1Factory1*)-1

            static ID2D1Factory1* s_pD2DFactory;

            ID2D1Factory1* __YYAPI D2D1_1DrawContext::GetD2D1Factory()
            {
                if (auto _pTmp = s_pD2DFactory)
                {
                    return _pTmp == INVALID_FACTORY ? nullptr : _pTmp;
                }

                ID2D1Factory1* _pD2DFactory = nullptr;
#ifdef _DEBUG
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, D2D1_FACTORY_OPTIONS {D2D1_DEBUG_LEVEL_INFORMATION}, &_pD2DFactory);
#else
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &_pD2DFactory);
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
         
#define INVALID_DIRECT_DEVICE_CACHE_FACTORY (D2D1_1DrawContext::DirectDeviceCache*)-1
            struct D3D11Device1Cache
            {
                SRWLOCK oLock;
                D2D1_1DrawContext::DirectDeviceCache* pDirectDeviceCache;
                bool bRegistExit;
            };

            static D3D11Device1Cache s_oDirectDeviceCache;

            RefPtr<D2D1_1DrawContext::DirectDeviceCache> __YYAPI D2D1_1DrawContext::GetDirectDeviceCache()
            {
                RefPtr<D2D1_1DrawContext::DirectDeviceCache> _pTmp;

                bool _bInit = false;
                AcquireSRWLockShared(&s_oDirectDeviceCache.oLock);
                if (auto p = s_oDirectDeviceCache.pDirectDeviceCache)
                {
                    if (p != INVALID_DIRECT_DEVICE_CACHE_FACTORY)
                        _pTmp = p;

                    _bInit = true;
                }
                ReleaseSRWLockShared(&s_oDirectDeviceCache.oLock);

                if (_bInit)
                {
                    return _pTmp;
                }

                AcquireSRWLockExclusive(&s_oDirectDeviceCache.oLock);

                do
                {
                    auto _pLastCache = Sync::CompareExchangePoint(&s_oDirectDeviceCache.pDirectDeviceCache, INVALID_DIRECT_DEVICE_CACHE_FACTORY, nullptr);
                    if (_pLastCache)
                    {
                        if (_pLastCache != INVALID_DIRECT_DEVICE_CACHE_FACTORY)
                        {
                            _pTmp = _pLastCache;
                        }
                        break;
                    }

                    auto _pD2DFactory = GetD2D1Factory();
                    if (!_pD2DFactory)
                        break;

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
    #ifdef _DEBUG
                    constexpr const DWORD kD3D10FlagsArray[] = {D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, D3D11_CREATE_DEVICE_BGRA_SUPPORT};
    #else
                    constexpr DWORD _fD3D11Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    #endif
                    static constexpr D3D_DRIVER_TYPE _arrDriverTypes[] =
                    {
                        D3D_DRIVER_TYPE_HARDWARE,
                        D3D_DRIVER_TYPE_WARP,
                    };

                    HRESULT _hr = E_FAIL;
                    UniquePtr<D2D1_1DrawContext::DirectDeviceCache> _pTmpCache(new D2D1_1DrawContext::DirectDeviceCache());

                    for (auto _eDriverType : _arrDriverTypes)
                    {
#ifdef _DEBUG
                        for (const auto _fD3D11Flags : kD3D10FlagsArray)
#endif
                        {
                            _hr = D3D11CreateDevice(
                                nullptr, // specify null to use the default adapter
                                _eDriverType,
                                0,
                                _fD3D11Flags,
                                _FeatureLevels,           // list of feature levels this app can support
                                _countof(_FeatureLevels), // number of possible feature levels
                                D3D11_SDK_VERSION,
                                _pTmpCache->pD3DDevice.ReleaseAndGetAddressOf(),       // returns the Direct3D device created
                                &_pTmpCache->FeatureLevel,                             // returns feature level of device created
                                _pTmpCache->pD3DDeviceContext.ReleaseAndGetAddressOf() // returns the device immediate context
                            );
                            if (SUCCEEDED(_hr))
                                break;
                        }
                    }

                    if (FAILED(_hr))
                        break;

                    RefPtr<IDXGIDevice> _pDxgiDevice;
                    // Obtain the underlying DXGI device of the Direct3D11 device.
                    _hr = _pTmpCache->pD3DDevice->QueryInterface(_pDxgiDevice.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        break;

                    // Obtain the Direct2D device for 2-D rendering.
                    _hr = _pD2DFactory->CreateDevice(_pDxgiDevice, _pTmpCache->pD2DDevice.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        break;
                     
                    // 非关键错误，Windows 8或者更高平台支持
                    DCompositionCreateDevice(_pDxgiDevice, IID_PPV_ARGS(_pTmpCache->pDcompDevice.ReleaseAndGetAddressOf()));

                    _pTmp.Attach(_pTmpCache.Detach());
                    s_oDirectDeviceCache.pDirectDeviceCache = _pTmp.Clone();

                    if (!s_oDirectDeviceCache.bRegistExit)
                    {
                        s_oDirectDeviceCache.bRegistExit = true;

                        atexit(
                            []()
                            {
                                // 故意没有加锁。防止卡住。
                                auto _pOld = Sync::ExchangePoint(&s_oDirectDeviceCache.pDirectDeviceCache, INVALID_DIRECT_DEVICE_CACHE_FACTORY);
                                if (_pOld && _pOld != INVALID_DIRECT_DEVICE_CACHE_FACTORY)
                                    _pOld->Release();
                            });
                    }                    
                } while (false);

                ReleaseSRWLockExclusive(&s_oDirectDeviceCache.oLock);

                return _pTmp;
            }

            void __YYAPI D2D1_1DrawContext::InvalidDirectDeviceCache(RefPtr<D2D1_1DrawContext::DirectDeviceCache>&& _oD3DDeviceCache)
            {
                if (_oD3DDeviceCache)
                {
                    if (s_oDirectDeviceCache.pDirectDeviceCache == _oD3DDeviceCache)
                    {
                        AcquireSRWLockExclusive(&s_oDirectDeviceCache.oLock);

                        auto _pOldDirectDeviceCache = Sync::CompareExchangePoint(&s_oDirectDeviceCache.pDirectDeviceCache, nullptr, _oD3DDeviceCache.Get());
                        
                        if (_pOldDirectDeviceCache == _oD3DDeviceCache.Get())
                        {
                            _pOldDirectDeviceCache->Release();
                        }

                        ReleaseSRWLockExclusive(&s_oDirectDeviceCache.oLock);
                    }

                    _oD3DDeviceCache.Reset();
                }
            }

            bool __YYAPI D2D1_1DrawContext::IsSupport()
            {
                auto _pDWriteFactory = DWrite::GetDWriteFactory();
                if (!_pDWriteFactory)
                    return false;

                auto _pD2D1Factory = D2D1_1DrawContext::GetD2D1Factory();
                if (!_pD2D1Factory)
                    return false;

                auto _pDeviceCache = D2D1_1DrawContext::GetDirectDeviceCache();
                if (!_pDeviceCache)
                    return false;

                return true;
            }

            bool __YYAPI D2D1_1DrawContext::IsMicrosoftCompositionEngineSupport()
            {
                auto _pDeviceCache = D2D1_1DrawContext::GetDirectDeviceCache();
                if (!_pDeviceCache)
                    return false;

                return _pDeviceCache->pDcompDevice != nullptr;
            }

            DrawContextFactory* __YYAPI D2D1_1DrawContext::GetDrawContextFactory()
            {
                if (!D2D1_1DrawContext::IsSupport())
                    return nullptr;

                static DrawContextFactoryImpl<D2D1_1DrawContext> s_DrawContextFactory;
                return &s_DrawContextFactory;
            }

            HRESULT __YYAPI D2D1_1DrawContext::CreateDrawTarget(HWND _hWnd, DrawContext** _ppDrawContext)
            {
                if (!_ppDrawContext)
                    return E_INVALIDARG;
                *_ppDrawContext = nullptr;

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return HRESULT_From_LSTATUS(GetLastError());

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

                pDirectDeviceCache = GetDirectDeviceCache();

                RefPtr<IDXGIDevice> _pDxgiDevice;
                // Obtain the underlying DXGI device of the Direct3D11 device.
                auto _hr = pDirectDeviceCache->pD3DDevice->QueryInterface(_pDxgiDevice.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                // Get Direct2D device's corresponding device context object.
                _hr = pDirectDeviceCache->pD2DDevice->CreateDeviceContext(
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
                _SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                if (pDirectDeviceCache->pDcompDevice)
                {
                    uSwapChainBufferCount = 2;
                    _SwapChainDesc.BufferCount = 2;
                    _SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;            
                    _SwapChainDesc.AlphaMode = /*DXGI_ALPHA_MODE_PREMULTIPLIED*/ DXGI_ALPHA_MODE_UNSPECIFIED;

                    _hr = _pDxgiFactory->CreateSwapChainForComposition(_pDxgiDevice, &_SwapChainDesc, nullptr, pSwapChain.ReleaseAndGetAddressOf());

                    _hr = pDirectDeviceCache->pDcompDevice->CreateTargetForHwnd(hWnd, TRUE, pTarget.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    RefPtr<IDCompositionVisual> _pVisual;
                    _hr = pDirectDeviceCache->pDcompDevice->CreateVisual(_pVisual.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    _hr = _pVisual->SetContent(pSwapChain);
                    if (FAILED(_hr))
                        return _hr;
                    _hr = pTarget->SetRoot(_pVisual);
                    if (FAILED(_hr))
                        return _hr;

                    _hr = pDirectDeviceCache->pDcompDevice->Commit();
                    if (FAILED(_hr))
                        return _hr;
                }
                else
                {
                    uSwapChainBufferCount = 1;
                    _SwapChainDesc.BufferCount = 1;
                    _SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
                    _SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

                    _hr = _pDxgiFactory->CreateSwapChainForHwnd(
                        pDirectDeviceCache->pD3DDevice,
                        hWnd,
                        &_SwapChainDesc,
                        nullptr,
                        nullptr,
                        pSwapChain.ReleaseAndGetAddressOf());
                }

                if (FAILED(_hr))
                    return _hr;

                // Direct2D needs the dxgi version of the backbuffer surface pointer.
                RefPtr<IDXGISurface> _pDxgiBackBuffer;
                _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(_pDxgiBackBuffer.ReleaseAndGetAddressOf()));
                if (FAILED(_hr))
                    return _hr;

                D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

                // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                    _pDxgiBackBuffer,
                    &_BitmapProperties,
                    pD2DTargetBitmap.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                pDeviceContext->SetTarget(pD2DTargetBitmap);
                
                return S_OK;
            }

            HRESULT __YYAPI D2D1_1DrawContext::UpdateTargetPixelSize(bool _bCopyToNewTarget)
            {
                if (!pDeviceContext)
                    return S_FALSE;

                auto _oPrevPixelSize = pDeviceContext->GetPixelSize();
                if (_oPrevPixelSize.width == PixelSize.width && _oPrevPixelSize.height == PixelSize.height)
                    return S_FALSE;

                bFirstPaint = true;
                
                const D2D1_SIZE_U _oCopyPixelSize = {min(_oPrevPixelSize.width, PixelSize.width), min(_oPrevPixelSize.height, PixelSize.height)};
                const auto _oPixelFormat = pDeviceContext->GetPixelFormat();

                RefPtr<ID3D11Texture2D> _pBackupTexture2D;
      
                do
                {
                    if (_bCopyToNewTarget)
                    {
                        // 为了支持脏矩阵，临时保存前台缓冲区，增量绘制。
                        RefPtr<ID3D11Texture2D> _pFrontTexture2D;
                        auto _hr = pSwapChain->GetBuffer(uSwapChainBufferCount - 1, IID_PPV_ARGS(_pFrontTexture2D.ReleaseAndGetAddressOf()));
                        if (FAILED(_hr))
                            return _hr;
                        D3D11_TEXTURE2D_DESC _SrcDesc;
                        _pFrontTexture2D->GetDesc(&_SrcDesc);
                        _SrcDesc.Width = _oCopyPixelSize.width;
                        _SrcDesc.Height = _oCopyPixelSize.height;
                        _SrcDesc.Usage = D3D11_USAGE_DEFAULT;
                        _SrcDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                        _SrcDesc.CPUAccessFlags = 0;
                        _SrcDesc.MiscFlags = 0;
                        _hr = pDirectDeviceCache->pD3DDevice->CreateTexture2D(&_SrcDesc, NULL, _pBackupTexture2D.ReleaseAndGetAddressOf());
                        if (FAILED(_hr))
                            return _hr;

                        D3D11_BOX _oIntersectBox;
                        _oIntersectBox.left = 0;
                        _oIntersectBox.top = 0;
                        _oIntersectBox.front = 0;
                        _oIntersectBox.right = _oCopyPixelSize.width;
                        _oIntersectBox.bottom = _oCopyPixelSize.height;
                        _oIntersectBox.back = 1;
                        pDirectDeviceCache->pD3DDeviceContext->CopySubresourceRegion(_pBackupTexture2D, 0, 0, 0, 0, _pFrontTexture2D, 0, &_oIntersectBox);
                    }
                    
                    pDeviceContext->SetTarget(nullptr);
                    pD2DTargetBitmap = nullptr;

                    auto _hr = pSwapChain->ResizeBuffers(uSwapChainBufferCount, PixelSize.width, PixelSize.height, DXGI_FORMAT_UNKNOWN, 0);
                    if (FAILED(_hr))
                        return _hr;

                    RefPtr<ID3D11Texture2D> _pBackTexture2D;
                    RefPtr<IDXGISurface> pDxgiBackBuffer;
                    _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(_pBackTexture2D.ReleaseAndGetAddressOf()));
                    if (FAILED(_hr))
                        return _hr;

                    if (_pBackupTexture2D)
                    {
                        D3D11_BOX _oIntersectBox;
                        _oIntersectBox.left = 0;
                        _oIntersectBox.top = 0;
                        _oIntersectBox.front = 0;
                        _oIntersectBox.right = _oCopyPixelSize.width;
                        _oIntersectBox.bottom = _oCopyPixelSize.height;
                        _oIntersectBox.back = 1;

                        RefPtr<ID2D1Multithread> _pD2DMultithread;
                        GetD2D1Factory()->QueryInterface(_pD2DMultithread.ReleaseAndGetAddressOf());
                        if (_pD2DMultithread)
                            _pD2DMultithread->Enter();
                        pDirectDeviceCache->pD3DDeviceContext->CopySubresourceRegion(_pBackTexture2D, 0, 0, 0, 0, _pBackupTexture2D, 0, &_oIntersectBox);
                        if (_pD2DMultithread)
                            _pD2DMultithread->Leave();
                    }

                    // Direct2D needs the dxgi version of the backbuffer surface pointer.
                    _hr = _pBackTexture2D->QueryInterface(pDxgiBackBuffer.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        _oPixelFormat);

                    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                    _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                        pDxgiBackBuffer,
                        &_BitmapProperties,
                        pD2DTargetBitmap.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    pDeviceContext->SetTarget(pD2DTargetBitmap);

                } while (false);

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
                if (bFirstPaint)
                    _pPaint = nullptr;

                // 无效区域恰好为绘制区域大小其实跟没有无效区域是一样的
                if (_pPaint && _pPaint->Left <= 0 && _pPaint->Top <= 0 && _pPaint->Left >= PixelSize.width && _pPaint->Bottom >= PixelSize.height)
                    _pPaint = nullptr;

                auto _hr = UpdateTargetPixelSize(_pPaint != nullptr);
                if (FAILED(_hr))
                    return _hr;

                _hr = TryInitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;
                
                pDeviceContext->BeginDraw();                
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
                        pD2DTargetBitmap = nullptr;
                        pSwapChain = nullptr;
                        pTarget = nullptr;
                        InvalidDirectDeviceCache(std::move(pDirectDeviceCache));
                    }
                    else
                    {
                        pDeviceContext->Flush();
                    }
                }
                else
                {
                    _hr = pSwapChain->Present1(0, 0, &_oPresentParameters);
                    if (SUCCEEDED(_hr))
                    {
                        bFirstPaint = false;
                    }
                }

                oPaint.Reset();
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
                if (!pDeviceContext)
                    return;

                pDeviceContext->DrawLine(_oPoint0, _oPoint1, GetNativeBrush(_oPen.GetBrush()), _oPen.GetThickness());
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

            RefPtr<IDWriteTextLayout> __YYAPI D2D1_1DrawContext::CreateTextLayout(uString _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign)
            {
                RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = DWrite::CreateTextFormat(_FontInfo, _fTextAlign, L"", _pTextFormat.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                {
                    return nullptr;
                }

                RefPtr<IDWriteTextLayout> _pTextLayout;
                _hr = DWrite::CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutSize, _pTextLayout.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                {
                    return nullptr;
                }

                return _pTextLayout;
            }

            void __YYAPI D2D1_1DrawContext::DrawString2(const Point& _Origin, RefPtr<IDWriteTextLayout> _pTextLayout, Brush _oBrush)
            {
                if (!pDeviceContext.Get())
                    return;

                auto _oNativeBrush = GetNativeBrush(_oBrush);
                if (!_oNativeBrush.Get())
                    return;

                pDeviceContext.Get()->DrawTextLayout(_Origin, _pTextLayout, _oNativeBrush, D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
