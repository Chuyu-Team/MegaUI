#include "pch.h"
#include "AccessibleEventManager.h"

#include <Base/Containers/Array.h>
#include <MegaUI/base/ComPtr.h>
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        static uint32_t g_uRegisteredEventIdCount;
        static uint32_t g_uRegisteredEventIdMap[UIA_LastEventId - UIA_FirstEventId + 1];

        static uint32_t g_uSubscribedPropertyIdCount;
        static uint32_t g_uSubscribedPropertyIdMap[UIA_LastPropertyId - UIA_FirstPropertyId + 1];


        static uint32_t __YYAPI EventIdToIndex(EVENTID _iEventId)
        {
            return uint32_t(_iEventId - UIA_FirstEventId);
        }

        static uint32_t __YYAPI PropertyIdToIndex(PROPERTYID _iPropertyId)
        {
            return uint32_t(_iPropertyId - UIA_FirstPropertyId);
        }

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

        struct AccessibleThreadData
        {
            YY::Array<RectangleData, AllocPolicy::SOO> RectangleChange;
        };

        thread_local AccessibleThreadData AccessibleData;

        HRESULT AccessibleEventManager::AdviseEventAdded(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            if (EventIdToIndex(_iEventId) >= std::size(g_uRegisteredEventIdMap))
                return E_INVALIDARG;

            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (!_pPropertyIds)
                    return E_POINTER;
            }
            
            // todo 需要加锁

            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (_pPropertyIds->cDims != 1)
                {
                    return E_INVALIDARG;
                }

                VARTYPE vt;
                auto _hr = SafeArrayGetVartype(_pPropertyIds, &vt);
                if (FAILED(_hr))
                    return _hr;

                if (vt != VT_I4)
                    return E_INVALIDARG;

                int32_t* _pData;
                _hr = SafeArrayAccessData(_pPropertyIds, (void**)&_pData);
                if (FAILED(_hr))
                    return _hr;

                for (size_t i = 0; i != _pPropertyIds->rgsabound->cElements; ++i)
                {
                    const auto _uIndex = PropertyIdToIndex(_pData[i]);
                    if (_uIndex < std::size(g_uSubscribedPropertyIdMap))
                    {
                        ++g_uSubscribedPropertyIdMap[_uIndex];
                        ++g_uSubscribedPropertyIdCount;
                    }
                }

                SafeArrayUnaccessData(_pPropertyIds);
            }
            else
            {
                ++g_uRegisteredEventIdMap[_iEventId - UIA_FirstEventId];
                ++g_uRegisteredEventIdCount;
            }

            return S_OK;
        }

        HRESULT AccessibleEventManager::AdviseEventRemoved(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            if (EventIdToIndex(_iEventId) >= std::size(g_uRegisteredEventIdMap))
                return E_INVALIDARG;

            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (!_pPropertyIds)
                    return E_POINTER;
            }
            
            // todo 需要加锁
            
            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (_pPropertyIds->cDims != 1)
                {
                    return E_INVALIDARG;
                }

                VARTYPE vt;
                auto _hr = SafeArrayGetVartype(_pPropertyIds, &vt);
                if (FAILED(_hr))
                    return _hr;

                if (vt != VT_I4)
                {
                    return E_INVALIDARG;
                }

                int32_t* _pData;
                _hr = SafeArrayAccessData(_pPropertyIds, (void**)&_pData);
                if (FAILED(_hr))
                    return _hr;

                for (size_t i = 0; i != _pPropertyIds->rgsabound->cElements; ++i)
                {
                    const auto _uIndex = PropertyIdToIndex(_pData[i]);
                    if (_uIndex < std::size(g_uSubscribedPropertyIdMap))
                    {
                        if (g_uSubscribedPropertyIdMap[_uIndex])
                        {
                            --g_uSubscribedPropertyIdMap[_uIndex];
                            --g_uSubscribedPropertyIdCount;
                        }
                    }
                }

                SafeArrayUnaccessData(_pPropertyIds);
            }
            else
            {
                auto& _uMapItem = g_uRegisteredEventIdMap[_iEventId - UIA_FirstEventId];
                if (_uMapItem)
                {
                    --_uMapItem;
                    --g_uRegisteredEventIdCount;
                }
            }

            return S_OK;
        }

        bool AccessibleEventManager::IsPropertyIdSubscribed(PROPERTYID _iPropertyId)
        {
            const auto _uIndex = PropertyIdToIndex(_iPropertyId);
            if (_uIndex >= std::size(g_uSubscribedPropertyIdMap))
                return false;

            return g_uSubscribedPropertyIdMap[_uIndex] != 0;
        }

        bool AccessibleEventManager::IsEventIdRegistered(EVENTID _iEventId)
        {
            const auto _uIndex = EventIdToIndex(_iEventId);
            if (_uIndex >= std::size(g_uRegisteredEventIdMap))
                return false;

            return g_uRegisteredEventIdMap[_uIndex] != 0;
        }

        HRESULT AccessibleEventManager::NotifyPropertyChanging(Element* _pElem, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value _OldValue, Value _NewValue)
        {
            // 未监听任何更改，退出
            if (g_uRegisteredEventIdCount == 0)
                return S_FALSE;

            if (_Prop == Element::g_ControlInfoData.LocationProp
                || _Prop == Element::g_ControlInfoData.ExtentProp)
            {
                // 通知 BoundingRectangleProperty更改
                const auto _bViewSize = IsPropertyIdSubscribed(UIA_BoundingRectanglePropertyId)
                    || IsPropertyIdSubscribed(UIA_ScrollHorizontalViewSizePropertyId)
                    || IsPropertyIdSubscribed(UIA_ScrollVerticalViewSizePropertyId);

                const auto _bOffscreen = IsPropertyIdSubscribed(UIA_IsOffscreenPropertyId);

                if (_bViewSize || _bOffscreen)
                {
                    for (auto& _Item : AccessibleData.RectangleChange)
                    {
                        // 已经添加
                        if (_Item.pElem == _pElem)
                            return S_FALSE;
                    }

                    AddRectangleChange(_pElem, _bViewSize, _bOffscreen);
                }
            }

            return S_OK;
        }

        HRESULT AccessibleEventManager::NotifyPropertyChanged(Element* _pElem, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value _OldValue, Value _NewValue)
        {
            __DuiWarningMassage("todo");
            return E_NOTIMPL;
        }

        HRESULT AccessibleEventManager::CommitPropertyChanges(Element* _pElem)
        {
            if (g_uRegisteredEventIdCount == 0)
                return S_FALSE;

            AccessibleData.RectangleChange.Clear();

            return S_FALSE;
        }

        HRESULT AccessibleEventManager::AddRectangleChange(Element* _pElem, bool _bViewSize, bool _bOffscreen)
        {
            // 无障碍已经禁用，本节点无需处理
            if (_pElem->IsAccessible())
            {
                RectangleData _RectangleChange = {};
                _RectangleChange.pElem = _pElem;
                
                YY::MegaUI::ComPtr<ElementAccessibleProvider> _pAccessibleProvider;
                auto _hr = _pElem->GetAccessibleProvider(&_pAccessibleProvider);
                if (FAILED(_hr))
                    return _hr;

                if (_bViewSize)
                {
                    _RectangleChange.OldBoundingRectangle = _pAccessibleProvider->GetBoundingRectangle();
                }

                if (_bOffscreen)
                {
                    // 检测是否滚出屏幕
                    _RectangleChange.bOldOffScreen = _pAccessibleProvider->IsOffscreen();
                }

                if (!AccessibleData.RectangleChange.EmplacePtr(std::move(_RectangleChange)))
                    return E_OUTOFMEMORY;
            }

            auto _Children = _pElem->GetChildren();
            for (auto _pItem : _Children)
            {
                auto _hr = AddRectangleChange(_pItem, _bViewSize, _bOffscreen);
                if (FAILED(_hr))
                    return _hr;
            }

            return S_OK;
        }

    }
}
