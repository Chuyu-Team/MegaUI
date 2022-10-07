#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "..\..\base\MegaUITypeInt.h"
#include "..\Render.h"
#include "..\..\base\alloc.h"
#include <MegaUI/base/ComPtr.h>
#include <MegaUI/Render/D2D/DWrite/DWriteHelper.h>

#pragma pack(push, __MEGA_UI_PACKING)

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "D3D11.lib")

namespace YY
{
    namespace MegaUI
    {
        // 适用于Windows 8以及更高版本
        class D2D1_1Render : public Render
        {
        private:
            HWND hWnd;
            ID2D1Factory1* pD2DFactory;
            ID2D1Device* pD2DDevice;
            ID2D1DeviceContext* pDeviceContext;
            IDXGISwapChain1* pSwapChain;
            ID2D1Bitmap1* pD2DTargetBitmap;
            D2D1_SIZE_U PixelSize;
            D3D_FEATURE_LEVEL FeatureLevel;
            DXGI_PRESENT_PARAMETERS PresentParameters;
            DWriteHelper DWrite;
        public:
            D2D1_1Render(HWND _hWnd, ID2D1Factory1* _pD2DFactory, const D2D1_SIZE_U& _PixelSize)
                : hWnd(_hWnd)
                , pD2DFactory(_pD2DFactory)
                , pD2DDevice(nullptr)
                , pDeviceContext(nullptr)
                , pSwapChain(nullptr)
                , pD2DTargetBitmap(nullptr)
                , PixelSize(_PixelSize)
                , FeatureLevel()
                , PresentParameters {}
            {
            }

            D2D1_1Render(const D2D1_1Render&) = delete;

            ~D2D1_1Render()
            {
                UnitializeRenderTarget();
                if (pD2DFactory)
                    pD2DFactory->Release();
            }

            void __MEGA_UI_API operator=(const D2D1_1Render&) = delete;

            HRESULT __MEGA_UI_API Initialize()
            {
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
                ComPtr<ID3D11Device> _pD3DDevice;
                ComPtr<ID3D11DeviceContext> _pD3DDeviceContext;

                auto _hr = D3D11CreateDevice(
                    nullptr, // specify null to use the default adapter
                    D3D_DRIVER_TYPE_HARDWARE,
                    0,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT, // optionally set debug and Direct2D compatibility flags
                    _FeatureLevels,                   // list of feature levels this app can support
                    _countof(_FeatureLevels),         // number of possible feature levels
                    D3D11_SDK_VERSION,
                    &_pD3DDevice,  // returns the Direct3D device created
                    &FeatureLevel, // returns feature level of device created
                    &_pD3DDeviceContext // returns the device immediate context
                    );
                if (FAILED(_hr))
                    return _hr;

                do
                {
                    ComPtr<IDXGIDevice> _pDxgiDevice;
                    // Obtain the underlying DXGI device of the Direct3D11 device.
                    _hr = _pD3DDevice->QueryInterface(&_pDxgiDevice);
                    if (FAILED(_hr))
                        break;

                    // Obtain the Direct2D device for 2-D rendering.
                    _hr = pD2DFactory->CreateDevice(_pDxgiDevice, &pD2DDevice);
                    if (FAILED(_hr))
                        break;

                    // Get Direct2D device's corresponding device context object.
                    _hr = pD2DDevice->CreateDeviceContext(
                        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                        &pDeviceContext);
                    if (FAILED(_hr))
                        break;

                    pDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);

                    ComPtr<IDXGIAdapter> _pDxgiAdapter;
                    _hr = _pDxgiDevice->GetAdapter(&_pDxgiAdapter);
                    if (FAILED(_hr))
                        break;
                    ComPtr<IDXGIFactory2> _pDxgiFactory;
                    _hr = _pDxgiAdapter->GetParent(IID_PPV_ARGS(&_pDxgiFactory));
                    if (FAILED(_hr))
                        break;
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

                    _SwapChainDesc.BufferCount = 1;
                    _SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                    // 应用商店程序
                    // swapChainDesc.Scaling = DXGI_SCALING_NONE;
                    // swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                    // 桌面应用程序
                    _SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                    _SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

                    _hr = _pDxgiFactory->CreateSwapChainForHwnd(
                        _pD3DDevice,
                        hWnd,
                        &_SwapChainDesc,
                        nullptr,
                        nullptr,
                        &pSwapChain);
                    if (FAILED(_hr))
                        break;

                    // Direct2D needs the dxgi version of the backbuffer surface pointer.
                    ComPtr<IDXGISurface> _pDxgiBackBuffer;
                    _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&_pDxgiBackBuffer));
                    if (FAILED(_hr))
                        break;

                    D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),0, 0);

                    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                    _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                        _pDxgiBackBuffer,
                        &_BitmapProperties,
                        &pD2DTargetBitmap);
                    if (FAILED(_hr))
                        break;

                    pDeviceContext->SetTarget(pD2DTargetBitmap);
                    return S_OK;
                } while (false);

                UnitializeRenderTarget();
                return _hr;
            }

            HRESULT __MEGA_UI_API InitializeRenderTarget()
            {
                if (pDeviceContext)
                    return S_OK;
                
                return Initialize();
            }

            void __MEGA_UI_API UnitializeRenderTarget()
            {
                if (pD2DTargetBitmap)
                {
                    pD2DTargetBitmap->Release();
                    pD2DTargetBitmap = nullptr;
                }

                if (pSwapChain)
                {
                    pSwapChain->Release();
                    pSwapChain = nullptr;
                }

                if (pD2DDevice)
                {
                    pD2DDevice->Release();
                    pD2DDevice = nullptr;
                }

                if (pDeviceContext)
                {
                    pDeviceContext->Release();
                    pDeviceContext = nullptr;
                }
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

                HRESULT _hr = S_OK;
                ID2D1Factory1* _pD2DFactory = nullptr;

                do
                {
                    _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
                    if (FAILED(_hr))
                        break;

                    if (!_pD2DFactory)
                    {
                        _hr = E_UNEXPECTED;
                        break;
                    }

                    D2D1_SIZE_U _PixelSize = D2D1::SizeU(
                        static_cast<UINT>(_ClientRect.right - _ClientRect.left),
                        static_cast<UINT>(_ClientRect.bottom - _ClientRect.top));

                    auto _pD2DRender = HNew<D2D1_1Render>(_hWnd, _pD2DFactory, _PixelSize);
                    if (!_pD2DRender)
                    {
                        _hr = E_OUTOFMEMORY;
                        break;
                    }

                    _hr = _pD2DRender->Initialize();

                    if (FAILED(_hr))
                    {
                        HDelete(_pD2DRender);
                        return _hr;
                    }
                    *_ppRender = _pD2DRender;
                    return S_OK;
                } while (false);

                if (_pD2DFactory)
                    _pD2DFactory->Release();

                return _hr;
            }

            virtual HRESULT __MEGA_UI_API BeginDraw(Rect* _pNeedPaintRect) override
            {
                if (_pNeedPaintRect)
                {
                    _pNeedPaintRect->Left = 0;
                    _pNeedPaintRect->Top = 0;
                    _pNeedPaintRect->Right = (float)PixelSize.width;
                    _pNeedPaintRect->Bottom = (float)PixelSize.height;
                }
                auto _hr = InitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;

                pDeviceContext->BeginDraw();
                return S_OK;
            }

            virtual HRESULT __MEGA_UI_API EndDraw() override
            {
                if (!pDeviceContext)
                    return E_UNEXPECTED;

                HRESULT _hr = pDeviceContext->EndDraw();

                // 需要重新呈现
                if (_hr == D2DERR_RECREATE_TARGET)
                {
                    _hr = S_OK;
                   
                }

                pSwapChain->Present1(1, 0, &PresentParameters);

                return _hr;
            }

            virtual void __MEGA_UI_API PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) override
            {
                pDeviceContext->PushAxisAlignedClip(_ClipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            }

            virtual void __MEGA_UI_API PopAxisAlignedClip() override
            {
                pDeviceContext->PopAxisAlignedClip();
            }

            virtual void __MEGA_UI_API FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) override
            {
                pDeviceContext->FillRectangle(_Rect, _pBrush);
            }

            virtual HRESULT __MEGA_UI_API SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) override
            {
                if (_PixelSize.width == PixelSize.width && _PixelSize.height == PixelSize.height)
                    return S_OK;

                PixelSize.width = _PixelSize.width;
                PixelSize.height = _PixelSize.height;
                if (!pDeviceContext)
                    return S_OK;

                pDeviceContext->SetTarget(nullptr);
                if (pD2DTargetBitmap)
                {
                    pD2DTargetBitmap->Release();
                    pD2DTargetBitmap = nullptr;
                }
                auto _hr = pSwapChain->ResizeBuffers(1, PixelSize.width, PixelSize.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

                do
                {
                    if (FAILED(_hr))
                        break;

                    // Direct2D needs the dxgi version of the backbuffer surface pointer.
                    ComPtr<IDXGISurface> pDxgiBackBuffer;
                    _hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
                    if (FAILED(_hr))
                        break;

                    D2D1_BITMAP_PROPERTIES1 _BitmapProperties = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);

                    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
                    _hr = pDeviceContext->CreateBitmapFromDxgiSurface(
                        pDxgiBackBuffer,
                        &_BitmapProperties,
                        &pD2DTargetBitmap);
                    if (FAILED(_hr))
                        break;

                    pDeviceContext->SetTarget(pD2DTargetBitmap);
                    return S_OK;
                } while (false);
                UnitializeRenderTarget();
                return _hr;
            }

            D2D1_SIZE_U __MEGA_UI_API GetPixelSize()
            {
                return PixelSize;
            }

            virtual HRESULT __MEGA_UI_API CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) override
            {
                return pDeviceContext->CreateSolidColorBrush(_Color, _ppSolidColorBrush);
            }

            virtual
            void
            __MEGA_UI_API
            DrawString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ Color _crTextColor,
                _In_ const Rect& _LayoutRect,
                _In_ int32_t _fTextAlign
                ) override
            {
                DWrite.DrawString(pDeviceContext, _szText, _FontInfo, _crTextColor, _LayoutRect, _fTextAlign);
            }

            virtual
            void
            __MEGA_UI_API
            MeasureString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Size& _LayoutSize,
                _In_ int32_t _fTextAlign,
                _Out_ Size* _pExtent
                ) override
            {
                DWrite.MeasureString(_szText, _FontInfo, _LayoutSize, _fTextAlign, _pExtent);
            }

        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
