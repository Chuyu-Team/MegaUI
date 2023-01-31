#pragma once

#include "MegaUI/base/MegaUITypeInt.h"
#include "MegaUI/core/Element.h"
#include "MegaUI/base/DynamicArray.h"

#pragma pack(push, __MEGA_UI_PACKING)
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

            HRESULT __MEGA_UI_API SetTitle(uString _szTitle);

            uString __MEGA_UI_API GetTitle();

        protected:
            virtual bool __MEGA_UI_API OnVisiblePropChanged(_In_ OnPropertyChangedHandleData* _pHandle) override;

            virtual bool __MEGA_UI_API OnEnabledPropChanged(_In_ OnPropertyChangedHandleData* _pHandle) override;
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
