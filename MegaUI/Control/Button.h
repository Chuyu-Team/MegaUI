#pragma once
#include <MegaUI/core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
    // clang-format off
	//     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_BUTTON_PROPERTY_TABLE(_APPLY) 

    // clang-format on

        class Button : public Element
        {
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(Button, Element, ControlInfoImp<Button>, 0u, _MEGA_UI_BUTTON_PROPERTY_TABLE);

        public:
            Button();

            ~Button();

            virtual HRESULT __YYAPI DefaultAction() override;

        protected:
            virtual bool __YYAPI OnKeyDown(const KeyboardEvent& _KeyEvent) override;

            virtual bool __YYAPI OnKeyUp(const KeyboardEvent& _KeyEvent) override;
        };
    }
} // namespace YY
