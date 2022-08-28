#include "pch.h"
#include "NativeWindowHost.h"

#include <atlcomcli.h>

#include "../Render/Render.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        NativeWindowHost::NativeWindowHost()
            : hWnd(NULL)
            , pHost(nullptr)
            , pRender(nullptr)
            , LastRenderSize {}
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

            LastRenderSize.width = _ClientRect.right - _ClientRect.left;
            LastRenderSize.height = _ClientRect.bottom - _ClientRect.top;

            pHost = _pHost;

            intptr_t _Cooike;
            pHost->StartDefer(&_Cooike);

            pHost->SetWidth(LastRenderSize.width);
            pHost->SetHeight(LastRenderSize.height);

            pHost->EndDefer(_Cooike);

            return S_OK;
        }

        bool __fastcall NativeWindowHost::IsMinimized() const
        {
            return (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE) != 0;
        }
        
        LRESULT NativeWindowHost::WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
        {
            auto _pNativeWindow = (NativeWindowHost*)GetWindowLongPtrW(_hWnd, GWLP_USERDATA);

            do
            {
                if (!_pNativeWindow)
                    break;

                if (_uMsg == NativeWindowHost::AsyncDestroyMsg())
                {
                    ::DestroyWindow(_hWnd);
                    return 0;
                }

                switch (_uMsg)
                {
                case WM_CLOSE:
                    _pNativeWindow->DestroyWindow();
                    return 0;
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    _pNativeWindow->hWnd = NULL;
                    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, NULL);
                    HDelete(_pNativeWindow);
                    break;
                case WM_PAINT:
                    _pNativeWindow->OnPaint();
                    ValidateRect(_hWnd, NULL);
                    return 0;
                    break;
                case WM_SIZE:
                    // 非最小化时不刷新 Host大小
                    if (!_pNativeWindow->IsMinimized())
                    {
                        _pNativeWindow->OnSize(LOWORD(_lParam), HIWORD(_lParam));
                    }
                    break;
                case WM_MOUSEMOVE:
                {
                    const auto fwKeys = _wParam;
                    if (_pNativeWindow->pHost && fwKeys != MK_LBUTTON)
                    {
                        Rect _Bounds;
                        ::GetWindowRect(_hWnd, &_Bounds);

                        _pNativeWindow->UpdateMouseWithin(_pNativeWindow->pHost, _Bounds, _Bounds, POINT {LOWORD(_lParam), HIWORD(_lParam)});
                    }
                    break;
                }
                default:
                    break;
                }
            } while (false);

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

        HRESULT __fastcall NativeWindowHost::OnPaint()
        {
            if (!pRender)
            {
                auto hr = CreateRender(hWnd, &pRender);
                if (FAILED(hr))
                    return hr;
            }
            
            pRender->SetPixelSize(LastRenderSize);

            Rect _NeedPaint;
            auto _hr = pRender->BeginDraw(&_NeedPaint);
            if (FAILED(_hr))
                return _hr;
            Rect _Bounds(0, 0, LastRenderSize.width, LastRenderSize.height);
            PaintElement(pRender, pHost, _Bounds, _NeedPaint);
            return pRender->EndDraw();
        }

        HRESULT __fastcall NativeWindowHost::PaintElement(Render* _pRender, Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentPaintRect)
        {
            if (_pRender == nullptr || _pElement == nullptr)
                return E_INVALIDARG;
            #if 0
            auto& RenderNode = _pElement->RenderNode;
            auto _uInvalidateMarks = RenderNode.uInvalidateMarks;
            if (_uInvalidateMarks & (ElementRenderNode::InvalidatePosition | ElementRenderNode::InvalidateExtent))
            {
                auto _NewBounds = RenderNode.Bounds;
                if (_uInvalidateMarks & ElementRenderNode::InvalidatePosition)
                {
                    auto _Location = _pElement->GetLocation();
                    _Location.x += _ParentBounds.left;
                    _Location.y += _ParentBounds.top;

                    if(_NewBounds == _Location)
                    {
                        _uInvalidateMarks &= ~ElementRenderNode::InvalidatePosition;
                    }
                    else
                    {
                        _NewBounds.SetPoint(_Location);
                        _uInvalidateMarks |= ElementRenderNode::InvalidateChild;
                    }
                }

                if (_uInvalidateMarks & ElementRenderNode::InvalidateExtent)
                {
                    auto _Extent = _pElement->GetExtent();
                    if (_NewBounds == _Extent)
                    {
                        _uInvalidateMarks &= ~ElementRenderNode::InvalidateExtent;
                    }
                    else
                    {
                        _NewBounds.SetSize(_Extent);
                    }
                }

                RenderNode.Bounds = _NewBounds;
            }
            #else
            auto _Location = _pElement->GetLocation();
            auto _Extent = _pElement->GetExtent();

            Rect _BoundsElement;
            _BoundsElement.left = _ParentBounds.left + _Location.x;
            _BoundsElement.right = _BoundsElement.left + _Extent.cx;

            _BoundsElement.top = _ParentBounds.top + _Location.y;
            _BoundsElement.bottom = _BoundsElement.top + _Extent.cy;
            #endif
            // 如果没有交集，那么我们可以不绘制
            Rect _PaintRect;
            if (!IntersectRect(&_PaintRect, &_ParentPaintRect, &_BoundsElement))
                return S_OK;

            const auto _bNeedClip = _PaintRect != _BoundsElement;
            if (_bNeedClip)
            {
                _pRender->PushAxisAlignedClip(_PaintRect);
            }

            _pElement->Paint(_pRender, _BoundsElement);

            if (_bNeedClip)
                _pRender->PopAxisAlignedClip();

            for (auto pItem : _pElement->GetChildren())
            {
                PaintElement(_pRender, pItem, _BoundsElement, _ParentPaintRect);
            }

            return S_OK;
        }

        void __fastcall NativeWindowHost::UpdateMouseWithin(
            Element* _pElement,
            const Rect& _ParentBounds,
            const Rect& _ParentVisibleBounds,
            const POINT& _ptPoint)
        {
            auto _Location = _pElement->GetLocation();
            auto _Extent = _pElement->GetExtent();

            Rect _BoundsElement;
            _BoundsElement.left = _ParentBounds.left + _Location.x;
            _BoundsElement.right = _BoundsElement.left + _Extent.cx;

            _BoundsElement.top = _ParentBounds.top + _Location.y;
            _BoundsElement.bottom = _BoundsElement.top + _Extent.cy;

            Rect _VisibleBounds;
            if (IntersectRect(&_VisibleBounds, &_ParentVisibleBounds, &_BoundsElement))
            {
                if (_VisibleBounds.PointInRect(_ptPoint))
                {
                    intptr_t _Cooike = 0;

                    if (!_pElement->IsMouseWithin())
                    {
                        _pElement->StartDefer(&_Cooike);

                        _pElement->PreSourceChange(Element::g_ClassInfoData.MouseWithinProp, PropertyIndicies::PI_Local, Value::GetBoolFalse(), Value::GetBoolTrue());
                        _pElement->bLocMouseWithin = TRUE;
                        _pElement->PostSourceChange();
                    }

                    for (auto _pChild : _pElement->GetChildren())
                    {
                        UpdateMouseWithin(_pChild, _BoundsElement, _VisibleBounds, _ptPoint);
                    }

                    if (_Cooike)
                        _pElement->EndDefer(_Cooike);
                    return;
                }
            }

            UpdateMouseWithinToFalse(_pElement);
        }

        void __fastcall NativeWindowHost::UpdateMouseWithinToFalse(Element* _pElement)
        {
            if (!_pElement->IsMouseWithin())
                return;

            intptr_t _Cooike = 0;
            _pElement->StartDefer(&_Cooike);

            _pElement->PreSourceChange(Element::g_ClassInfoData.MouseWithinProp, PropertyIndicies::PI_Local, Value::GetBoolTrue(), Value::GetBoolFalse());
            _pElement->bLocMouseWithin = FALSE;
            _pElement->PostSourceChange();

            for (auto _pChild : _pElement->GetChildren())
            {
                UpdateMouseWithinToFalse(_pChild);
            }

            _pElement->EndDefer(_Cooike);
        }

        void __fastcall NativeWindowHost::OnSize(UINT _uWidth, UINT _uHeight)
        {
            LastRenderSize.width = _uWidth;
            LastRenderSize.height = _uHeight;

            intptr_t _Cooike;
            pHost->StartDefer(&_Cooike);

            pHost->SetWidth(_uWidth);
            pHost->SetHeight(_uHeight);

            pHost->EndDefer(_Cooike);
        }
        

    } // namespace MegaUI
} // namespace YY
