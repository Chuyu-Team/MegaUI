#include "pch.h"
#include "D2D1_0DrawContext.h"

#include <dxgi1_2.h>

#include <Media/Graphics/D2D/DWriteHelper.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "D3D10_1.lib")

constexpr UINT uSwapChainBufferCount = 1;

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
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, D2D1_FACTORY_OPTIONS {D2D1_DEBUG_LEVEL_INFORMATION}, &_pD2DFactory);
#else
                auto _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &_pD2DFactory);
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

            struct D3D10Device1Cache
            {
                ID3D10Device1* pDevice;
                SRWLOCK oLock;
                bool bRegistExit;
                bool bInit;
            };

            static D3D10Device1Cache s_oD3D10Device1Cache;

            RefPtr<ID3D10Device1> __YYAPI D2D1_0DrawContext::GetD3D10DeviceCache()
            {
                RefPtr<ID3D10Device1> _oTmp;
                bool _bInit = false;
                AcquireSRWLockShared(&s_oD3D10Device1Cache.oLock);
                if (s_oD3D10Device1Cache.bInit)
                {
                    _oTmp = s_oD3D10Device1Cache.pDevice;
                    _bInit = true;
                }
                ReleaseSRWLockShared(&s_oD3D10Device1Cache.oLock);

                if (_bInit)
                {
                    return _oTmp;
                }
                
                HRESULT _hr = E_NOINTERFACE;

                AcquireSRWLockExclusive(&s_oD3D10Device1Cache.oLock);

                do
                {
                    if (s_oD3D10Device1Cache.bInit)
                        break;
                    s_oD3D10Device1Cache.bInit = true;

     #ifdef _DEBUG
                    constexpr DWORD _fD3D10Flags = D3D10_CREATE_DEVICE_BGRA_SUPPORT | D3D10_CREATE_DEVICE_DEBUG;
    #else
                    constexpr DWORD _fD3D10Flags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
    #endif
                    static constexpr D3D10_DRIVER_TYPE _arrDriverTypes[] =
                    {
                        D3D10_DRIVER_TYPE_HARDWARE,
                        D3D10_DRIVER_TYPE_WARP,
                    };

                    static constexpr D3D10_FEATURE_LEVEL1 _arrFeatureLevels[] =
                    {
                        D3D10_FEATURE_LEVEL_10_1,
                        D3D10_FEATURE_LEVEL_10_0,
                        D3D10_FEATURE_LEVEL_9_3,
                        D3D10_FEATURE_LEVEL_9_2,
                        D3D10_FEATURE_LEVEL_9_1,
                    };

                    for (auto _eDriverType : _arrDriverTypes)
                    {
                        for (auto _eFeatureLevel : _arrFeatureLevels)
                        {
                            _hr = D3D10CreateDevice1(
                                nullptr,
                                _eDriverType,
                                NULL,
                                _fD3D10Flags,
                                _eFeatureLevel,
                                D3D10_1_SDK_VERSION,
                                &s_oD3D10Device1Cache.pDevice);

                            if (SUCCEEDED(_hr))
                                goto CREATE_SUCCESS__;
                        }
                    }

                    break;

                CREATE_SUCCESS__:
                    if (!s_oD3D10Device1Cache.bRegistExit)
                    {
                        s_oD3D10Device1Cache.bRegistExit = true;

                        atexit(
                            []()
                            {
                                // 故意没有加锁。防止卡住。
                                auto _pDevice = s_oD3D10Device1Cache.pDevice;
                                s_oD3D10Device1Cache.pDevice = nullptr;
                                if (_pDevice)
                                    _pDevice->Release();
                            });
                    }
                    
                    _oTmp = s_oD3D10Device1Cache.pDevice;
                } while (false);

              
                ReleaseSRWLockExclusive(&s_oD3D10Device1Cache.oLock);

                return _oTmp;
            }

            void __YYAPI D2D1_0DrawContext::InvalidD3D10DeviceCache(RefPtr<ID3D10Device1>&& _pD3D10Device1Cache)
            {
                if (_pD3D10Device1Cache == nullptr)
                {
                    return;
                }

                if (s_oD3D10Device1Cache.pDevice == _pD3D10Device1Cache)
                {
                    AcquireSRWLockExclusive(&s_oD3D10Device1Cache.oLock);

                    if (s_oD3D10Device1Cache.pDevice == _pD3D10Device1Cache)
                    {
                        s_oD3D10Device1Cache.pDevice = nullptr;
                        s_oD3D10Device1Cache.bInit = false;
                        _pD3D10Device1Cache->Release();
                    }

                    ReleaseSRWLockExclusive(&s_oD3D10Device1Cache.oLock);
                }

                _pD3D10Device1Cache = nullptr;
            }

            DrawContextFactory* __YYAPI D2D1_0DrawContext::GetDrawContextFactory()
            {
                if (!D2D1_0DrawContext::IsSupport())
                    return nullptr;

                static DrawContextFactoryImpl<D2D1_0DrawContext> s_DrawContextFactory;
                return &s_DrawContextFactory;
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
                    return HRESULT_From_LSTATUS(GetLastError());

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

            bool __YYAPI D2D1_0DrawContext::IsSupport()
            {
                auto _pDWriteFactory = DWrite::GetDWriteFactory();
                if (!_pDWriteFactory)
                    return false;
                auto _pD2D1Factory = D2D1_0DrawContext::GetD2D1Factory();
                if (!_pD2D1Factory)
                    return false;

                auto _pDeviceCache = D2D1_0DrawContext::GetD3D10DeviceCache();
                if (!_pDeviceCache)
                    return false;

                return true;
            }

            bool __YYAPI D2D1_0DrawContext::IsMicrosoftCompositionEngineSupport()
            {
                return false;
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
                if (bFirstPaint)
                    _pPaint = nullptr;

                // 无效区域跟整个绘制大小一样就不用无效区域了。
                if (_pPaint && _pPaint->Left <= 0 && _pPaint->Top <= 0 && _pPaint->Left >= PixelSize.width && _pPaint->Bottom >= PixelSize.height)
                    _pPaint = nullptr;

                auto _hr = UpdateTargetPixelSize(_pPaint != nullptr);
                if (FAILED(_hr))
                    return _hr;

                _hr = TryInitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;

                pRenderTarget->BeginDraw();

                if (_pPaint)
                {
                    oPaint = *_pPaint;
                    PushAxisAlignedClip(*_pPaint);
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

                if (SUCCEEDED(_hr))
                {
                    _hr = pSwapChain->Present(0, 0);
                }

                if (FAILED(_hr))
                {
                    // 需要重新呈现
                    if (_hr == D2DERR_RECREATE_TARGET)
                    {
                        _hr = S_OK;
                        bFirstPaint = true;
                        pRenderTarget = nullptr;
                        pBackBuffer = nullptr;
                        pSwapChain = nullptr;
                        InvalidD3D10DeviceCache(std::move(pD3DDevice));
                    }

                    return _hr;
                }

                bFirstPaint = false;
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

                pD3DDevice = GetD3D10DeviceCache();
                if (!pD3DDevice)
                {
                    return E_NOINTERFACE;
                }

                RefPtr<IDXGIDevice> _pDxgiDevice;
                auto _hr = pD3DDevice->QueryInterface(_pDxgiDevice.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                RefPtr<IDXGIAdapter> _pDxgiAdapter;
                _hr = _pDxgiDevice->GetAdapter(_pDxgiAdapter.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                RefPtr<IDXGIFactory> _pDxgiFactory;
                _hr = _pDxgiAdapter->GetParent(IID_PPV_ARGS(_pDxgiFactory.ReleaseAndGetAddressOf()));
                if (FAILED(_hr))
                    return _hr;
                
                DXGI_SWAP_CHAIN_DESC _oSwapChainDesc = {};
                _oSwapChainDesc.BufferDesc.Width = PixelSize.width;
                _oSwapChainDesc.BufferDesc.Height = PixelSize.height;
                _oSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
                _oSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
                _oSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                _oSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                _oSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                _oSwapChainDesc.SampleDesc.Count = 1;
                _oSwapChainDesc.SampleDesc.Quality = 0;
                _oSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                _oSwapChainDesc.BufferCount = uSwapChainBufferCount;
                _oSwapChainDesc.OutputWindow = hWnd;
                _oSwapChainDesc.Windowed = TRUE;
                _oSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

                _hr = _pDxgiFactory->CreateSwapChain(pD3DDevice.Get(), &_oSwapChainDesc, pSwapChain.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                // Create our BackBuffer
                _hr = pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)pBackBuffer.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                RefPtr<IDXGISurface> _pDxgiBackBuffer;
                pBackBuffer->QueryInterface(_pDxgiBackBuffer.ReleaseAndGetAddressOf());

                _hr = _pD2D1Factory->CreateDxgiSurfaceRenderTarget(
                    _pDxgiBackBuffer,
                    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f),
                    pRenderTarget.ReleaseAndGetAddressOf());

#if 0
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
#endif

                return _hr;
            }

            HRESULT __YYAPI D2D1_0DrawContext::UpdateTargetPixelSize(bool _bCopyToNewTarget)
            {
                if (!pRenderTarget)
                {
                    return S_FALSE;
                }

                const auto _oPrevPixelSize = pRenderTarget->GetPixelSize();
                if (_oPrevPixelSize.width == PixelSize.width && _oPrevPixelSize.height == PixelSize.height)
                {
                    return S_FALSE;
                }

                bFirstPaint = true;
                const D2D1_SIZE_U _oCopyPixelSize = {min(_oPrevPixelSize.width, PixelSize.width), min(_oPrevPixelSize.height, PixelSize.height)};
                const auto _oPixelFormat = pRenderTarget->GetPixelFormat();
                
                RefPtr<ID3D10Multithread> _pD3DMultithread;
                RefPtr<ID3D10Texture2D> _pBackupTexture2D;
                HRESULT _hr;

                if (_bCopyToNewTarget)
                {
                    pD3DDevice->QueryInterface(_pD3DMultithread.ReleaseAndGetAddressOf());

                    D3D10_TEXTURE2D_DESC _SrcDesc;
                    pBackBuffer->GetDesc(&_SrcDesc);
                    _SrcDesc.Width = _oCopyPixelSize.width;
                    _SrcDesc.Height = _oCopyPixelSize.height;
                    _SrcDesc.Usage = D3D10_USAGE_DEFAULT;
                    _SrcDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
                    _SrcDesc.CPUAccessFlags = 0;
                    _SrcDesc.MiscFlags = 0;
                    _hr = pD3DDevice->CreateTexture2D(&_SrcDesc, NULL, _pBackupTexture2D.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    D3D10_BOX _oIntersectBox;
                    _oIntersectBox.left = 0;
                    _oIntersectBox.top = 0;
                    _oIntersectBox.front = 0;
                    _oIntersectBox.right = _oCopyPixelSize.width;
                    _oIntersectBox.bottom = _oCopyPixelSize.height;
                    _oIntersectBox.back = 1;
                    if (_pD3DMultithread)
                        _pD3DMultithread->Enter();
                    pD3DDevice->CopySubresourceRegion(_pBackupTexture2D, 0, 0, 0, 0, pBackBuffer, 0, &_oIntersectBox);
                    if (_pD3DMultithread)
                        _pD3DMultithread->Leave();
                }

                pBackBuffer = nullptr;
                pRenderTarget = nullptr;

                _hr = pSwapChain->ResizeBuffers(uSwapChainBufferCount, PixelSize.width, PixelSize.height, DXGI_FORMAT_UNKNOWN, 0);
                if (FAILED(_hr))
                {
                    return _hr;
                }
                _hr = pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)pBackBuffer.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                if (_pBackupTexture2D)
                {
                    D3D10_BOX _oIntersectBox;
                    _oIntersectBox.left = 0;
                    _oIntersectBox.top = 0;
                    _oIntersectBox.front = 0;
                    _oIntersectBox.right = _oCopyPixelSize.width;
                    _oIntersectBox.bottom = _oCopyPixelSize.height;
                    _oIntersectBox.back = 1;
                    // RefPtr<ID3D10Multithread> _pD3DMultithread;
                    // pD3DDevice->QueryInterface(_pD3DMultithread.ReleaseAndGetAddressOf());
                    if (_pD3DMultithread)
                        _pD3DMultithread->Enter();
                    pD3DDevice->CopySubresourceRegion(pBackBuffer, 0, 0, 0, 0, _pBackupTexture2D, 0, &_oIntersectBox);
                    if (_pD3DMultithread)
                        _pD3DMultithread->Leave();
                }

                RefPtr<IDXGISurface> _pDxgiBackBuffer;
                _hr = pBackBuffer->QueryInterface(_pDxgiBackBuffer.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    return _hr;

                const auto _pD2D1Factory = GetD2D1Factory();
                _hr = _pD2D1Factory->CreateDxgiSurfaceRenderTarget(
                    _pDxgiBackBuffer,
                    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, _oPixelFormat, 96.0f, 96.0f),
                    pRenderTarget.ReleaseAndGetAddressOf());

                return _hr;
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
