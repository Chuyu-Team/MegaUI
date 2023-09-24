#include "pch.h"

#include "Window.h"

#include <MegaUI/Core/ControlInfoImp.h>
#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawAsyncCommandContext.h>

#ifdef _WIN32
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>
#endif

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {
        thread_local size_t s_uWindowCount = 0;

        Element* Window::g_pLastKeyboardFocusedElement = nullptr;

        Window::Window(int32_t _DefaultDpi)
            : 
#ifdef _WIN32
            hWnd(nullptr)
            ,
#endif
            pHost(nullptr)
            , pDrawContextFactory(nullptr)
            , LastRenderSize {}
            , fTrackMouse(0u)
            , bCapture(false)
            , pLastMouseFocusedElement(nullptr)
            , pLastPressedElement(nullptr)
            , iDpi(_DefaultDpi ? _DefaultDpi : 96)
        {
        }

        Window::~Window()
        {
            pDrawContext = nullptr;
        }

#ifdef _WIN32
        EXTERN_C extern IMAGE_DOS_HEADER __ImageBase;

        HRESULT Window::Initialize(
            HWND _hWndParent,
            HICON _hIcon,
            int _dX,
            int _dY,
            DWORD _fExStyle,
            DWORD _fStyle,
            UINT _nOptions,
            DrawContextFactory* _pDrawContextFactory
            )
        {
            if (!_pDrawContextFactory)
                return E_INVALIDARG;

            pDrawContextFactory = _pDrawContextFactory;

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
                wcex.hbrBackground = NULL;
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

                if (pHost->IsVisible())
                    _fStyle |= WS_VISIBLE;
                else
                    _fStyle &= ~WS_VISIBLE;

                if (pHost->IsEnabled())
                    _fStyle &= ~WS_DISABLED;
                else
                    _fStyle |= WS_DISABLED;
            }

            if (pDrawContextFactory->IsMicrosoftCompositionEngineSupport())
            {
                _fExStyle |= WS_EX_NOREDIRECTIONBITMAP;
            }

            hWnd = CreateWindowExW(
                _fExStyle /*| WS_EX_NOREDIRECTIONBITMAP*/,
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
            //if (!_hWndParent)
             //   SendMessage(hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);

            ++s_uWindowCount;
            return S_OK;
        }
#endif

        HRESULT Window::SetHost(WindowElement* _pHost)
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

        WindowElement* Window::GetHost()
        {
            return pHost;
        }

#ifdef _WIN32
        UINT Window::AsyncDestroyMsg()
        {
            static UINT g_AsyncDestroyMsg = 0;

            if (g_AsyncDestroyMsg == 0)
            {
                g_AsyncDestroyMsg = ::RegisterWindowMessageW(L"MegaUIAsyncDestroy");
            }

            return g_AsyncDestroyMsg;
        }
#endif
#ifdef _WIN32
        void Window::DestroyWindow()
        {
            if (hWnd)
                ::PostMessageW(hWnd, AsyncDestroyMsg(), 0, 0);
        }
#endif

#ifdef _WIN32
        bool Window::IsMinimized() const
        {
            return hWnd && (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE) != 0;
        }
#endif

#ifdef _WIN32
        bool Window::CanMinimize() const
        {
            return hWnd && (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZEBOX) != 0;
        }
#endif

#ifdef _WIN32
        bool Window::IsTopmost() const
        {
            return hWnd && (GetWindowLongPtrW(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        }
#endif
#ifdef _WIN32
        bool Window::CanMaximize() const
        {
            return hWnd && (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MAXIMIZEBOX) != 0;
        }
#endif

#ifdef _WIN32
        void Window::ShowWindow(int _iCmdShow)
        {
            if (hWnd)
                ::ShowWindow(hWnd, _iCmdShow);
        }
#endif

#ifdef _WIN32
        void Window::InvalidateRect(const Rect* _pRect)
        {
            if (hWnd)
            {
                if (_pRect)
                {
                    if (_pRect->IsEmpty())
                        return;

                    RECT _oInvalidateRect = {_pRect->Left, _pRect->Top, _pRect->Right, _pRect->Bottom};

                    ::InvalidateRect(hWnd, &_oInvalidateRect, FALSE);
                }
                else
                {
                    ::InvalidateRect(hWnd, nullptr, FALSE);
                }
            }
        }
#endif

        HRESULT Window::PostDelayedDestroyElement(Element* _pElement)
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

#ifdef _WIN32
        bool Window::HandleVisiblePropChanged(OnPropertyChangedHandleData* _pHandle)
        {
            if (_pHandle->eIndicies == PropertyIndicies::PI_Computed && hWnd)
            {
                const auto dwOldStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
                auto dwNewStyle = dwOldStyle;

                if (_pHandle->NewValue.GetBool())
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

            return true;
        }

        bool Window::HandleEnabledPropChanged(OnPropertyChangedHandleData* _pHandle)
        {
            if (_pHandle->eIndicies != PropertyIndicies::PI_Specified || hWnd == NULL)
                return false;

            const auto dwOldStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
            auto dwNewStyle = dwOldStyle;

            if (_pHandle->NewValue.GetBool())
            {
                dwNewStyle &= ~WS_DISABLED;
            }
            else
            {
                dwNewStyle |= WS_DISABLED;
            }

            if (dwNewStyle != dwOldStyle)
                SetWindowLongPtrW(hWnd, GWL_STYLE, dwNewStyle);
            
            return true;
        }
#endif

        Element* Window::FindElementFromPoint(const Point& _ptPoint, uint32_t fFindMarks)
        {
            if (!pHost)
                return nullptr;
            
            if ((fFindMarks & FindVisible) && pHost->IsVisible() == false)
                return nullptr;
            
            if ((fFindMarks & FindEnable) && pHost->IsEnabled() == false)
                return nullptr;

            Rect _Bounds(Point(0, 0), LastRenderSize);
            Rect _VisibleBounds = _Bounds;
            
            if (!_VisibleBounds.PointInRect(_ptPoint))
                return nullptr;

            Element* _pLastFind = nullptr;
            Element* _pElement = pHost;

            auto _fActiveMarks = fFindMarks & FindActionMarks;
            for (;;)
            {
                if (_fActiveMarks == 0 || HasFlags(_pElement->GetActive(), ActiveStyle(_fActiveMarks)))
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

                    if ((fFindMarks & FindVisible) && _pChild->IsVisible() == false)
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
                    
                    if ((fFindMarks & FindEnable) && _pChild->IsEnabled() == false)
                        return nullptr;

                    _pElement = _pChild;
                    _Bounds = _ChildBoundsElement;
                    _VisibleBounds = _ChildVisibleBounds;
                    break;
                }
            }

            return _pLastFind;
        }

        int32_t Window::GetDpi() const
        {
            return iDpi;
        }

#ifdef _WIN32
        bool Window::IsInitialized() const
        {
            return hWnd != NULL;
        }
#endif

#ifdef _WIN32
        DrawContext* Window::GetDrawContext()
        {
            if (!pDrawContext)
            {
                UniquePtr<DrawContext> _pDrawContext;
                HRESULT _hr = pDrawContextFactory->CreateDrawTarget(hWnd, _pDrawContext.ReleaseAndGetAddressOf());
                if (FAILED(_hr))
                    throw Exception(_hr);
#if 0
                pDrawContext.Attach(_pDrawContext.Detach());
#else
                _hr = DrawAsyncCommandContext::CreateDrawTarget(std::move(_pDrawContext), pDrawContext.ReleaseAndGetAddressOf());
                // 创建异步任务如果失败，则重新恢复到同步
                if (FAILED(_hr))
                    pDrawContext.Attach(_pDrawContext.Detach());
#endif
            }

            return pDrawContext;

        }
#endif

        bool Window::SetKeyboardFocus(Element* _pElement)
        {
            if (_pElement == g_pLastKeyboardFocusedElement)
                return true;

            auto _pOldKeyboardFocusedElement = g_pLastKeyboardFocusedElement;

            intptr_t _Cooike = 0;
            HRESULT _hr = S_OK;

            if (_pElement)
            {
                // 必须在某个窗口中
                if (!_pElement->pWindow)
                    return false;

                if (_pElement->IsVisible() == false || _pElement->IsEnabled() == false || HasFlags(_pElement->GetActive(), ActiveStyle::Keyboard) == false)
                    return false;

                _pElement->StartDefer(&_Cooike);

                _hr = _pElement->SetValueInternal(Element::g_ControlInfoData.KeyboardFocusedProp, Value::CreateBoolTrue(), false);
                if (SUCCEEDED(_hr))
                {
                    g_pLastKeyboardFocusedElement = _pElement;
                    
                    // 键盘焦点为物理焦点，所以也更新逻辑焦点
                    if (_pElement->pWindow->pLastFocusedElement != _pElement)
                    {
                        auto _pOldFocusedElement = _pElement->pWindow->pLastFocusedElement;
                        _hr = _pElement->SetValueInternal(Element::g_ControlInfoData.FocusedProp, Value::CreateBoolTrue(), false);
                        if (SUCCEEDED(_hr))
                        {
                            _pElement->pWindow->pLastFocusedElement = _pElement;
                            if (_pOldFocusedElement)
                                _hr = _pOldFocusedElement->SetValueInternal(Element::g_ControlInfoData.FocusedProp, Value::CreateBoolFalse(), false);
                        }
                    }
                }
            }
            else
            {
                g_pLastKeyboardFocusedElement = nullptr;
            }
            
            if (SUCCEEDED(_hr) && _pOldKeyboardFocusedElement)
            {
                _hr = _pOldKeyboardFocusedElement->SetValueInternal(Element::g_ControlInfoData.KeyboardFocusedProp, Value::CreateBoolFalse(), false);
            }

            if (_pElement)
                _pElement->EndDefer(_Cooike);

            return SUCCEEDED(_hr);
        }

        Element* Window::GetFocus()
        {
            return pLastFocusedElement;
        }

#ifdef _WIN32
        void Window::ClientToScreen(Rect* _Bounds)
        {
            POINT _Point = {};
            ::ClientToScreen(hWnd, &_Point);

            _Bounds->Left += _Point.x;
            _Bounds->Right += _Point.x;
            _Bounds->Top += _Point.y;
            _Bounds->Bottom += _Point.y;
        }
#endif

#ifdef _WIN32
        void Window::ClientToScreen(Point* _pPoint)
        {
            POINT _Point = {};
            ::ClientToScreen(hWnd, &_Point);
            _pPoint->X += _Point.x;
            _pPoint->Y += _Point.y;
        }
#endif

#ifdef _WIN32
        void Window::ScreenToClient(Rect* _Bounds)
        {
            POINT _Point = {};
            ::ScreenToClient(hWnd, &_Point);

            _Bounds->Left += _Point.x;
            _Bounds->Right += _Point.x;
            _Bounds->Top += _Point.y;
            _Bounds->Bottom += _Point.y;
        }
#endif

#ifdef _WIN32
        void Window::ScreenToClient(Point* _pPoint)
        {

            POINT _Point = {};
            ::ScreenToClient(hWnd, &_Point);

            _pPoint->X += _Point.x;
            _pPoint->Y += _Point.y;
        }

        HWND Window::GetWnd()
        {
            return hWnd;
        }
#endif

        Element* Window::GetPressed()
        {
            return pLastPressedElement;
        }

#ifdef _WIN32
        void Window::SetPressed(Element* _pElement)
        {
            if (bCapture && _pElement == nullptr)
            {
                bCapture = false;
                ReleaseCapture();
            }

            if (pLastPressedElement == _pElement)
                return;

            if (_pElement && _pElement->GetWindow() != this)
            {
                throw Exception();
                return;
            }

            auto _pOldPressed = pLastPressedElement;
            pLastPressedElement = _pElement;

            intptr_t _Cooike;
            pHost->StartDefer(&_Cooike);

            if (pLastPressedElement)
                pLastPressedElement->SetValue(Element::g_ControlInfoData.PressedProp, Value::CreateBoolTrue());

            if (_pOldPressed)
                _pOldPressed->SetValue(Element::g_ControlInfoData.PressedProp, Value::CreateUnset());

            pHost->EndDefer(_Cooike);
        }
#endif

#ifdef _WIN32
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
                auto _pInfo = (CREATESTRUCTW*)_lParam;
                if (_pInfo && _pInfo->lpCreateParams)
                {
                    _pNativeWindow = (Window*)_pInfo->lpCreateParams;
                    _pNativeWindow->hWnd = _hWnd;
                    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, (LONG_PTR)_pNativeWindow);
                }
                EnableNonClientDpiScaling(_hWnd);
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
            case WM_CREATE:
                return OnCreate();
                break;
            case WM_CLOSE:
                DestroyWindow();
                return 0;
                break;
            case WM_DESTROY:
                pDrawContext = nullptr;
                --s_uWindowCount;  
                if (s_uWindowCount == 0)
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
            {
                RECT _oPaint;
                GetUpdateRect(_hWnd, &_oPaint, FALSE);
                ValidateRect(_hWnd, NULL);
                OnPaint(_oPaint);
                return 0;
                break;
            }
            case WM_SIZE:
                // 非最小化时不刷新 Host大小
                if (!IsMinimized())
                {
                    OnSize(LOWORD(_lParam), HIWORD(_lParam));
                }
                break;
            case WM_MOUSEMOVE:
                OnMouseMove(MAKEPOINTS(_lParam), (uint32_t)_wParam);              
                break;
            case WM_MOUSELEAVE:
                fTrackMouse &= ~TME_LEAVE;
                if (pHost && pLastMouseFocusedElement)
                    OnMouseFocusMoved(pLastMouseFocusedElement, nullptr);
                break;
            case WM_LBUTTONDOWN:
            {
                MouseEvent _Event(nullptr, Win32EventModifierToEventModifier(_wParam), MAKEPOINTS(_lParam));
                _Event.pTarget = FindElementFromPoint(_Event.pt, FindVisible | FindEnable | FindActionMouse);
                OnLeftButtonDown(_Event);
                break;
            }
            case WM_LBUTTONUP:
            {
                MouseEvent _Event(nullptr, Win32EventModifierToEventModifier(_wParam), MAKEPOINTS(_lParam));
                _Event.pTarget = FindElementFromPoint(_Event.pt, FindVisible | FindEnable | FindActionMouse);

                OnLeftButtonUp(_Event);
                break;
            }
            case WM_CAPTURECHANGED:
                if (bCapture)
                {
                    bCapture = false;
                }
                break;
            case WM_ENABLE:
                if (pHost)
                    pHost->SetEnabled(_wParam != FALSE);
                break;
            case WM_DPICHANGED:
                OnDpiChanged(LOWORD(_wParam), (Rect*)_lParam);
                break;
            case WM_UPDATEUISTATE:
                OnUpdateUiState(LOWORD(_wParam), HIWORD(_wParam));
                break;
            case WM_KEYDOWN:
                OnKeyDown(KeyboardEvent(pLastFocusedElement ? pLastFocusedElement : pHost, _wParam, _lParam));
                break;
            case WM_KEYUP:
                OnKeyUp(KeyboardEvent(pLastFocusedElement ? pLastFocusedElement : pHost, _wParam, _lParam));
                break;
            case WM_CHAR:
                OnChar(KeyboardEvent(pLastFocusedElement ? pLastFocusedElement : pHost, _wParam, _lParam, KeyboardEvent::GetEventModifier()));
                break;
            case WM_GETOBJECT:
            {
                LRESULT _lResult = 0;
                if (OnGetObject((uint32_t)_wParam, (int32_t)_lParam, &_lResult))
                    return _lResult;
                break;
            }
            default:
                break;
            }

            return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
        }
#endif

#ifdef _WIN32
        bool Window::OnCreate()
        {
            //Rect Client2;
            //::GetWindowRect(_hWnd, &Client2);
            fUIState = LOWORD(SendMessageW(hWnd, WM_QUERYUISTATE, 0, 0));

            RECT Client;
            ::GetClientRect(hWnd, &Client);
            //intptr_t _Cooike;
            //_pNativeWindow->StartDefer(&_Cooike);
            //_pNativeWindow->SetX(_pInfo->x);
            //_pNativeWindow->SetY(_pInfo->y);
            intptr_t _Cooike = 0;
            if (pHost)
                pHost->StartDefer(&_Cooike);

            OnSize(Client.right - Client.left, Client.bottom - Client.top);

            const auto _iNewDpi = GetDpiForWindow(hWnd);
            const auto _iOldDpi = GetDpi();
            if (_iNewDpi != _iOldDpi)
            {
                RECT Client2;
                ::GetWindowRect(hWnd, &Client2);
                //Client2.Left = UpdatePixel(Client2.Left, _iOldDpi, _iNewDpi);
                //Client2.Top = UpdatePixel(Client2.Top, _iOldDpi, _iNewDpi);
                Client2.right = Client2.left + (int32_t)UpdatePixel(float(Client2.right - Client2.left), _iOldDpi, _iNewDpi);
                Client2.bottom = Client2.top + (int32_t)UpdatePixel(float(Client2.bottom - Client2.top), _iOldDpi, _iNewDpi);
                MoveWindow(hWnd, Client2.left, Client2.top, Client2.right, Client2.bottom, TRUE);
                OnDpiChanged(_iNewDpi, nullptr);
            }

            if (_Cooike && pHost)
                pHost->EndDefer(_Cooike);

            return true;
        }
#endif

        HRESULT Window::OnPaint(const Rect& oPaint)
        {
            // 无效区域为空则不进行绘制。
            if (oPaint.IsEmpty())
                return S_FALSE;

            if (!pHost)
                return S_FALSE;
            // PAINTSTRUCT ps;
            // auto hDC = BeginPaint(hWnd, &ps);

            auto _pDrawContext = GetDrawContext();

            if (!_pDrawContext)
            {
                return E_UNEXPECTED;
            }
            
            _pDrawContext->SetPixelSize(LastRenderSize);

            _pDrawContext->BeginDraw(&oPaint);

            // pDrawContext->DrawLine(SolidColorBrush(Color(0xFFu, 237, 28, 36)), Point(10, 10), Point(200, 300), 10);
            // pDrawContext->FillRectangle(SolidColorBrush(Color(0xFFu, 237, 28, 36)), Rect(10, 10, 200, 300));
            
            /* Font FontInfo;
            FontInfo.iSize = 20;
            FontInfo.uWeight = 700;
            FontInfo.szFace = L"微软雅黑";
            pDrawContext->DrawString(L"123", FontInfo, Color(0xFFu, 237, 28, 36), Rect(10, 10, 200, 300), (Media::Graphics::ContentAlignStyle)0);*/
            Rect _Bounds(Point(0, 0), LastRenderSize);
            PaintElement(_pDrawContext, pHost, _Bounds, _Bounds);

            pDrawContext->EndDraw();

            return 0;
        }

        HRESULT Window::PaintElement(DrawContext* _pDrawContext, Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentPaintRect)
        {
            if (_pDrawContext == nullptr || _pElement == nullptr)
                return E_INVALIDARG;

            // 不显示就不用绘制
            if (!_pElement->IsVisible())
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
                _pDrawContext->PushAxisAlignedClip(_PaintRect);
            }

            _pElement->Paint(_pDrawContext, _BoundsElement);

            if (_bNeedClip)
                _pDrawContext->PopAxisAlignedClip();

            for (auto pItem : _pElement->GetChildren())
            {
                PaintElement(_pDrawContext, pItem, _BoundsElement, _PaintRect);
            }

            return S_OK;
        }

        void Window::OnSize(float _uWidth, float _uHeight)
        {
            LastRenderSize.Width = _uWidth;
            LastRenderSize.Height = _uHeight;

            if (pHost)
            {
                intptr_t _Cooike;
                pHost->StartDefer(&_Cooike);

                pHost->SetWidth(_uWidth);
                pHost->SetHeight(_uHeight);

                pHost->EndDefer(_Cooike);
                // D2D1.0 调用这个会发生抖动
                // DwmFlush();
            }
        }

        void Window::ClearDelayedDestroyList()
        {
            auto _TmpList = std::move(DelayedDestroyList);

            for (auto pItem : _TmpList)
            {
                pItem->Destroy(false);
            }
        }

#ifdef _WIN32
        void Window::UpdateStyles(uint32_t _uOld, uint32_t _uNew)
        {
            if (!pHost)
                return;

            auto _uStyleDiff = _uOld ^ _uNew;
            if (_uStyleDiff & WS_VISIBLE)
                pHost->SetVisible(_uNew & WS_VISIBLE);
        }
#endif

        void Window::OnDpiChanged(int32_t _iNewDPI, const Rect* _pNewRect)
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
                pHost->PreSourceChange(Element::g_ControlInfoData.DpiProp, PropertyIndicies::PI_Local, _OldValue, _NewValue);
                pHost->iLocDpi = _iNewDPI;
                pHost->PostSourceChange();
            }

            UpdateDpi(pHost, _OldValue, _NewValue);
            pHost->EndDefer(_Cooike);
        }
        
        HRESULT Window::UpdateDpi(Element* _pElement, Value _OldValue, const Value& _NewValue)
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
                    pChild->PreSourceChange(Element::g_ControlInfoData.DpiProp, PropertyIndicies::PI_Local, _OldValue, _NewValue);
                    pChild->iLocDpi = _iNewDPI;
                    pChild->PostSourceChange();
                }

                auto _hr = UpdateDpi(pChild, _OldValue, _NewValue);
                if (FAILED(_hr))
                    return _hr;
            }

            return S_OK;
        }
        
#ifdef _WIN32
        void Window::OnUpdateUiState(uint16_t _eType, uint16_t _fState)
        {
            auto _fOldUIState = fUIState;

            switch (_eType)
            {
            case UIS_SET:
                fUIState |= _fState;
                break;
            case UIS_CLEAR:
                fUIState &= ~_fState;
                break;
            default:
                break;
            }
        }
#endif

#ifdef _WIN32
        void Window::OnMouseMove(Point _MousePoint, uint32_t _fFlags)
        {
            if ((_fFlags & MK_LBUTTON) == 0)
            {
                if (pHost)
                {
                    auto _pNewMouseElement = FindElementFromPoint(_MousePoint, FindVisible | FindEnable | FindActionMouse);

                    if (_pNewMouseElement != pLastMouseFocusedElement)
                        OnMouseFocusMoved(pLastMouseFocusedElement, _pNewMouseElement);                        

                    if ((fTrackMouse & TME_LEAVE) == 0)
                    {
                        if (_pNewMouseElement)
                        {
                            TRACKMOUSEEVENT tme;
                            tme.cbSize = sizeof(tme);
                            tme.dwFlags = TME_LEAVE;
                            tme.dwHoverTime = HOVER_DEFAULT;
                            tme.hwndTrack = hWnd;
                            if (TrackMouseEvent(&tme))
                            {
                                fTrackMouse |= TME_LEAVE;
                            }
                        }
                    }
                }
            }
        }
#endif

        void Window::OnMouseFocusMoved(Element* _pFrom, Element* _pTo)
        {
            if (_pFrom == _pTo)
                return;

            pLastMouseFocusedElement = _pTo;

            intptr_t _Cooike = 0;

            pHost->StartDefer(&_Cooike);
            
            if (_pTo)
            {
                _pTo->SetValueInternal(Element::g_ControlInfoData.MouseFocusedProp, Value::CreateBoolTrue());
            }

            if (_pFrom)
            {
                _pFrom->SetValueInternal(Element::g_ControlInfoData.MouseFocusedProp, Value::CreateUnset());
            }

            pHost->EndDefer(_Cooike);
        }

#ifdef _WIN32
        bool Window::OnLeftButtonDown(const MouseEvent& _Event)
        {
            SetPressed(_Event.pTarget);

            if (_Event.pTarget)
            {
                SetKeyboardFocus(_Event.pTarget);

                // 启用鼠标消息跟踪
                ::SetCapture(hWnd);
                bCapture = TRUE;                

                _Event.pTarget->OnLeftButtonDown(_Event);
            }

            return true;
        }
#endif

        bool Window::OnLeftButtonUp(const MouseEvent& _Event)
        {
            SetPressed(nullptr);
            if (_Event.pTarget)
                _Event.pTarget->OnLeftButtonUp(_Event);

            return true;
        }

        bool Window::OnKeyDown(const KeyboardEvent& _KeyEvent)
        {
            if (!_KeyEvent.pTarget)
                return false;
            
            return _KeyEvent.pTarget->OnKeyDown(_KeyEvent);
        }

        bool Window::OnKeyUp(const KeyboardEvent& _KeyEvent)
        {
            if (!_KeyEvent.pTarget)
                return false;

            return _KeyEvent.pTarget->OnKeyUp(_KeyEvent);
        }

        bool Window::OnChar(const KeyboardEvent& _KeyEvent)
        {
            if (!_KeyEvent.pTarget)
                return false;

            return _KeyEvent.pTarget->OnChar(_KeyEvent);
        }

#ifdef _WIN32
        bool Window::OnGetObject(uint32_t _fFlags, int32_t _uObjectId, LRESULT* _plResult)
        {
            if (_uObjectId != UiaRootObjectId)
                return false;

            if (!pHost)
                return false;

            ElementAccessibleProvider* _pAccessibleProvider;
            auto _hr = pHost->GetAccessibleProvider(&_pAccessibleProvider);
            if (FAILED(_hr) || _pAccessibleProvider == nullptr)
                return false;

            *_plResult = UiaReturnRawElementProvider(hWnd, _fFlags, _uObjectId, static_cast<IRawElementProviderSimple*>(_pAccessibleProvider));

            return true;
        }
#endif
    } // namespace MegaUI
} // namespace YY
