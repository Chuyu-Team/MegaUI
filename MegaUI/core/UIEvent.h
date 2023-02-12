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


        enum class EventModifier
        {
            None = 0x00000000,
            // ��ߵ� Ctrl ����
            LeftControl = 0x00000001,
            // �ұߵ� Ctrl ����
            RightControl = 0x00000002,
            // Ctrl ����
            Control = LeftControl | RightControl,

            LeftShift = 0x00000004,
            RightShift = 0x00000008,
            Shift = LeftShift | RightShift,

            LeftAlt = 0x00000010,
            RightAlt = 0x00000020,
            Alt = LeftAlt | RightAlt,

            // ������
            LeftButton = 0x00000040,
            // ����Ҽ�
            RightButton = 0x00000080,
            // ����м�
            MiddleButton = 0x00000100,
        };

        YY_APPLY_ENUM_CALSS_BIT_OPERATOR(EventModifier);

        struct BaseEvent
        {
            // �����¼���Element
            Element* pTarget;
            EventId eCode;
            // EventModifier λ���
            EventModifier fModifiers;

            static EventModifier __YYAPI GetEventModifier()
            {
                EventModifier _fModifiers = EventModifier::None;
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

            KeyboardEvent(Element* _pTarget, WPARAM _wParam, LPARAM _lParam, EventModifier _fModifiers = EventModifier::None)
                : BaseEvent {_pTarget, EventId::KeyboardEvent, _fModifiers}
                , vKey(LOWORD(_wParam))
                , uRepeatCount(LOWORD(_lParam))
                , fFlags(HIWORD(_lParam))
            {
            }
        };
        
        enum class NavigatingStyle
        {
            // �߼��ϵģ����û�д˱�־λ���ʾ �����
            Logical = 0x00000001u,
            // ��ǰ�����û�д˱�־λ���ʾ ���
            Forward = 0x00000002u,
            // ��ֱ�������û�д˱�־λ���ʾ ˮƽ����
            Vertical = 0x00000004u,
            // ��Եģ����û�д˱�־λ���ʾ ���Ե�
            Relative = 0x00000008u,
        };

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
        
        inline constexpr bool HasFlags(NavigatingType _eLeft, NavigatingStyle _eRight)
        {
            using _LeftType = std::_Underlying_type<NavigatingType>::type;
            using _RigthType = std::_Underlying_type<NavigatingStyle>::type;
            return (_LeftType)_eLeft & (_RigthType)_eRight;
        }

        struct KeyboardNavigateEvent : public BaseEvent
        {
            NavigatingType Navigate;

            KeyboardNavigateEvent(Element* _pTarget, NavigatingType _Navigate)
                : BaseEvent {_pTarget, EventId::KeyboardNavigateEvent, EventModifier::None}
                , Navigate(_Navigate)
            {
            }
        };
    }
} // namespace YY

#pragma pack(pop)
