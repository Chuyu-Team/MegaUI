#pragma once

#include <combaseapi.h>
#include <UIAutomationClient.h>

#include <Base/YY.h>

#include <MegaUI/core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        constexpr auto UIA_FirstEventId = UIA_ToolTipOpenedEventId;
        constexpr auto UIA_LastEventId = UIA_ActiveTextPositionChangedEventId;

        constexpr auto UIA_FirstPropertyId = UIA_RuntimeIdPropertyId;
        constexpr auto UIA_LastPropertyId = UIA_IsDialogPropertyId;


        class AccessibleEventManager
        {
        public:
            AccessibleEventManager() = delete;
            
            static HRESULT __YYAPI AdviseEventAdded(EVENTID _iEventId, SAFEARRAY* _pPropertyIds);

            static HRESULT __YYAPI AdviseEventRemoved(EVENTID _iEventId, SAFEARRAY* _pPropertyIds);
            
            /// <summary>
            /// ���ָ�������Ƿ��ٶ����б��С�
            /// </summary>
            /// <param name="_iPropertyId"></param>
            /// <returns></returns>
            static bool __YYAPI IsPropertyIdSubscribed(
                _In_range_(UIA_FirstPropertyId, UIA_LastPropertyId) PROPERTYID _iPropertyId);

            static bool __YYAPI IsEventIdRegistered(
                _In_range_(UIA_FirstEventId, UIA_LastEventId) EVENTID _iEventId);

            /// <summary>
            /// �����Ը����ݴ档ע�⣬ʹ��CommitPropertyChanges���Խ��ݴ����Ӧ�õ�UIA��
            /// </summary>
            /// <param name="pElem"></param>
            /// <param name="_Prop"></param>
            /// <param name="_eIndicies"></param>
            /// <param name="_OldValue"></param>
            /// <param name="_NewValue"></param>
            /// <returns></returns>
            static HRESULT __YYAPI NotifyPropertyChanging(
                _In_ Element* _pElem,
                _In_ const PropertyInfo& _Prop,
                _In_ PropertyIndicies _eIndicies,
                _In_ Value _OldValue,
                _In_ Value _NewValue
                );
            
            static HRESULT __YYAPI NotifyPropertyChanged(
                _In_ Element* _pElem,
                _In_ const PropertyInfo& _Prop,
                _In_ PropertyIndicies _eIndicies,
                _In_ Value _OldValue,
                _In_ Value _NewValue
                );

            /// <summary>
            /// ��NotifyPropertyChanging�ݴ����Ӧ�õ�UIA��
            /// </summary>
            /// <param name="pElem"></param>
            /// <returns></returns>
            static HRESULT __YYAPI CommitPropertyChanges(_In_ Element* _pElem);
            
        private:
            static HRESULT __YYAPI AddRectangleChange(_In_ Element* _pElem, _In_ bool _bViewSize, _In_ bool _bOffscreen);
        };
    }
} // namespace YY
