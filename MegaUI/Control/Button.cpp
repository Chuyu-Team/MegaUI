﻿#include "pch.h"
#include "Button.h"

#include <MegaUI/core/ControlInfoImp.h>
#include <MegaUI/Window/Window.h>

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        _APPLY_MEGA_UI_STATIC_CONTROL_INFO(Button, _MEGA_UI_BUTTON_PROPERTY_TABLE);

        Button::Button()
        {
        }

        Button::~Button()
        {
        }

        HRESULT Button::DefaultAction()
        {
            ClickEvent _ClickEvent(this, EventModifier::None);
            OnClick(_ClickEvent);
            return S_OK;
        }

        bool Button::OnKeyDown(const KeyboardEvent& _KeyEvent)
        {
            if (HasFlags(_KeyEvent.fModifiers, EventModifier::LeftButton) == false)
            {
                if (_KeyEvent.vKey == VK_SPACE)
                {
                    GetWindow()->SetPressed(this);
                    return true;
                }
                else if (_KeyEvent.vKey == VK_RETURN)
                {
                    GetWindow()->SetPressed(nullptr);
                    ClickEvent _ClickEvent(_KeyEvent.pTarget, _KeyEvent.fModifiers);
                    return OnClick(_ClickEvent);
                }
                else if (_KeyEvent.vKey == VK_ESCAPE)
                {
                    GetWindow()->SetPressed(nullptr);
                    return false;
                }
            }

            return Element::OnKeyDown(_KeyEvent);
        }

        bool Button::OnKeyUp(const KeyboardEvent& _KeyEvent)
        {
            if (_KeyEvent.vKey == VK_SPACE)
            {
                if (GetWindow()->GetPressed() == this)
                {
                    GetWindow()->SetPressed(nullptr);
                    ClickEvent _ClickEvent(_KeyEvent.pTarget, _KeyEvent.fModifiers);
                    return OnClick(_ClickEvent);
                }
            }

            return Element::OnKeyUp(_KeyEvent);
        }


    } // namespace MegaUI
} // namespace YY
