#include "pch.h"

#include "Window.h"
#include "../core/ControlInfoImp.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        Window::Window(int32_t _DefaultDpi)
            : hWnd(nullptr)
            , pHost(nullptr)
            , pRender(nullptr)
            , LastRenderSize {}
            , fTrackMouse(0u)
            , bCapture(false)
            , pLastFocusedElement(nullptr)
            , pLastPressedElement(nullptr)
            , iDpi(_DefaultDpi ? _DefaultDpi : 96)
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
                wcex.lpfnWndProc = Window::StaticWndProc;

                if (RegisterClassExW(&wcex) == 0)
                    return E_UNEXPECTED;
            }

            int32_t _iWidth = CW_USEDEFAULT;
            int32_t _iHeight = CW_USEDEFAULT;
            uString _szTitle;

            if (pHost)
            {
                _iWidth = (int32_t)pHost->GetWidth();
                if (_iWidth < 0)
                    _iWidth = CW_USEDEFAULT;

                _iHeight = (int32_t)pHost->GetHeight();
                if (_iHeight < 0)
                    _iHeight = CW_USEDEFAULT;

                _szTitle = pHost->GetTitle();

                if (pHost->GetVisible())
                    _fStyle |= WS_VISIBLE;
                else
                    _fStyle &= ~WS_VISIBLE;

                if (pHost->GetEnabled())
                    _fStyle &= ~WS_DISABLED;
                else
                    _fStyle |= WS_DISABLED;
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
                ::InvalidateRect(hWnd, nullptr, FALSE);
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

        void __MEGA_UI_API Window::HandleVisiblePropChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _pOldValue, const Value& _NewValue)
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

        void __MEGA_UI_API Window::HandleEnabledPropChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _pOldValue, const Value& _NewValue)
        {
            if (_eIndicies != PropertyIndicies::PI_Specified || hWnd == NULL)
                return;

            const auto dwOldStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
            auto dwNewStyle = dwOldStyle;

            if (_NewValue.GetBool())
            {
                dwNewStyle &= ~WS_DISABLED;
            }
            else
            {
                dwNewStyle |= WS_DISABLED;
            }

            if (dwNewStyle != dwOldStyle)
                SetWindowLongPtrW(hWnd, GWL_STYLE, dwNewStyle);
            
        }

        Element* __MEGA_UI_API Window::FindElementFromPoint(const Point& _ptPoint, uint32_t fActiveMarks)
        {
            if (!pHost)
                return nullptr;

            if (!pHost->GetVisible())
                return nullptr;

            Rect _Bounds(0, 0, (float)LastRenderSize.width, (float)LastRenderSize.height);
            Rect _VisibleBounds = _Bounds;
            
            if (!_VisibleBounds.PointInRect(_ptPoint))
                return nullptr;

            Element* _pLastFind = nullptr;
            Element* _pElement = pHost;

            for (;;)
            {
                if (fActiveMarks == 0 || (_pElement->GetActive() & fActiveMarks))
                    _pLastFind = _pElement;

                auto _Children = _pElement->GetChildren();
                auto _uSize = _Children.GetSize();
                auto _pData = _Children.GetData();

                for (;;)
                {
                    if (_uSize == 0)
                        return _pLastFind;

                    --_uSize;

                    auto _pChild = _pData[_uSize];

                    if (!_pChild->GetVisible())
                        continue;

                    auto _Location = _pChild->GetLocation();
                    auto _Extent = _pChild->GetExtent();

                    Rect _ChildBoundsElement;
                    _ChildBoundsElement.Left = _Bounds.Left + _Location.X;
                    _ChildBoundsElement.Right = _ChildBoundsElement.Left + _Extent.Width;

                    _ChildBoundsElement.Top = _Bounds.Top + _Location.Y;
                    _ChildBoundsElement.Bottom = _ChildBoundsElement.Top + _Extent.Height;

                    Rect _ChildVisibleBounds = _VisibleBounds & _ChildBoundsElement;
                    if (_ChildVisibleBounds.IsEmpty())
                    {
                        continue;
                    }

                    if (!_ChildVisibleBounds.PointInRect(_ptPoint))
                    {
                        continue;
                    }

                    _pElement = _pChild;
                    _Bounds = _ChildBoundsElement;
                    _VisibleBounds = _ChildVisibleBounds;
                    break;
                }
            }

            return _pLastFind;
        }

        int32_t __MEGA_UI_API Window::GetDpi() const
        {
            return iDpi;
        }

        bool __MEGA_UI_API Window::IsInitialized() const
        {
            return hWnd != NULL;
        }

        Render* __MEGA_UI_API Window::GetRender()
        {
            if (!pRender)
            {
                auto _hr = CreateRender(hWnd, &pRender);
                if (FAILED(_hr))
                    throw Exception(_hr);
            }

            return pRender;
        }

        LRESULT Window::StaticWndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
        {
            if (_uMsg == Window::AsyncDestroyMsg())
            {
                ::DestroyWindow(_hWnd);
                return 0;
            }

            Window* _pNativeWindow = nullptr;
            if (_uMsg == WM_NCCREATE)
            {
                EnableNonClientDpiScaling(_hWnd);
            }
            else if (_uMsg == WM_CREATE)
            {
                auto _pInfo = (CREATESTRUCTW*)_lParam;
                if (_pInfo && _pInfo->lpCreateParams)
                {
                    _pNativeWindow = (Window*)_pInfo->lpCreateParams;
                    _pNativeWindow->hWnd = _hWnd;
                    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, (LONG_PTR)_pNativeWindow);

                    //Rect Client2;
                    //::GetWindowRect(_hWnd, &Client2);

                    RECT Client;
                    ::GetClientRect(_hWnd, &Client);
                    //intptr_t _Cooike;
                    //_pNativeWindow->StartDefer(&_Cooike);
                    //_pNativeWindow->SetX(_pInfo->x);
                    //_pNativeWindow->SetY(_pInfo->y);
                    intptr_t _Cooike = 0;
                    if (_pNativeWindow->pHost)
                        _pNativeWindow->pHost->StartDefer(&_Cooike);

                    _pNativeWindow->OnSize(Client.right - Client.left, Client.bottom - Client.top);
                    
                    const auto _iNewDpi = GetDpiForWindow(_hWnd);
                    const auto _iOldDpi = _pNativeWindow->GetDpi();
                    if (_iNewDpi != _iOldDpi)
                    {
                        RECT Client2;
                        ::GetWindowRect(_hWnd, &Client2);
                        //Client2.Left = UpdatePixel(Client2.Left, _iOldDpi, _iNewDpi);
                        //Client2.Top = UpdatePixel(Client2.Top, _iOldDpi, _iNewDpi);
                        Client2.right = Client2.left + (int32_t)UpdatePixel(float(Client2.right - Client2.left), _iOldDpi, _iNewDpi);
                        Client2.bottom = Client2.top + (int32_t)UpdatePixel(float(Client2.bottom - Client2.top), _iOldDpi, _iNewDpi);
                        MoveWindow(_hWnd, Client2.left, Client2.top, Client2.right, Client2.bottom, TRUE);
                        _pNativeWindow->OnDpiChanged(_iNewDpi, nullptr);
                    }

                    if (_Cooike && _pNativeWindow->pHost)
                        _pNativeWindow->pHost->EndDefer(_Cooike);
                }
            }
            else
            {
                _pNativeWindow = (Window*)GetWindowLongPtrW(_hWnd, GWLP_USERDATA);
            }

            if (_pNativeWindow)
                return _pNativeWindow->WndProc(_hWnd, _uMsg, _wParam, _lParam);

            return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
        }

        LRESULT Window::WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
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
            case WM_DISPLAYCHANGE:
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
                    InvalidateRect(nullptr);
                }
                break;
            case WM_MOUSEMOVE:
                if (pHost && _wParam != MK_LBUTTON)
                {
                    Rect _Bounds(0, 0, (float)LastRenderSize.width, (float)LastRenderSize.height);
                    UpdateMouseWithin(pHost, _Bounds, _Bounds, Point((float)LOWORD(_lParam), (float)HIWORD(_lParam)));
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
                    pHost->UpdateMouseWithinToFalse();
                break;
            case WM_LBUTTONDOWN:
            {
                auto _pOldPressedElement = pLastPressedElement;
                auto _pOldFocusedElement = pLastFocusedElement;

                auto _pFind = FindElementFromPoint(Point((float)LOWORD(_lParam), (float)HIWORD(_lParam)), Active::Mouse);
                if (_pFind && _pFind->GetEnabled())
                {
                    pLastPressedElement = _pFind;
                    pLastFocusedElement = _pFind;

                    // 启用鼠标消息跟踪
                    ::SetCapture(hWnd);
                    bCapture = TRUE;
                }
                else
                {
                    pLastPressedElement = nullptr;
                }
                
                auto _pNewPressedElement = pLastPressedElement;
                auto _pNewFocusedElement = pLastFocusedElement;

                if (_pOldFocusedElement || _pNewFocusedElement)
                {
                    intptr_t _Cooike = 0;

                    if (pHost)
                        pHost->StartDefer(&_Cooike);

                    if (_pOldFocusedElement && _pNewFocusedElement != _pOldFocusedElement)
                    {
                        _pOldFocusedElement->PreSourceChange(Element::g_ControlInfoData.MouseFocusedProp, PropertyIndicies::PI_Local, Value::CreateBoolTrue(), Value::CreateUnset());
                        _pOldFocusedElement->bHasLocMouseFocused = FALSE;
                        _pOldFocusedElement->PostSourceChange();
                    }

                    if (_pNewFocusedElement)
                    {
                        if (_pNewFocusedElement->bHasLocMouseFocused == FALSE || _pNewFocusedElement->bLocMouseFocused == FALSE)
                        {
                            _pNewFocusedElement->PreSourceChange(Element::g_ControlInfoData.MouseFocusedProp, PropertyIndicies::PI_Local, Value::CreateBoolFalse(), Value::CreateBoolTrue());
                            _pNewFocusedElement->bHasLocMouseFocused = TRUE;
                            _pNewFocusedElement->bLocMouseFocused = TRUE;
                            _pNewFocusedElement->PostSourceChange();
                        }
                    }
                    
                    if (pHost)
                        pHost->EndDefer(_Cooike);
                }
                break;
            }
            case WM_LBUTTONUP:
                if (bCapture)
                {
                    ReleaseCapture();
                    bCapture = false;
                }
                pLastPressedElement = nullptr;
                break;
            case WM_CAPTURECHANGED:
                bCapture = false;
                pLastPressedElement = nullptr;
                break;
            case WM_ENABLE:
                if (pHost)
                    pHost->SetEnabled(_wParam != FALSE);
                break;
            case WM_DPICHANGED:
                OnDpiChanged(LOWORD(_wParam), (Rect*)_lParam);
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

             RECT Client;
            ::GetClientRect(hWnd, &Client);

            pRender->SetPixelSize(LastRenderSize);

            Rect _NeedPaint;
            auto _hr = pRender->BeginDraw(&_NeedPaint);
            if (FAILED(_hr))
                return _hr;
            Rect _Bounds(0, 0, (float)LastRenderSize.width, (float)LastRenderSize.height);
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
                    _Location.x += _ParentBounds.Left;
                    _Location.y += _ParentBounds.Top;

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
            _BoundsElement.Left = _ParentBounds.Left + _Location.X;
            _BoundsElement.Right = _BoundsElement.Left + _Extent.Width;

            _BoundsElement.Top = _ParentBounds.Top + _Location.Y;
            _BoundsElement.Bottom = _BoundsElement.Top + _Extent.Height;
#endif
            // 如果没有交集，那么我们可以不绘制
            Rect _PaintRect = _ParentPaintRect & _BoundsElement;
            if (_PaintRect.IsEmpty())
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

                pHost->SetWidth((float)_uWidth);
                pHost->SetHeight((float)_uHeight);

                pHost->EndDefer(_Cooike);
            }
        }

        void __MEGA_UI_API Window::UpdateMouseWithin(Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentVisibleBounds, const Point& _ptPoint)
        {
            // 禁用的控件不分发鼠标消息。
            if (!_pElement->GetEnabled())
                return;

            auto _Location = _pElement->GetLocation();
            auto _Extent = _pElement->GetExtent();

            Rect _BoundsElement;
            _BoundsElement.Left = _ParentBounds.Left + _Location.X;
            _BoundsElement.Right = _BoundsElement.Left + _Extent.Width;

            _BoundsElement.Top = _ParentBounds.Top + _Location.Y;
            _BoundsElement.Bottom = _BoundsElement.Top + _Extent.Height;

            Rect _VisibleBounds = _ParentVisibleBounds & _BoundsElement;
            if (!_VisibleBounds.IsEmpty())
            {
                if (_VisibleBounds.PointInRect(_ptPoint))
                {
                    intptr_t _Cooike = 0;

                    if (!_pElement->IsMouseWithin())
                    {
                        _pElement->StartDefer(&_Cooike);

                        _pElement->PreSourceChange(Element::g_ControlInfoData.MouseWithinProp, PropertyIndicies::PI_Local, Value::CreateBoolFalse(), Value::CreateBoolTrue());
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

            _pElement->UpdateMouseWithinToFalse();
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

        void __MEGA_UI_API Window::OnDpiChanged(int32_t _iNewDPI, const Rect* _pNewRect)
        {
            if (_iNewDPI == 0 || _iNewDPI == iDpi)
                return;
            iDpi = _iNewDPI;

            //if ((pHost == nullptr || pHost->GetHighDpi()) && _pNewRect)
            //{
            //    MoveWindow(hWnd, _pNewRect->Left, _pNewRect->Top, _pNewRect->Right, _pNewRect->Bottom, TRUE);
            //}

            if (!pHost)
            {
                return;
            }

            intptr_t _Cooike = 0;
            pHost->StartDefer(&_Cooike);
            
            /*if (_pNewRect)
            {
                OnSize(_pNewRect->GetWidth(), _pNewRect->GetHeight());
            }*/

            auto _OldValue = Value::CreateInt32(pHost->GetDpi());
            auto _NewValue = Value::CreateInt32(_iNewDPI);

            if (pHost->iLocDpi != _iNewDPI)
            {
                pHost->PreSourceChange(Element::g_ControlInfoData.DPIProp, PropertyIndicies::PI_Local, _OldValue, _NewValue);
                pHost->iLocDpi = _iNewDPI;
                pHost->PostSourceChange();
            }

            UpdateDPI(pHost, _OldValue, _NewValue);
            pHost->EndDefer(_Cooike);
        }
        
        HRESULT __MEGA_UI_API Window::UpdateDPI(Element* _pElement, Value _OldValue, const Value& _NewValue)
        {
            const auto _iNewDPI = _NewValue.GetInt32();
            for (auto pChild : _pElement->GetChildren())
            {
                const auto _iOldDPI = pChild->GetDpi();
                if (_iNewDPI != _iOldDPI)
                {
                    if (_iOldDPI != _OldValue.GetInt32())
                    {
                        _OldValue = Value::CreateInt32(_iOldDPI);
                        if (_OldValue == nullptr)
                            return E_OUTOFMEMORY;
                    }
                    pChild->PreSourceChange(Element::g_ControlInfoData.DPIProp, PropertyIndicies::PI_Local, _OldValue, _NewValue);
                    pChild->iLocDpi = _iNewDPI;
                    pChild->PostSourceChange();
                }

                auto _hr = UpdateDPI(pChild, _OldValue, _NewValue);
                if (FAILED(_hr))
                    return _hr;
            }

            return S_OK;
        }
    } // namespace MegaUI
} // namespace YY
