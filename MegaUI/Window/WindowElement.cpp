#include "pch.h"
#include "WindowElement.h"
#include "../core/ClassInfoBase.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        _APPLY_MEGA_UI_STATIC_CALSS_INFO(WindowElement, _MEGA_UI_WINDOW_ELEMENT_PROPERTY_TABLE);

        HRESULT __MEGA_UI_API WindowElement::SetTitle(uString _szTitle)
        {
            auto _TitleValue = Value::CreateString(_szTitle);
            if (_TitleValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(WindowElement::g_ClassInfoData.TitleProp, PropertyIndicies::PI_Local, _TitleValue);
        }
        
        uString __MEGA_UI_API WindowElement::GetTitle()
        {
            auto _TitleValue = GetValue(WindowElement::g_ClassInfoData.TitleProp);

            return _TitleValue.GetString();
        }
    } // namespace MegaUI
} // namespace YY
