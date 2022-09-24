#include "pch.h"
#include "WindowElement.h"
#include "../core/ControlInfoImp.h"
#include "Window.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        _APPLY_MEGA_UI_STATIC_CONTROL_INFO(WindowElement, _MEGA_UI_WINDOW_ELEMENT_PROPERTY_TABLE);

        HRESULT __MEGA_UI_API WindowElement::SetTitle(uString _szTitle)
        {
            auto _TitleValue = Value::CreateString(_szTitle);
            if (_TitleValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(WindowElement::g_ControlInfoData.TitleProp, PropertyIndicies::PI_Local, _TitleValue);
        }
        
        uString __MEGA_UI_API WindowElement::GetTitle()
        {
            auto _TitleValue = GetValue(WindowElement::g_ControlInfoData.TitleProp);

            return _TitleValue.GetString();
        }
        
        void __MEGA_UI_API WindowElement::OnVisiblePropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _pOldValue, const Value& _NewValue)
        {
            // pWindow 的Host是 自己才会调用 HandleVisiblePropertyChanged
            if (pWindow && GetParent() == nullptr)
                pWindow->HandleVisiblePropertyChanged(_Prop, _eIndicies, _pOldValue, _NewValue);
            else
                Element::OnVisiblePropertyChanged(_Prop, _eIndicies, _pOldValue, _NewValue);
        }
    } // namespace MegaUI
} // namespace YY
