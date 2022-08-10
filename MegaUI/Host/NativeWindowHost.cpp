#include "pch.h"
#include "NativeWindowHost.h"
#include <atlcomcli.h>


#pragma comment(lib, "d2d1.lib")

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        NativeWindowHost::NativeWindowHost()
            : hWnd(NULL)
            , pHost(nullptr)
            , m_pD2DFactory(nullptr)
            , m_pWICFactory(nullptr)
            , m_pDWriteFactory(nullptr)
            , m_pRenderTarget(nullptr)
            , m_pCompatibleRenderTarget(nullptr)
            , m_pTextFormat(nullptr)
            , m_pPathGeometry(nullptr)
            , m_pLinearGradientBrush(nullptr)
            , m_pBlackBrush(nullptr)
            , m_pGridPatternBitmapBrush(nullptr)
            , m_pBitmap(nullptr)
            , m_pAnotherBitmap(nullptr)
        {
        }

        NativeWindowHost::~NativeWindowHost()
        {
        }

        HRESULT
        __fastcall
        NativeWindowHost::Initialize(
            LPCWSTR _szTitle,
            HWND    _hWndParent,
            HICON   _hIcon,
            int     _dX,
            int     _dY,
            int     _dWidth,
            int     _dHeight,
            DWORD   _fExStyle,
            DWORD   _fStyle,
            UINT    _nOptions
            )
        {
            //_pe = NULL;
            //_hWnd = NULL;

            //_nOptions = nOptions;

            // Make sure window class is registered
            WNDCLASSEXW wcex;

            // Register host window class, if needed
            wcex.cbSize = sizeof(wcex);

            if (!GetClassInfoExW(GetModuleHandleW(NULL), L"MegaUI", &wcex))
            {
                ZeroMemory(&wcex, sizeof(wcex));

                wcex.cbSize = sizeof(wcex);
                wcex.style = CS_GLOBALCLASS;
                wcex.hInstance = GetModuleHandleW(NULL);
                wcex.hIcon = _hIcon;
                wcex.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
                wcex.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
                wcex.lpszClassName = L"MegaUI";
                wcex.lpfnWndProc = DefWindowProcW;

                if (RegisterClassExW(&wcex) == 0)
                    return E_UNEXPECTED;
            }

            hWnd = CreateWindowExW(
                _fExStyle,
                L"MegaUI",
                _szTitle,
                _fStyle | WS_CLIPCHILDREN,
                _dX, _dY, _dWidth, _dHeight,
                _hWndParent,
                0,
                NULL,
                NULL);

            if (!hWnd)
                return E_UNEXPECTED;

            SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)NativeWindowHost::WndProc);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)this);

            // If top-level, initialize keyboard cue state, start all hidden
            if (!_hWndParent)
                SendMessage(hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);

            return S_OK;
        }

        HRESULT
        __fastcall
        NativeWindowHost::Create(
            LPCWSTR _szTitle,
            HWND    _hWndParent,
            HICON   _hIcon,
            int     _dX,
            int     _dY,
            int     _dWidth,
            int     _dHeight,
            DWORD   _fExStyle,
            DWORD   _fStyle,
            UINT    _nOptions,
            NativeWindowHost** _ppWindow
            )
        {
            if (!_ppWindow)
                return E_INVALIDARG;
            *_ppWindow = nullptr;

            auto _pWindow = HNew<NativeWindowHost>();
            if (!_pWindow)
                return E_OUTOFMEMORY;

            auto _hr = _pWindow->Initialize(_szTitle, _hWndParent, _hIcon, _dX, _dY, _dWidth, _dHeight, _fExStyle, _fStyle, _nOptions);
            if (SUCCEEDED(_hr))
            {
                *_ppWindow = _pWindow;
            }
            else
            {
                HDelete(_pWindow);
            }

            return _hr;
        }
        
        void __fastcall NativeWindowHost::ShowWindow(int _iCmdShow)
        {
            if (hWnd)
                ::ShowWindow(hWnd, _iCmdShow);
        }

        void NativeWindowHost::DestroyWindow()
        {
            ::PostMessageW(hWnd, NativeWindowHost::AsyncDestroyMsg(), 0, 0);
        }

        HRESULT __fastcall NativeWindowHost::SetHost(Element* _pHost)
        {
            if (!_pHost)
                return E_INVALIDARG;

            // 已经初始化了。
            if (pHost)
                return E_UNEXPECTED;

            // 需要首先初始化。
            if (hWnd == NULL)
                return E_UNEXPECTED;
            
            RECT _ClientRect;
            if (!GetClientRect(hWnd, &_ClientRect))
                return __HRESULT_FROM_WIN32(GetLastError());

            pHost = _pHost;

            intptr_t _Cooike;
            pHost->StartDefer(&_Cooike);

            pHost->SetWidth(_ClientRect.right - _ClientRect.left);
            pHost->SetHeight(_ClientRect.bottom - _ClientRect.top);

            pHost->EndDefer(_Cooike);

            return S_OK;
        }
        
        LRESULT NativeWindowHost::WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
        {
            auto _pNativeWindow = (NativeWindowHost*)GetWindowLongPtrW(_hWnd, GWLP_USERDATA);

            if (_uMsg == NativeWindowHost::AsyncDestroyMsg())
            {
                ::DestroyWindow(_hWnd);
                return 0;
            }

            switch (_uMsg)
            {
            case WM_CLOSE:
                if (_pNativeWindow)
                {
                    _pNativeWindow->DestroyWindow();
                    return 0;
                }
                break;
            case WM_DESTROY:
                if (_pNativeWindow)
                {
                    PostQuitMessage(0);
                    _pNativeWindow->hWnd = NULL;
                    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, NULL);
                    HDelete(_pNativeWindow);
                }
                break;
            case WM_PAINT:
                if (_pNativeWindow)
                {
                    _pNativeWindow->OnPaint();
                    ValidateRect(_hWnd, NULL);
                    return 0;
                }
                break;
            case WM_SIZE:
                if (_pNativeWindow)
                {
                    _pNativeWindow->OnSize(LOWORD(_lParam), HIWORD(_lParam));
                }
            default:
                break;
            }

            return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
        }
        
        UINT __fastcall NativeWindowHost::AsyncDestroyMsg()
        {
            static UINT g_AsyncDestroyMsg = 0;

            if (g_AsyncDestroyMsg == 0)
            {
                g_AsyncDestroyMsg = RegisterWindowMessageW(L"MegaUIAsyncDestroy");
            }

            return g_AsyncDestroyMsg;
        }

        HRESULT __fastcall NativeWindowHost::InitializeD2D()
        {
            if (m_pD2DFactory)
                return S_OK;

            auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
            if (FAILED(hr))
                return hr;

            if (!m_pD2DFactory)
                return E_UNEXPECTED;

            return hr;
        }

        HRESULT __fastcall NativeWindowHost::OnPaint()
        {
            auto hr = InitializeD2D();
            if (FAILED(hr))
                return hr;

            hr = InitializeRenderTarget();

            if (FAILED(hr))
                return hr;

            m_pCompatibleRenderTarget->BeginDraw();
            do
            {
               /* ATL::CComPtr<ID2D1SolidColorBrush> m_pBlackBrush;
                hr = m_pRenderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(D2D1::ColorF::DarkRed),
                    &m_pBlackBrush);*/

                
                auto _RenderTargetSize = m_pRenderTarget->GetSize();
                Rect _Bounds(0, 0, _RenderTargetSize.width, _RenderTargetSize.height);
                PaintElement(m_pCompatibleRenderTarget, pHost, _Bounds, _Bounds);
                
                //m_pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, _RenderTargetSize.width, _RenderTargetSize.height), m_pBlackBrush);


            } while (false);
            m_pCompatibleRenderTarget->EndDraw();
            
            ATL::CComPtr<ID2D1Bitmap> _pBitmap;
            hr = m_pCompatibleRenderTarget->GetBitmap(&_pBitmap);
            if (FAILED(hr))
                return hr;

            m_pRenderTarget->BeginDraw();

            m_pRenderTarget->DrawBitmap(_pBitmap);

            hr = m_pRenderTarget->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET)
            {
                hr = S_OK;
                m_pCompatibleRenderTarget->Release();
                m_pCompatibleRenderTarget = nullptr;
                m_pRenderTarget->Release();
                m_pRenderTarget = nullptr;
            }

            return hr;
        }

        HRESULT __fastcall NativeWindowHost::PaintElement(ID2D1BitmapRenderTarget* _pCompatibleRenderTarget, Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentPaintRect)
        {
            if (_pCompatibleRenderTarget == nullptr || _pElement == nullptr)
                return E_INVALIDARG;

            auto _Location = _pElement->GetLocation();
            auto _Extent = _pElement->GetExtent();

            Rect _BoundsElement;
            _BoundsElement.left = _ParentBounds.left + _Location.x;
            _BoundsElement.right = _BoundsElement.left + _Extent.cx;

            _BoundsElement.top = _ParentBounds.top + _Location.y;
            _BoundsElement.bottom = _BoundsElement.top + _Extent.cy;
            
            // 如果没有交集，那么我们可以不绘制
            Rect _PaintRect;
            if (!IntersectRect(&_PaintRect, &_ParentPaintRect, &_BoundsElement))
                return S_OK;

            const auto _bNeedClip = _PaintRect != _BoundsElement;
            if (_bNeedClip)
            {
                m_pCompatibleRenderTarget->PushAxisAlignedClip(_PaintRect, D2D1_ANTIALIAS_MODE_ALIASED);
            }

            _pElement->Paint(m_pCompatibleRenderTarget, _BoundsElement);

            if (_bNeedClip)
                m_pCompatibleRenderTarget->PopAxisAlignedClip();

            for (auto pItem : _pElement->GetChildren())
            {
                PaintElement(_pCompatibleRenderTarget, pItem, _BoundsElement, _ParentPaintRect);
            }

            return S_OK;
        }

        HRESULT __fastcall NativeWindowHost::InitializeRenderTarget()
        {
            if (!m_pRenderTarget)
            {

                if (!m_pD2DFactory)
                    return E_UNEXPECTED;

                RECT rc;
                GetClientRect(hWnd, &rc);

                D2D1_SIZE_U size = D2D1::SizeU(
                    static_cast<UINT>(rc.right - rc.left),
                    static_cast<UINT>(rc.bottom - rc.top));

                // Create a Direct2D render target.
                auto hr = m_pD2DFactory->CreateHwndRenderTarget(
                    D2D1::RenderTargetProperties(),
                    D2D1::HwndRenderTargetProperties(hWnd, size),
                    &m_pRenderTarget);

                if (FAILED(hr))
                    return hr;

                if (!m_pRenderTarget)
                    return E_UNEXPECTED;
            }

            if (!m_pCompatibleRenderTarget)
            {
                auto hr = m_pRenderTarget->CreateCompatibleRenderTarget(&m_pCompatibleRenderTarget);
                if (FAILED(hr))
                    return hr;

                if (!m_pCompatibleRenderTarget)
                    return E_UNEXPECTED;
            }
            return S_OK;
        }

        void __fastcall NativeWindowHost::OnSize(UINT _uWidth, UINT _uHeight)
        {
            if (m_pRenderTarget)
            {
                intptr_t _Cooike;
                pHost->StartDefer(&_Cooike);

                pHost->SetWidth(_uWidth);
                pHost->SetHeight(_uHeight);

                pHost->EndDefer(_Cooike);

                D2D1_SIZE_U _NewSize;
                _NewSize.width = _uWidth;
                _NewSize.height = _uHeight;

                m_pRenderTarget->Resize(_NewSize);
            }
            
            if (m_pCompatibleRenderTarget)
            {
                m_pCompatibleRenderTarget->Release();
                m_pCompatibleRenderTarget = nullptr;
            }
        }
        

    } // namespace MegaUI
} // namespace YY
