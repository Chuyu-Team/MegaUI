#pragma once
#include <MegaUI/Accessibility/UIAutomation/PatternProviderImpl.h>
#include <MegaUI/Window/WindowElement.h>
#include <MegaUI/Window/Window.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        template<>
        class PatternProvider<WindowElement, ITransformProvider2>
            : public ComUnknowImpl<PatternProvider<WindowElement, ITransformProvider2>, ITransformProvider2>
        {
        protected:
            ThreadTaskRunner TaskRunner;
            WindowElement* pElement;

        public:
            PatternProvider(WindowElement* _pElement, ThreadTaskRunner _TaskRunner)
                : TaskRunner(std::move(_TaskRunner))
                , pElement(_pElement)
            {
            }
            
            __YY_BEGIN_COM_QUERY_MAP(PatternProvider)
                __YY_QUERY_ENTRY(IUnknown)
                __YY_QUERY_ENTRY(ITransformProvider)
                __YY_QUERY_ENTRY(ITransformProvider2)
            __YY_END_COM_QUERY_MAP();

            // ITransformProvider

            virtual HRESULT STDMETHODCALLTYPE Move(
                /* [in] */ double _X,
                /* [in] */ double _Y) override
            {
                BOOL _bCanMove;
                auto _hr = get_CanMove(&_bCanMove);
                if (FAILED(_hr))
                    return _hr;

                if (!_bCanMove)
                    return E_NOTIMPL;

                auto _bRet = ::SetWindowPos(
                    pElement->GetWindow()->GetWnd(),
                    NULL,
                    _X,
                    _Y,
                    0,
                    0,
                    SWP_NOZORDER | SWP_NOSIZE);

                if (_bRet)
                    return S_OK;

                return __HRESULT_FROM_WIN32(GetLastError());
            }

            virtual HRESULT STDMETHODCALLTYPE Resize(
                /* [in] */ double _Width,
                /* [in] */ double _Height) override
            {
                BOOL _bCanResize;
                auto _hr = get_CanResize(&_bCanResize);
                if (FAILED(_hr))
                    return _hr;
                if (!_bCanResize)
                    return E_NOTIMPL;
                
                auto _bRet = ::SetWindowPos(
                    pElement->GetWindow()->GetWnd(),
                    NULL,
                    0,
                    0,
                    _Width,
                    _Height,
                    SWP_NOZORDER | SWP_NOMOVE);

                if (_bRet)
                    return S_OK;

                return __HRESULT_FROM_WIN32(GetLastError());
            }

            virtual HRESULT STDMETHODCALLTYPE Rotate(
                /* [in] */ double degrees) override
            {
                return E_NOTIMPL;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanMove(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;

                *_pRetVal = TRUE;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanResize(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = TRUE;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanRotate(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = FALSE;
                return S_OK;
            }

            // ITransformProvider2

            virtual HRESULT STDMETHODCALLTYPE Zoom(
                /* [in] */ double _Zoom) override
            {
                return E_NOTIMPL;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanZoom(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;
                *_pRetVal = FALSE;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ZoomLevel(
                /* [retval][out] */ __RPC__out double* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = 100;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ZoomMinimum(
                /* [retval][out] */ __RPC__out double* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = 100;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ZoomMaximum(
                /* [retval][out] */ __RPC__out double* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = 100;
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE ZoomByUnit(
                /* [in] */ enum ZoomUnit zoomUnit) override
            {
                return E_NOTIMPL;
            }
        };

        template<>
        class PatternProvider<WindowElement, IWindowProvider>
            : public ComUnknowImpl<PatternProvider<WindowElement, IWindowProvider>, IWindowProvider>
        {
        protected:
            ThreadTaskRunner TaskRunner;
            WindowElement* pElement;
            
        public:
            PatternProvider(WindowElement* _pElement, ThreadTaskRunner _TaskRunner)
                : TaskRunner(std::move(_TaskRunner))
                , pElement(_pElement)
            {
            }
            
            // IWindowProvider

            virtual HRESULT STDMETHODCALLTYPE SetVisualState(
                /* [in] */ enum WindowVisualState _eState) override
            {
                switch (_eState)
                {
                case WindowVisualState_Normal:
                {
                    if (!pElement->GetWindow()->CanMaximize())
                        return E_NOTIMPL;

                    pElement->GetWindow()->ShowWindow(SW_NORMAL);
                    return S_OK;
                }
                case WindowVisualState_Maximized:
                {
                    if (!pElement->GetWindow()->CanMaximize())
                        return E_NOTIMPL;

                    pElement->GetWindow()->ShowWindow(SW_MAXIMIZE);
                    return S_OK;
                }
                case WindowVisualState_Minimized:
                {
                    if (!pElement->GetWindow()->CanMinimize())
                        return E_NOTIMPL;

                    pElement->GetWindow()->ShowWindow(SW_MINIMIZE);
                    return S_OK;
                }
                default:
                    return E_NOTIMPL;
                    break;
                }
            }

            virtual HRESULT STDMETHODCALLTYPE Close() override
            {
                ::PostMessageW(pElement->GetWindow()->GetWnd(), WM_CLOSE, 0, 0);
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE WaitForInputIdle(
                /* [in] */ int _iMilliseconds,
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = TRUE;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanMaximize(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;
                *_pRetVal = pElement->GetWindow()->CanMaximize();
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanMinimize(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;
                *_pRetVal = pElement->GetWindow()->CanMinimize();
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsModal(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = FALSE;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WindowVisualState(
                /* [retval][out] */ __RPC__out enum WindowVisualState* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;
                const auto _fStyle = GetWindowLongPtrW(pElement->GetWindow()->GetWnd(), GWL_STYLE);
                if (_fStyle & WS_MINIMIZE)
                {
                    *_pRetVal = WindowVisualState::WindowVisualState_Minimized;
                }
                else if (_fStyle & WS_MAXIMIZE)
                {
                    *_pRetVal = WindowVisualState::WindowVisualState_Maximized;
                }
                else
                {
                    *_pRetVal = WindowVisualState::WindowVisualState_Normal;
                }

                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_WindowInteractionState(
                /* [retval][out] */ __RPC__out enum WindowInteractionState* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = WindowInteractionState::WindowInteractionState_Running;
                return S_OK;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsTopmost(
                /* [retval][out] */ __RPC__out BOOL* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_NOINTERFACE;

                *_pRetVal = pElement->GetWindow()->IsTopmost();
                return S_OK;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
