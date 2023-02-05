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
            // ��ߵ� Ctrl ����
            constexpr uint32_t LeftControl = 0x00000001;
            // �ұߵ� Ctrl ����
            constexpr uint32_t RightControl = 0x00000002;
            // Ctrl ����
            constexpr uint32_t Control = LeftControl | RightControl;

            constexpr uint32_t LeftShift = 0x00000004;
            constexpr uint32_t RightShift = 0x00000008;
            constexpr uint32_t Shift = LeftShift | RightShift;

            constexpr uint32_t LeftAlt = 0x00000010;
            constexpr uint32_t RightAlt = 0x00000020;
            constexpr uint32_t Alt = LeftAlt | RightAlt;

            // ������
            constexpr uint32_t LeftButton = 0x00000040;
            // ����Ҽ�
            constexpr uint32_t RightButton = 0x00000080;
            // ����м�
            constexpr uint32_t MiddleButton = 0x00000100;
        }

        struct BaseEvent
        {
            // �����¼���Element
            Element* pTarget;
            EventId eCode;
            // EventModifier λ���
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
            // �߼��ϵģ����û�д˱�־λ���ʾ �����
            constexpr auto Logical = 0x00000001u;
            // ��ǰ�����û�д˱�־λ���ʾ ���
            constexpr auto Forward = 0x00000002u;
            // ��ֱ�������û�д˱�־λ���ʾ ˮƽ����
            constexpr auto Vertical = 0x00000004u;
            // ��Եģ����û�д˱�־λ���ʾ ���Ե�
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
