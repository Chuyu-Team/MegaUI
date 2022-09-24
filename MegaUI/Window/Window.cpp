#include "pch.h"

#include "Window.h"
#include "../core/ControlInfoImp.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        Window::Window()
            : hWnd(nullptr)
            , pHost(nullptr)
            , pRender(nullptr)
            , LastRenderSize {}
        {
        }

        Window::~Window()
        {
            if (pRender)
                HDelete(pRender);
        }

        EXTERN_C extern IMAGE_DOS_HEADER __ImageBase;

        HRESULT __MEGA_UI_API Window::Initialize(HWND _hWndParent, HICON _hIcon, int _dX, int _dY, DWORD _fExStyle, DWORD _fStyle, UINT _nOptions)
        {
            WNDCLASSEXW wcex;

            // Register host window class, if needed
            wcex.cbSize = sizeof(wcex);


            if (!GetClassInfoExW((HINSTANCE)&__ImageBase, L"MegaUI", &wcex))
            {
                ZeroMemory(&wcex, sizeof(wcex));

                wcex.cbSize = sizeof(wcex);
                wcex.style = CS_GLOBALCLASS;
                wcex.hInstance = (HINSTANCE)&__ImageBase;
                wcex.hIcon = _hIcon;
                wcex.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
                wcex.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
                wcex.lpszClassName = L"MegaUI";
                wcex.lpfnWndProc = Window::WndProc;

                if (RegisterClassExW(&wcex) == 0)
                    return E_UNEXPECTED;
            }

            int32_t _iWidth = CW_USEDEFAULT;
            int32_t _iHeight = CW_USEDEFAULT;
            uString _szTitle;

            if (pHost)
            {
                _iWidth = pHost->GetWidth();
                if (_iWidth < 0)
                    _iWidth = CW_USEDEFAULT;

                _iHeight = pHost->GetHeight();
                if (_iHeight < 0)
                    _iHeight = CW_USEDEFAULT;

                _szTitle = pHost->GetTitle();

                if (pHost->GetVisible())
                    _fStyle |= WS_VISIBLE;
                else
                    _fStyle &= ~WS_VISIBLE;
            }

            hWnd = CreateWindowExW(
                _fExStyle,
                L"MegaUI",
                _szTitle,
                _fStyle | WS_CLIPCHILDREN,
                _dX, _dY, _iWidth, _iHeight,
                _hWndParent,
                0,
                (HINSTANCE)&__ImageBase,
                this);

            if (!hWnd)
                return E_UNEXPECTED;
            
            //SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)this);
            //SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)Window::WndProc);

            // If top-level, initialize keyboard cue state, start all hidden
            if (!_hWndParent)
                SendMessage(hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);

            return S_OK;
        }

        HRESULT __MEGA_UI_API Window::SetHost(WindowElement* _pHost)
        {
            if (_pHost == nullptr || _pHost->pWindow)
                return E_INVALIDARG;

            // 已经初始化了。
            if (pHost)
            {
                pHost->OnUnHosted(this);
                pHost = nullptr;
            }

            pHost = _pHost;
            pHost->OnHosted(this);
            /*intptr_t _Cooike;
            pHost->StartDefer(&_Cooike);

            pHost->SetWidth(LastRenderSize.width);
            pHost->SetHeight(LastRenderSize.height);

            pHost->EndDefer(_Cooike);*/

            return S_OK;
        }

        UINT __MEGA_UI_API Window::AsyncDestroyMsg()
        {
            static UINT g_AsyncDestroyMsg = 0;

            if (g_AsyncDestroyMsg == 0)
            {
                g_AsyncDestroyMsg = ::RegisterWindowMessageW(L"MegaUIAsyncDestroy");
            }

            return g_AsyncDestroyMsg;
        }

        void __MEGA_UI_API Window::DestroyWindow()
        {
            if (hWnd)
                ::PostMessageW(hWnd, AsyncDestroyMsg(), 0, 0);
        }

        bool __MEGA_UI_API Window::IsMinimized() const
        {
            return hWnd && (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE) != 0;
        }

        void __MEGA_UI_API Window::ShowWindow(int _iCmdShow)
        {
            if (hWnd)
                ::ShowWindow(hWnd, _iCmdShow);
        }

        void __MEGA_UI_API Window::InvalidateRect(const Rect* _pRect)
        {
            if (hWnd)
                ::InvalidateRect(hWnd, _pRect, FALSE);
        }

        HRESULT __MEGA_UI_API Window::PostDelayedDestroyElement(Element* _pElement)
        {
            if (!_pElement)
                return E_INVALIDARG;

            // 目标关联的窗口不一致
            if (_pElement->pWindow != this)
                return E_INVALIDARG;

            if (!DelayedDestroyList.EmplacePtr(_pElement))
                return E_OUTOFMEMORY;

            return S_OK;
        }

        void __MEGA_UI_API Window::HandleVisiblePropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _pOldValue, const Value& _NewValue)
        {
            if (_eIndicies == PropertyIndicies::PI_Computed && hWnd)
            {
                const auto dwOldStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
                auto dwNewStyle = dwOldStyle;

                if (_NewValue.GetBool())
                {
                    dwNewStyle |= WS_VISIBLE;
                }
                else
                {
                    dwNewStyle &= ~WS_VISIBLE;
                }
                
                if (dwNewStyle != dwOldStyle)
                    SetWindowLongPtrW(hWnd, GWL_STYLE, dwNewStyle);
            }
        }

        LRESULT Window::WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
        {
            if (_uMsg == Window::AsyncDestroyMsg())
            {
                ::DestroyWindow(_hWnd);
                return 0;
            }

            Window* _pNativeWindow = nullptr;

            if (_uMsg == WM_CREATE)
            {
                auto _pInfo = (CREATESTRUCTW*)_lParam;
                if (_pInfo && _pInfo->lpCreateParams)
                {
                    _pNativeWindow = (Window*)_pInfo->lpCreateParams;
                    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, (LONG_PTR)_pNativeWindow);

                    //Rect Client2;
                    //::GetWindowRect(_hWnd, &Client2);

                    Rect Client;
                    ::GetClientRect(_hWnd, &Client);
                    //intptr_t _Cooike;
                    //_pNativeWindow->StartDefer(&_Cooike);
                    //_pNativeWindow->SetX(_pInfo->x);
                    //_pNativeWindow->SetY(_pInfo->y);
                    _pNativeWindow->OnSize(Client.GetWidth(), Client.GetHeight());
                }
            }
            else
            {
                _pNativeWindow = (Window*)GetWindowLongPtrW(_hWnd, GWLP_USERDATA);
            }

            if (_pNativeWindow)
                return _pNativeWindow->CurrentWndProc(_hWnd, _uMsg, _wParam, _lParam);

            return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
        }

        LRESULT Window::CurrentWndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
        {
            switch (_uMsg)
            {
            case WM_CLOSE:
                DestroyWindow();
                return 0;
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                SetWindowLongPtrW(_hWnd, GWLP_USERDATA, NULL);
                hWnd = NULL;
                HDelete(this);
                break;
            case WM_STYLECHANGED:
                if (GWL_STYLE == _wParam && _lParam)
                {
                    auto _pStyles = (STYLESTRUCT*)_lParam;
                    UpdateStyles(_pStyles->styleOld, _pStyles->styleNew);
                }
                break;
            case WM_WINDOWPOSCHANGED:
                if (pHost)
                {
                    auto _pWindowPos = (WINDOWPOS*)_lParam;
                    if (_pWindowPos->flags & SWP_SHOWWINDOW)
                        pHost->SetVisible(true);
                    else if (_pWindowPos->flags & SWP_HIDEWINDOW)
                        pHost->SetVisible(false);
                }
                break;
            case WM_ERASEBKGND:
                // MegaUI 自己负责背景擦除
                return 1;
                break;
            case WM_PAINT:
                OnPaint();
                ValidateRect(_hWnd, NULL);
                return 0;
                break;
            case WM_SIZE:
                // 非最小化时不刷新 Host大小
                if (!IsMinimized())
                {
                    OnSize(LOWORD(_lParam), HIWORD(_lParam));
                }
                break;
            case WM_MOUSEMOVE:
                if (pHost && _wParam != MK_LBUTTON)
                {
                    Rect _Bounds(0, 0, LastRenderSize.width, LastRenderSize.height);
                    UpdateMouseWithin(pHost, _Bounds, _Bounds, POINT {LOWORD(_lParam), HIWORD(_lParam)});
                }

                if ((fTrackMouse & TME_LEAVE) == 0)
                {
                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.dwHoverTime = HOVER_DEFAULT;
                    tme.hwndTrack = _hWnd;
                    if (TrackMouseEvent(&tme))
                    {
                        fTrackMouse |= TME_LEAVE;
                    }
                }
                break;
            case WM_MOUSELEAVE:
                fTrackMouse &= ~TME_LEAVE;
                if (pHost)
                    UpdateMouseWithinToFalse(pHost);
                break;
            default:
                break;
            }

            return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
        }

        HRESULT __MEGA_UI_API Window::OnPaint()
        {
            if (!pHost)
                return S_FALSE;

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

        HRESULT __MEGA_UI_API Window::PaintElement(Render* _pRender, Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentPaintRect)
        {
            if (_pRender == nullptr || _pElement == nullptr)
                return E_INVALIDARG;

            // 不显示就不用绘制
            if (!_pElement->GetVisible())
                return S_FALSE;
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

        void __MEGA_UI_API Window::OnSize(UINT _uWidth, UINT _uHeight)
        {
            LastRenderSize.width = _uWidth;
            LastRenderSize.height = _uHeight;

            if (pHost)
            {
                intptr_t _Cooike;
                pHost->StartDefer(&_Cooike);

                pHost->SetWidth(_uWidth);
                pHost->SetHeight(_uHeight);

                pHost->EndDefer(_Cooike);
            }
        }

        void __MEGA_UI_API Window::UpdateMouseWithin(Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentVisibleBounds, const POINT& _ptPoint)
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

                        _pElement->PreSourceChange(Element::g_ControlInfoData.MouseWithinProp, PropertyIndicies::PI_Local, Value::GetBoolFalse(), Value::GetBoolTrue());
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

        void __MEGA_UI_API Window::UpdateMouseWithinToFalse(Element* _pElement)
        {
            if (!_pElement->IsMouseWithin())
                return;

            intptr_t _Cooike = 0;
            _pElement->StartDefer(&_Cooike);

            _pElement->PreSourceChange(Element::g_ControlInfoData.MouseWithinProp, PropertyIndicies::PI_Local, Value::GetBoolTrue(), Value::GetBoolFalse());
            _pElement->bLocMouseWithin = FALSE;
            _pElement->PostSourceChange();

            for (auto _pChild : _pElement->GetChildren())
            {
                UpdateMouseWithinToFalse(_pChild);
            }

            _pElement->EndDefer(_Cooike);
        }

        void __MEGA_UI_API Window::ClearDelayedDestroyList()
        {
            auto _TmpList = std::move(DelayedDestroyList);

            for (auto pItem : _TmpList)
            {
                pItem->Destroy(false);
            }
        }

        void __MEGA_UI_API Window::UpdateStyles(uint32_t _uOld, uint32_t _uNew)
        {
            if (!pHost)
                return;

            auto _uStyleDiff = _uOld ^ _uNew;
            if (_uStyleDiff & WS_VISIBLE)
                pHost->SetVisible(_uNew & WS_VISIBLE);
        }


    } // namespace MegaUI
} // namespace YY
