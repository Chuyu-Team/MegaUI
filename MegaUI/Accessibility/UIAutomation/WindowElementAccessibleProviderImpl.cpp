#include "pch.h"
#include "WindowElementAccessibleProviderImpl.h"

#include <MegaUI/Window/Window.h>
#include <MegaUI/Accessibility/UIAutomation/WindowElementPatternProviderImpl.h>

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        WindowElementAccessibleProvider::WindowElementAccessibleProvider(Element* _pElement, ThreadTaskRunner _TaskRunner)
            : ElementAccessibleProvider(_pElement, _TaskRunner)
        {
        }

        HRESULT WindowElementAccessibleProvider::QueryInterface(REFIID _riid, void** _ppvObject)
        {
            if (!_ppvObject)
                return E_INVALIDARG;
            *_ppvObject = nullptr;

            if (_riid == __uuidof(IRawElementProviderFragmentRoot))
            {
                AddRef();
                *_ppvObject = static_cast<IRawElementProviderFragmentRoot*>(this);
                return S_OK;
            }

            return ElementAccessibleProvider::QueryInterface(_riid, _ppvObject);
        }

        ULONG WindowElementAccessibleProvider::AddRef()
        {
            return ElementAccessibleProvider::AddRef();
        }

        ULONG WindowElementAccessibleProvider::Release()
        {
            return ElementAccessibleProvider::Release();
        }

        HRESULT WindowElementAccessibleProvider::GetPatternProvider(PATTERNID _iPatternId, IUnknown** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;

            *_pRetVal = nullptr;

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    if (_iPatternId == UIA_TransformPatternId || _iPatternId == UIA_TransformPattern2Id)
                    {
                        _hr = PatternProvider<WindowElement, ITransformProvider2>::Create(this, _pRetVal, &PatternProviderCache[UIA_TransformPatternId - UIA_FirstPatternId]);
                        return;
                    }
                    else if (_iPatternId == UIA_WindowPatternId)
                    {
                        _hr = PatternProvider<WindowElement, ITransformProvider2>::Create(this, _pRetVal, &PatternProviderCache[UIA_WindowPatternId - UIA_FirstPatternId]);
                        return;
                    }

                    _hr = ElementAccessibleProvider::GetPatternProvider(_iPatternId, _pRetVal);
                    return;
                });

            return _hr;
        }

        HRESULT WindowElementAccessibleProvider::ElementProviderFromPoint(double _X, double _Y, IRawElementProviderFragment** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            if (isnan(_X) || isnan(_Y))
                return E_INVALIDARG;

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    Point _Point((float)_X, (float)_Y);
                    pElement->GetWindow()->ScreenToClient(&_Point);
                    auto _pFind = pElement->GetWindow()->FindElementFromPoint(_Point);
                    if (_pFind)
                    {
                        if (!_pFind->IsAccessible())
                            _pFind = GetVisibleAccessibleParent(_pFind);

                        if (_pFind)
                        {
                            ElementAccessibleProvider* _pAccessibleProvider;
                            _hr = _pFind->GetAccessibleProvider(&_pAccessibleProvider);
                            if (FAILED(_hr))
                                return _hr;

                            if (!_pAccessibleProvider)
                                return _hr = E_UNEXPECTED;

                            *_pRetVal = static_cast<IRawElementProviderFragment*>(_pAccessibleProvider);
                            return _hr = S_OK;
                        }
                    }

                    return _hr = E_NOT_SET;
                });

            return _hr;
        }
        
        HRESULT WindowElementAccessibleProvider::GetFocus(IRawElementProviderFragment** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    auto _pFocus = pElement->GetWindow()->GetFocus();
                    if (!_pFocus)
                        return _hr = S_FALSE;

                    ElementAccessibleProvider* _pAccessibleProvider;
                    _hr = _pFocus->GetAccessibleProvider(&_pAccessibleProvider);
                    if (FAILED(_hr))
                        return _hr;

                    if (!_pAccessibleProvider)
                        return _hr = E_UNEXPECTED;

                    *_pRetVal = static_cast<IRawElementProviderFragment*>(_pAccessibleProvider);
                    return _hr = S_OK;
                });       

            return _hr;
        }
    } // namespace MegaUI
} // namespace YY
