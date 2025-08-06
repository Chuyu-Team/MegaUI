#pragma once

#include <combaseapi.h>
#include <UIAutomationClient.h>

#include <YY/Base/YY.h>

#include <MegaUI/core/Element.h>
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        constexpr auto UIA_FirstEventId = UIA_ToolTipOpenedEventId;
        constexpr auto UIA_LastEventId = UIA_ActiveTextPositionChangedEventId;

        constexpr auto UIA_FirstPropertyId = UIA_RuntimeIdPropertyId;
        constexpr auto UIA_LastPropertyId = UIA_IsDialogPropertyId;

        struct RectangleData
        {
            Element* pElem;
            //4
            Rect OldBoundingRectangle;
            //0x14
            Rect NewBoundingRectangle;
            //0x24
            bool bOldOffScreen;
            bool bNewOffScreen;
        };

        struct ChildrenChangeData
        {
            Element* pElem;
            // pElem 即将要移除的孩子 RuntimeId列表
            Array<RuntimeId, AllocPolicy::SOO> RemoveChildrenRuntimeIds;
            Array<Element*, AllocPolicy::SOO> AddChildrenArray;
        };

        struct ChildrenVisibleChangeData
        {
            Element* pElem;
            Array<Element*, AllocPolicy::SOO> Children;
        };

        struct AccessibleThreadData
        {
            YY::Array<Element*, AllocPolicy::SOO> VisibleChange;
            YY::Array<RectangleData, AllocPolicy::SOO> RectangleChange;
            YY::Array<ChildrenChangeData, AllocPolicy::SOO> ChildrenChange;
        };
        
        class AccessibleEventManager
        {
        public:
            AccessibleEventManager() = delete;
            
            static HRESULT __YYAPI AdviseEventAdded(EVENTID _iEventId, SAFEARRAY* _pPropertyIds);

            static HRESULT __YYAPI AdviseEventRemoved(EVENTID _iEventId, SAFEARRAY* _pPropertyIds);
            
            /// <summary>
            /// 检测指定属性是否再订阅列表中。
            /// </summary>
            /// <param name="_iPropertyId"></param>
            /// <returns></returns>
            static bool __YYAPI IsPropertyIdSubscribed(
                _In_range_(UIA_FirstPropertyId, UIA_LastPropertyId) PROPERTYID _iPropertyId);

            static bool __YYAPI IsEventIdRegistered(
                _In_range_(UIA_FirstEventId, UIA_LastEventId) EVENTID _iEventId);

            /// <summary>
            /// 将属性更改暂存。注意，使用CommitPropertyChanges可以将暂存更改应用到UIA。
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
                _In_ Value _OldValue
                );
            
            static HRESULT __YYAPI NotifyPropertyChanged(
                _In_ Element* _pElem,
                _In_ const PropertyInfo& _Prop,
                _In_ PropertyIndicies _eIndicies,
                _In_ Value _OldValue,
                _In_ Value _NewValue
                );

            /// <summary>
            /// 将NotifyPropertyChanging暂存更改应用到UIA。
            /// </summary>
            /// <param name="pElem"></param>
            /// <returns></returns>
            static HRESULT __YYAPI CommitPropertyChanges(_In_ Element* _pElem);
            
            static _Ret_notnull_ AccessibleThreadData* GetAccessibleThreadData();

            static HRESULT __YYAPI AddRectangleChange(_In_ Element* _pElem, _In_ bool _bViewSize, _In_ bool _bOffscreen);

            static HRESULT __YYAPI AddVisibleChange(_In_ Element* _pElem);

        private:

            static HRESULT __YYAPI RaiseGeometryEvents();

            static HRESULT __YYAPI RaiseVisibilityEvents();

            // Child信息变更应用
            static HRESULT __YYAPI RaiseStructureEvents();

            static HRESULT __YYAPI HandleChildrenChanged(_In_ Element* _pElem, ElementList _OldChildren, ElementList _NewChildren);
        };
    }
} // namespace YY

#pragma pack(pop)
