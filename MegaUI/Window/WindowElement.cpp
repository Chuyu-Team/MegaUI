#include "pch.h"
#include <MegaUI/Window/WindowElement.h>

#include <MegaUI/Core/ControlInfoImp.h>
#include <MegaUI/Window/Window.h>

#ifdef _WIN32
#include <MegaUI/Accessibility/UIAutomation/WindowElementAccessibleProviderImpl.h>
#endif

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {
        _APPLY_MEGA_UI_STATIC_CONTROL_INFO(WindowElement, _MEGA_UI_WINDOW_ELEMENT_PROPERTY_TABLE);

        HRESULT WindowElement::SetTitle(uString _szTitle)
        {
            auto _TitleValue = Value::CreateString(_szTitle);
            if (_TitleValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(WindowElement::g_ControlInfoData.TitleProp, _TitleValue);
        }
        
        uString WindowElement::GetTitle()
        {
            auto _TitleValue = GetValue(WindowElement::g_ControlInfoData.TitleProp);

            return _TitleValue.GetString();
        }

#ifdef _WIN32
        HRESULT WindowElement::GetAccessibleProvider(ElementAccessibleProvider** _ppAccessibleProvider)
        {
            if (!_ppAccessibleProvider)
                return E_INVALIDARG;
            *_ppAccessibleProvider = nullptr;

            if (!IsAccessible())
                return E_NOTIMPL;

            if (!pAccessibleProvider)
            {
                pAccessibleProvider = new (std::nothrow) WindowElementAccessibleProvider(this, ThreadTaskRunner::GetCurrent());
                if (!pAccessibleProvider)
                    return E_OUTOFMEMORY;
            }

            pAccessibleProvider->AddRef();
            *_ppAccessibleProvider = pAccessibleProvider;
            return S_OK;
        }
#endif

        bool WindowElement::OnVisiblePropChanged(OnPropertyChangedHandleData* _pHandle)
        {
            // pWindow 的Host是 自己才会调用 HandleVisiblePropChanged
            if (pWindow && GetParent() == nullptr)
                return pWindow->HandleVisiblePropChanged(_pHandle);
            else
                return Element::OnVisiblePropChanged(_pHandle);
        }
        
        bool  WindowElement::OnEnabledPropChanged(OnPropertyChangedHandleData* _pHandle)
        {
            if (pWindow && GetParent() == nullptr)
                return pWindow->HandleEnabledPropChanged(_pHandle);
            else
                return Element::OnEnabledPropChanged(_pHandle);
        }
    } // namespace MegaUI
} // namespace YY
