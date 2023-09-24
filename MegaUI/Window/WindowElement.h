#pragma once

#include <Base/YY.h>
#include <MegaUI/Core/Element.h>
#include <Base/Containers/Array.h>

#pragma pack(push, __YY_PACKING)
/*
WindowElement 是Window与Element之间的纽带。
Window 是真实的窗口，用于承载 Element
*/

namespace YY
{
    namespace MegaUI
    {
#define _MEGA_UI_WINDOW_ELEMENT_PROPERTY_TABLE(_APPLY) \
    _APPLY(Title, PF_Normal, 0, &Value::CreateEmptyString, nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::uString)

        class WindowElement : public Element
        {
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(WindowElement, Element, ControlInfoImp<WindowElement>, 0u, _MEGA_UI_WINDOW_ELEMENT_PROPERTY_TABLE);
        protected:

        public:
            WindowElement() = default;

            HRESULT __YYAPI SetTitle(uString _szTitle);

            uString __YYAPI GetTitle();

            virtual HRESULT __YYAPI GetAccessibleProvider(_Outptr_ ElementAccessibleProvider** _ppAccessibleProvider) override;

        protected:
            virtual bool __YYAPI OnVisiblePropChanged(_In_ OnPropertyChangedHandleData* _pHandle) override;

            virtual bool __YYAPI OnEnabledPropChanged(_In_ OnPropertyChangedHandleData* _pHandle) override;
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
