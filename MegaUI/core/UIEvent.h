#pragma once
#include <Base/YY.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;

        enum class EventId : uint32_t
        {
            KeyboardEvent,
            KeyboardNavigateEvent,
            MouseEvent,
            // 点击事件，一般是指鼠标左键
            ClickEvent,
            // 上下文事件，来自鼠标右键或者其他快捷键，一般行为是弹出菜单。
            ContextEvent,
        };


        enum class EventModifier
        {
            None = 0x00000000,
            // 左边的 Ctrl 按键
            LeftControl = 0x00000001,
            // 右边的 Ctrl 按键
            RightControl = 0x00000002,
            // Ctrl 按键
            Control = LeftControl | RightControl,

            LeftShift = 0x00000004,
            RightShift = 0x00000008,
            Shift = LeftShift | RightShift,

            LeftAlt = 0x00000010,
            RightAlt = 0x00000020,
            Alt = LeftAlt | RightAlt,

            // 鼠标左键
            LeftButton = 0x00000040,
            // 鼠标右键
            RightButton = 0x00000080,
            // 鼠标中键
            MiddleButton = 0x00000100,
            // 第一个 X按钮
            XButton1 = 0x00000200,
            // 第二个 X按钮
            XButton2 = 0x00000400,
        };

        YY_APPLY_ENUM_CALSS_BIT_OPERATOR(EventModifier);

        inline EventModifier __YYAPI Win32EventModifierToEventModifier(uint32_t _fWin32EventModifier)
        {
            EventModifier _Modifier = EventModifier::None;

#ifdef _WIN32
            if (_fWin32EventModifier & MK_CONTROL)
                _Modifier |= EventModifier::Control;
            
            if (_fWin32EventModifier & MK_LBUTTON)
                _Modifier |= EventModifier::LeftButton;
            
            if (_fWin32EventModifier & MK_MBUTTON)
                _Modifier |= EventModifier::MiddleButton;

            if (_fWin32EventModifier & MK_RBUTTON)
                _Modifier |= EventModifier::RightButton;

            if (_fWin32EventModifier & MK_SHIFT)
                _Modifier |= EventModifier::Shift;

            if (_fWin32EventModifier & MK_XBUTTON1)
                _Modifier |= EventModifier::XButton1;

            if (_fWin32EventModifier & MK_XBUTTON2)
                _Modifier |= EventModifier::XButton2;
#endif
            return _Modifier;
        }


        struct BaseEvent
        {
            // 产生事件的Element
            Element* pTarget;
            EventId eCode;
            // EventModifier 位组合
            EventModifier fModifiers;

            static EventModifier __YYAPI GetEventModifier()
            {
                EventModifier _fModifiers = EventModifier::None;
#ifdef _WIN32
                BYTE bKeys[256];
                if (GetKeyboardState(bKeys))
                {
                    if (bKeys[VK_LBUTTON] & 0x80)
                        _fModifiers |= EventModifier::LeftButton;
                    if (bKeys[VK_RBUTTON] & 0x80)
                        _fModifiers |= EventModifier::RightButton;
                    if (bKeys[VK_MBUTTON] & 0x80)
                        _fModifiers |= EventModifier::MiddleButton;
                    if (bKeys[VK_XBUTTON1] & 0x80)
                        _fModifiers |= EventModifier::XButton1;
                    if (bKeys[VK_XBUTTON2] & 0x80)
                        _fModifiers |= EventModifier::XButton2;
                    if (bKeys[VK_LSHIFT] & 0x80)
                        _fModifiers |= EventModifier::LeftShift;
                    if (bKeys[VK_RSHIFT] & 0x80)
                        _fModifiers |= EventModifier::RightShift;
                    if (bKeys[VK_LCONTROL] & 0x80)
                        _fModifiers |= EventModifier::LeftControl;
                    if (bKeys[VK_RCONTROL] & 0x80)
                        _fModifiers |= EventModifier::RightControl;
                    if (bKeys[VK_LMENU] & 0x80)
                        _fModifiers |= EventModifier::LeftAlt;
                    if (bKeys[VK_RMENU] & 0x80)
                        _fModifiers |= EventModifier::RightAlt;
                }
#endif
                return _fModifiers;
            }
        };

        struct KeyboardEvent : public BaseEvent
        {
            constexpr static EventId Id = EventId::KeyboardEvent;
            u16char_t vKey;
            uint16_t uRepeatCount;
            uint16_t fFlags;

#ifdef _WIN32
            KeyboardEvent(Element* _pTarget, WPARAM _wParam, LPARAM _lParam, EventModifier _fModifiers = EventModifier::None)
                : BaseEvent {_pTarget, EventId::KeyboardEvent, _fModifiers}
                , vKey(LOWORD(_wParam))
                , uRepeatCount(LOWORD(_lParam))
                , fFlags(HIWORD(_lParam))
            {
            }
#endif
        };
        
        enum class NavigatingStyle
        {
            // 逻辑上的，如果没有此标志位则表示 定向的
            Logical = 0x00000001u,
            // 向前，如果没有此标志位则表示 向后
            Forward = 0x00000002u,
            // 垂直方向，如果没有此标志位则表示 水平方向
            Vertical = 0x00000004u,
            // 相对的，如果没有此标志位则表示 绝对的
            Relative = 0x00000008u,
        };

        YY_APPLY_ENUM_CALSS_BIT_OPERATOR(NavigatingStyle);

        enum class NavigatingType
        {
            First = (int32_t)NavigatingStyle::Forward | (int32_t)NavigatingStyle::Logical,
            Last = (int32_t)NavigatingStyle::Logical,
            Up = (int32_t)NavigatingStyle::Relative | (int32_t)NavigatingStyle::Vertical,
            Down = (int32_t)NavigatingStyle::Relative | (int32_t)NavigatingStyle::Vertical | (int32_t)NavigatingStyle::Forward,
            Left = (int32_t)NavigatingStyle::Relative,
            Right = (int32_t)NavigatingStyle::Relative | (int32_t)NavigatingStyle::Forward,
            Next = (int32_t)NavigatingStyle::Relative | (int32_t)NavigatingStyle::Forward | (int32_t)NavigatingStyle::Logical,
            Previous = (int32_t)NavigatingStyle::Relative | (int32_t)NavigatingStyle::Logical,
        };
        
        
        inline constexpr NavigatingType& operator&=(NavigatingType& _eLeft, NavigatingStyle _eRight)
        {
            using _LeftType = std::underlying_type<NavigatingType>::type;
            using _RigthType = std::underlying_type<NavigatingStyle>::type;
            (_LeftType&)_eLeft &= _LeftType((_RigthType)_eRight);
            return _eLeft;
        }

        inline constexpr NavigatingType& operator^=(NavigatingType& _eLeft, NavigatingStyle _eRight)
        {
            using _LeftType = std::underlying_type<NavigatingType>::type;
            using _RigthType = std::underlying_type<NavigatingStyle>::type;
            (_LeftType&)_eLeft ^= _LeftType((_RigthType)_eRight);
            return _eLeft;
        }

        inline constexpr bool HasFlags(NavigatingType _eLeft, NavigatingStyle _eRight)
        {
            using _LeftType = std::underlying_type<NavigatingType>::type;
            using _RigthType = std::underlying_type<NavigatingStyle>::type;
            return (_LeftType)_eLeft & (_RigthType)_eRight;
        }

        struct KeyboardNavigateEvent : public BaseEvent
        {
            constexpr static EventId Id = EventId::KeyboardNavigateEvent;
            NavigatingType Navigate;

            KeyboardNavigateEvent(Element* _pTarget, NavigatingType _Navigate)
                : BaseEvent {_pTarget, EventId::KeyboardNavigateEvent, EventModifier::None}
                , Navigate(_Navigate)
            {
            }
        };

        struct MouseEvent : public BaseEvent
        {
            constexpr static EventId Id = EventId::MouseEvent;
            // 鼠标坐标
            Point pt;

            MouseEvent(Element* _pTarget, EventModifier _fEventModifier, Point _pt)
                : BaseEvent {_pTarget, EventId::MouseEvent, _fEventModifier}
                , pt(_pt)
            {
            }
        };




        struct ClickEvent : public BaseEvent
        {
            constexpr static EventId Id = EventId::ClickEvent;
            // 鼠标坐标
            Point pt;

            ClickEvent(Element* _pTarget, EventModifier _fEventModifier = EventModifier::None, Point _pt = Point {-1, -1})
                : BaseEvent {_pTarget, EventId::ClickEvent, _fEventModifier}
                , pt(_pt)
            {
            }
        };
        
        struct ContextEvent : public BaseEvent
        {
            constexpr static EventId Id = EventId::ContextEvent;
            // 鼠标坐标
            Point pt;
        };
    }
} // namespace YY

#pragma pack(pop)
