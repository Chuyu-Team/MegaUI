#pragma once
#include <MegaUI/base/MegaUITypeInt.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;

        enum class EventId
        {
            KeyboardEvent,
            KeyboardNavigateEvent,
        };


        namespace EventModifier
        {
            // 左边的 Ctrl 按键
            constexpr uint32_t LeftControl = 0x00000001;
            // 右边的 Ctrl 按键
            constexpr uint32_t RightControl = 0x00000002;
            // Ctrl 按键
            constexpr uint32_t Control = LeftControl | RightControl;

            constexpr uint32_t LeftShift = 0x00000004;
            constexpr uint32_t RightShift = 0x00000008;
            constexpr uint32_t Shift = LeftShift | RightShift;

            constexpr uint32_t LeftAlt = 0x00000010;
            constexpr uint32_t RightAlt = 0x00000020;
            constexpr uint32_t Alt = LeftAlt | RightAlt;

            // 鼠标左键
            constexpr uint32_t LeftButton = 0x00000040;
            // 鼠标右键
            constexpr uint32_t RightButton = 0x00000080;
            // 鼠标中键
            constexpr uint32_t MiddleButton = 0x00000100;
        }

        struct BaseEvent
        {
            // 产生事件的Element
            Element* pTarget;
            EventId eCode;
            // EventModifier 位组合
            uint32_t fModifiers;

            static uint32_t __YYAPI GetEventModifier()
            {
                uint32_t _fModifiers = 0;
                BYTE bKeys[256];
                if (GetKeyboardState(bKeys))
                {
                    if (bKeys[VK_LBUTTON] & 0x80)
                        _fModifiers |= EventModifier::LeftButton;
                    if (bKeys[VK_RBUTTON] & 0x80)
                        _fModifiers |= EventModifier::RightButton;
                    if (bKeys[VK_MBUTTON] & 0x80)
                        _fModifiers |= EventModifier::MiddleButton;
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

                return _fModifiers;
            }
        };

        struct KeyboardEvent : public BaseEvent
        {
            u16char_t vKey;
            uint16_t uRepeatCount;
            uint16_t fFlags;

            KeyboardEvent(Element* _pTarget, WPARAM _wParam, LPARAM _lParam, uint32_t _fModifiers = 0)
                : BaseEvent {_pTarget, EventId::KeyboardEvent, _fModifiers}
                , vKey(LOWORD(_wParam))
                , uRepeatCount(LOWORD(_lParam))
                , fFlags(HIWORD(_lParam))
            {
            }
        };
        
        namespace NavigatingMarks
        {
            // 逻辑上的，如果没有此标志位则表示 定向的
            constexpr auto Logical = 0x00000001u;
            // 向前，如果没有此标志位则表示 向后
            constexpr auto Forward = 0x00000002u;
            // 垂直方向，如果没有此标志位则表示 水平方向
            constexpr auto Vertical = 0x00000004u;
            // 相对的，如果没有此标志位则表示 绝对的
            constexpr auto Relative = 0x00000008u;
        } // namespace NavigatingMarks

        enum class NavigatingType
        {
            First = NavigatingMarks::Forward | NavigatingMarks::Logical,
            Last = NavigatingMarks::Logical,
            Up = NavigatingMarks::Relative | NavigatingMarks::Vertical,
            Down = NavigatingMarks::Relative | NavigatingMarks::Vertical | NavigatingMarks::Forward,
            Left = NavigatingMarks::Relative,
            Right = NavigatingMarks::Relative | NavigatingMarks::Forward,
            Next = NavigatingMarks::Relative | NavigatingMarks::Forward | NavigatingMarks::Logical,
            Previous = NavigatingMarks::Relative | NavigatingMarks::Logical,
        };

        struct KeyboardNavigateEvent : public BaseEvent
        {
            NavigatingType Navigate;

            KeyboardNavigateEvent(Element* _pTarget, NavigatingType _Navigate)
                : BaseEvent {_pTarget, EventId::KeyboardNavigateEvent, 0}
                , Navigate(_Navigate)
            {
            }
        };
    }
} // namespace YY

#pragma pack(pop)
