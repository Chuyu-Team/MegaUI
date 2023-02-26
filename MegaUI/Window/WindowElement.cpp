#include "pch.h"
#include "WindowElement.h"
#include "MegaUI/core/ControlInfoImp.h"
#include "Window.h"

#include <MegaUI/Accessibility/UIAutomation/WindowElementAccessibleProviderImp.h>

#pragma warning(disable : 28251)

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

        HRESULT WindowElement::GetAccessibleProvider(ElementAccessibleProvider** _ppAccessibleProvider)
        {
            if (!_ppAccessibleProvider)
                return E_INVALIDARG;
            *_ppAccessibleProvider = nullptr;

            if (!pAccessibleProvider)
            {
                pAccessibleProvider = new (std::nothrow) WindowElementAccessibleProvider(this, ThreadTaskRunner::GetCurrentThreadTaskRunner());
                if (!pAccessibleProvider)
                    return E_OUTOFMEMORY;
            }

            pAccessibleProvider->AddRef();
            *_ppAccessibleProvider = pAccessibleProvider;
            return S_OK;
        }

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
