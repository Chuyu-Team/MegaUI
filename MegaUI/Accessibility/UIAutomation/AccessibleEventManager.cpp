#include "pch.h"
#include "AccessibleEventManager.h"

#include <Base/Containers/Array.h>
#include <Base/Memory/RefPtr.h>

#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

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

        static thread_local AccessibleThreadData s_AccessibleData;
        static SRWLOCK g_EventLock;

        HRESULT AccessibleEventManager::AdviseEventAdded(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            if (EventIdToIndex(_iEventId) >= std::size(g_uRegisteredEventIdMap))
                return E_INVALIDARG;

            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (!_pPropertyIds)
                    return E_POINTER;

                if (_pPropertyIds->cDims != 1)
                {
                    return E_INVALIDARG;
                }
            }
            
            HRESULT _hr = S_OK;

            AcquireSRWLockExclusive(&g_EventLock);
            do
            {
                if (_iEventId == UIA_AutomationPropertyChangedEventId)
                {
                    VARTYPE vt;
                    _hr = SafeArrayGetVartype(_pPropertyIds, &vt);
                    if (FAILED(_hr))
                        break;

                    if (vt != VT_I4)
                    {
                        _hr = E_INVALIDARG;
                        break;
                    }

                    int32_t* _pData;
                    _hr = SafeArrayAccessData(_pPropertyIds, (void**)&_pData);
                    if (FAILED(_hr))
                        break;

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
            } while (false);

            ReleaseSRWLockExclusive(&g_EventLock);

            return _hr;
        }

        HRESULT AccessibleEventManager::AdviseEventRemoved(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            if (EventIdToIndex(_iEventId) >= std::size(g_uRegisteredEventIdMap))
                return E_INVALIDARG;

            if (_iEventId == UIA_AutomationPropertyChangedEventId)
            {
                if (!_pPropertyIds)
                    return E_POINTER;

                if (_pPropertyIds->cDims != 1)
                {
                    return E_INVALIDARG;
                }
            }

            HRESULT _hr = S_OK;
            AcquireSRWLockExclusive(&g_EventLock);
            
            do
            {
                if (_iEventId == UIA_AutomationPropertyChangedEventId)
                {
                    VARTYPE vt;
                    _hr = SafeArrayGetVartype(_pPropertyIds, &vt);
                    if (FAILED(_hr))
                        break;

                    if (vt != VT_I4)
                    {
                        _hr = E_INVALIDARG;
                        break;
                    }

                    int32_t* _pData;
                    _hr = SafeArrayAccessData(_pPropertyIds, (void**)&_pData);
                    if (FAILED(_hr))
                        break;

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
            } while (false);

            ReleaseSRWLockExclusive(&g_EventLock);

            return _hr;
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

        HRESULT AccessibleEventManager::NotifyPropertyChanging(Element* _pElem, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value _OldValue)
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
                    for (auto& _Item : s_AccessibleData.RectangleChange)
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
            if (g_uRegisteredEventIdCount == 0)
                return S_FALSE;

            YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
            auto _hr = _pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
            if (FAILED(_hr))
                return _hr;

            return _pAccessibleProvider->HandlePropertyChanged(_Prop, _eIndicies, _OldValue, _NewValue);
        }

        HRESULT AccessibleEventManager::CommitPropertyChanges(Element* _pElem)
        {
            if (g_uRegisteredEventIdCount == 0)
                return S_FALSE;
            
            auto _hr = RaiseGeometryEvents();
            if (FAILED(_hr))
                return _hr;

            _hr = RaiseVisibilityEvents();
            if (FAILED(_hr))
                return _hr;

            _hr = RaiseStructureEvents();
            if (FAILED(_hr))
                return _hr;

            return S_OK;
        }

        AccessibleThreadData* AccessibleEventManager::GetAccessibleThreadData()
        {
            return &s_AccessibleData;
        }

        HRESULT AccessibleEventManager::AddRectangleChange(Element* _pElem, bool _bViewSize, bool _bOffscreen)
        {
            // 无障碍已经禁用，本节点无需处理
            if (_pElem->IsAccessible())
            {
                RectangleData _RectangleChange = {};
                _RectangleChange.pElem = _pElem;
                
                YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                auto _hr = _pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
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

                if (!s_AccessibleData.RectangleChange.EmplacePtr(std::move(_RectangleChange)))
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

        HRESULT __YYAPI AccessibleEventManager::AddVisibleChange(Element* _pElem)
        {
            if (!IsEventIdRegistered(UIA_StructureChangedEventId))
                return S_OK;

            // 如果自己或者跟节点已经添加，那么直接返回 S_OK
            // 因为这时工作是重复的。
            for (auto _pItem : s_AccessibleData.VisibleChange)
            {
                if (_pElem == _pItem)
                    return S_OK;

                if (ElementAccessibleProvider::IsAccessibleAncestor(_pElem, _pItem))
                    return S_OK;
            }

            if (!s_AccessibleData.VisibleChange.EmplacePtr(_pElem))
                return E_OUTOFMEMORY;

            return S_OK;
        }

        HRESULT AccessibleEventManager::RaiseGeometryEvents()
        {
            auto _RectangleChange = std::move(s_AccessibleData.RectangleChange);
            if (_RectangleChange.GetSize() == 0)
                return S_OK;

            const auto _bSubscribedBoundingRectangleProperty = IsPropertyIdSubscribed(UIA_BoundingRectanglePropertyId);
            const auto _bSubscribedScrollHorizontalViewSizeProperty = IsPropertyIdSubscribed(UIA_ScrollHorizontalViewSizePropertyId);
            const auto _bSubscribedScrollVerticalViewSizeProperty = IsPropertyIdSubscribed(UIA_ScrollVerticalViewSizePropertyId);

            auto const _bSubscribedBounds= _bSubscribedBoundingRectangleProperty | _bSubscribedScrollHorizontalViewSizeProperty | _bSubscribedScrollVerticalViewSizeProperty;

            const auto _bSubscribedIsOffscreenProperty = IsPropertyIdSubscribed(UIA_IsOffscreenPropertyId);

            // 如果没有监听相关更改，则后续无需处理。
            if (_bSubscribedBounds == false && _bSubscribedIsOffscreenProperty == false)
                return S_FALSE;

            size_t _uHandleCount = 0;

            // 首先确定相关属性前后是否更改，我们把没有更改的从列表删除
            for (auto& _Item : _RectangleChange)
            {
                if (!_Item.pElem)
                    continue;
                
                bool _bChanged = false;
                
                YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                auto _hr = _Item.pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                if (SUCCEEDED(_hr))
                {
                    if (_bSubscribedBounds)
                    {
                        _Item.NewBoundingRectangle = _pAccessibleProvider->GetBoundingRectangle();
                        if (_Item.OldBoundingRectangle != _Item.NewBoundingRectangle)
                            _bChanged = true;
                    }

                    if (_bSubscribedIsOffscreenProperty)
                    {
                        _Item.bNewOffScreen = _pAccessibleProvider->IsOffscreen();
                        if (_Item.bOldOffScreen != _Item.bNewOffScreen)
                            _bChanged = true;
                    }
                }

                if (!_bChanged)
                {
                    _Item.pElem = nullptr;
                    continue;
                }

                ++_uHandleCount;
            }

            if (_uHandleCount >= 25)
            {
                // 如果通知处理的数目较大，那么通过 UIA_LayoutInvalidatedEventId 统一通知
                // 统一通知前，我们先删除不必要的 UIA_LayoutInvalidatedEventId 通知。
                // 因为 UIA_LayoutInvalidatedEventId 是包含子元素的。
                for (size_t i = 1; i < _RectangleChange.GetSize(); ++i)
                {
                    auto& _Item = _RectangleChange[i];
                    if (!_Item.pElem)
                        continue;

                    // 如果前面是当前节点的父级，那么当前可以忽略
                    for (size_t j = 0; j != i; ++j)
                    {
                        auto& _Prev = _RectangleChange[i];
                        if (!_Prev.pElem)
                            continue;

                        if (ElementAccessibleProvider::IsAccessibleAncestor(_Item.pElem, _Prev.pElem))
                        {
                            _Item.pElem = nullptr;
                            break;
                        }
                    }

                    // 如果当前节点是前面元素的父级，那么之前的元素也可以删除
                    for (size_t j = 0; j != i; ++j)
                    {
                        auto& _Prev = _RectangleChange[i];
                        if (!_Prev.pElem)
                            continue;

                        if (ElementAccessibleProvider::IsAccessibleAncestor(_Prev.pElem, _Item.pElem))
                        {
                            _Prev.pElem = nullptr;
                        }
                    }
                }

                for (auto& _Item : _RectangleChange)
                {
                    if (!_Item.pElem)
                        continue;

                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _Item.pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        continue;

                    UiaRaiseAutomationEvent(_pAccessibleProvider, UIA_LayoutInvalidatedEventId);
                }
            }
            else
            {
                // 通知数目相对较少，我们挨个通知一遍
                for (auto& _Item : _RectangleChange)
                {
                    if (!_Item.pElem)
                        continue;
                    
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _Item.pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        continue;

                    if (_bSubscribedBounds)
                    {
                        if (_Item.OldBoundingRectangle != _Item.NewBoundingRectangle)
                        {
                            VARIANT _OldValue;
                            VARIANT _NewValue;
                            auto _hr1 = VariantSetUiaRect(&_OldValue, _Item.OldBoundingRectangle);
                            auto _hr2 = VariantSetUiaRect(&_NewValue, _Item.NewBoundingRectangle);
                            if (SUCCEEDED(_hr1) && SUCCEEDED(_hr2))
                            {
                                UiaRaiseAutomationPropertyChangedEvent(_pAccessibleProvider, UIA_BoundingRectanglePropertyId, _OldValue, _NewValue);
                            }
                            
                            VariantClear(&_OldValue);
                            VariantClear(&_NewValue);
                        }
                    }

                    if (_bSubscribedIsOffscreenProperty)
                    {
                        if (_Item.bOldOffScreen != _Item.bNewOffScreen)
                        {
                            VARIANT _OldValue;
                            VARIANT _NewValue;

                            VariantSetBool(&_OldValue, _Item.bOldOffScreen);
                            VariantSetBool(&_NewValue, _Item.bNewOffScreen);

                            UiaRaiseAutomationPropertyChangedEvent(_pAccessibleProvider, UIA_BoundingRectanglePropertyId, _OldValue, _NewValue);

                            VariantClear(&_OldValue);
                            VariantClear(&_NewValue);
                        }
                    }
                }
            }

            return S_OK;
        }

        HRESULT AccessibleEventManager::RaiseStructureEvents()
        {
            auto _ChildrenChange = std::move(s_AccessibleData.ChildrenChange);
            auto _uCount = _ChildrenChange.GetSize();

            if (_uCount == 0)
                return S_OK;

            // 先把同一父中的节点统一合并到顶级父。
            for (size_t i = _uCount; i > 0; )
            {
                --i;
                auto& _SrcItem = _ChildrenChange[i];
                if (!_SrcItem.pElem)
                    continue;

                auto _pSrcItemParent = _SrcItem.pElem->GetParent();
                if (!_pSrcItemParent)
                {
                    continue;
                }

                for (size_t j = 0; j < _uCount; ++j)
                {
                    if (j == i)
                        continue;

                    auto& _DstItem = _ChildrenChange[j];
                    if (!_DstItem.pElem)
                        continue;

                    for (auto _pDstChild : _DstItem.AddChildrenArray)
                    {
                        if (!_pDstChild->IsAccessible())
                            continue;

                        for (auto _pItem = _pSrcItemParent; _pItem; _pItem = _pItem->GetParent())
                        {
                            if (_pItem == _pDstChild)
                            {
                                // _pDstChild 刚好就是 _SrcItem.pElem 的父
                                // 所以 _SrcItem 就不需要再处理了。
                                _SrcItem.pElem = nullptr;
                                goto EndLoop;
                            }
                        }
                    }

                    for (auto& _Id : _DstItem.RemoveChildrenRuntimeIds)
                    {
                        for (auto _pItem = _pSrcItemParent; _pItem; _pItem = _pItem->GetParent())
                        {
                            if (uint64_t(_pItem) == _Id.pElementPtr64)
                            {
                                // _Id.pElementPtr64  刚好就是 _SrcItem.pElem 的父
                                // 因为它 _Id.pElementPtr64 即将被删除了，
                                // 所以 _SrcItem.pElem 就不需要处理了。
                                _SrcItem.pElem = nullptr;
                                goto EndLoop;
                            }
                        }
                    }
                }
            EndLoop:
                continue;
            }


            // 开始将有效的改动提交到 UIA
            for (size_t i = _uCount; i > 0;)
            {
                --i;

                auto& _SrcItem = _ChildrenChange[i];
                if (!_SrcItem.pElem)
                    continue;

                auto _pRoot = _SrcItem.pElem;
                for (; _pRoot; _pRoot = _pRoot->GetParent())
                {
                    if (_pRoot->IsAccessible())
                        break;
                }

                bool _bAdd = false;
                bool _bRemove = false;

                // 通知 Child 添加
                if (_SrcItem.AddChildrenArray.GetSize() >= 25 && _pRoot)
                {
                    _bAdd = true;
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _pRoot->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;
                    UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildrenBulkAdded, nullptr, 0);
                }
                else
                {
                    for (auto _pChild : _SrcItem.AddChildrenArray)
                    {
                        if (!_pChild->IsAccessible())
                            continue;
                        _bAdd = true;

                        YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                        auto _hr = _pChild->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                        if (FAILED(_hr))
                            return _hr;

                        auto _Id = ElementAccessibleProvider::GetElementRuntimeId(_pChild);
                        UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildAdded, _Id.UiaIds, std::size(_Id.UiaIds));
                    }
                }

                // 通知 Child移除
                if (_SrcItem.RemoveChildrenRuntimeIds.GetSize() >= 25 && _pRoot)
                {
                    _bRemove = true;
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _pRoot->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;
                    
                    UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildrenBulkRemoved, nullptr, 0);
                }
                else if (_pRoot)
                {
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _pRoot->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;
                    
                    for (auto& _RuntimeId : _SrcItem.RemoveChildrenRuntimeIds)
                    {
                        if (!_RuntimeId.pElement->IsAccessible())
                            continue;

                        _bRemove = true;

                        auto _Id = ElementAccessibleProvider::GetElementRuntimeId(_RuntimeId.pElement);
                        UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildRemoved, _Id.UiaIds, std::size(_Id.UiaIds));
                    }
                }

                if (_pRoot && _bAdd == false && _bRemove == false)
                {
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _pRoot->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;
                    UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildrenReordered, nullptr, 0);
                }
            }

            return S_OK;
        }

        HRESULT AccessibleEventManager::RaiseVisibilityEvents()
        {
            auto _VisibleChange = std::move(s_AccessibleData.VisibleChange);
            auto _uVisibleChangeSize = _VisibleChange.GetSize();
            if (_uVisibleChangeSize == 0)
                return S_OK;

            YY::Array<ChildrenVisibleChangeData, AllocPolicy::SOO> _ElementVisibleChange;

            for (size_t i = _uVisibleChangeSize;i!=0;)
            {
                --i;
                auto _pElem = _VisibleChange[i];

                if (!_pElem->IsAccessible())
                    continue;

                auto _pVisibleAccessibleParent = ElementAccessibleProvider::GetVisibleAccessibleParent(_pElem);
                if (!_pVisibleAccessibleParent)
                {
                    // 它是顶级元素
                    // todo 如果有多个顶级，这行为可能存在问题
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;

                    _hr = UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildrenInvalidated, nullptr, 0);
                    return _hr;
                }

                ChildrenVisibleChangeData* _pItemChangeData = nullptr;
                for (auto& _Item : _ElementVisibleChange)
                {
                    if (_Item.pElem == _pVisibleAccessibleParent)
                    {
                        _pItemChangeData = &_Item;
                        break;
                    }
                }

                if (!_pItemChangeData)
                {
                    _pItemChangeData = _ElementVisibleChange.EmplacePtr(ChildrenVisibleChangeData {_pVisibleAccessibleParent});
                    if (!_pItemChangeData)
                        return E_OUTOFMEMORY;
                }


                if(!_pItemChangeData->Children.EmplacePtr(_pElem))
                    return E_OUTOFMEMORY;
            }

            for (auto& _Item : _ElementVisibleChange)
            {
                if (_Item.Children.GetSize() < 25)
                {
                    for (auto _pChild : _Item.Children)
                    {
                        if (_pChild->IsVisible())
                        {
                            YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                            auto _hr = _pChild->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                            if (FAILED(_hr))
                                return _hr;

                            _hr = UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildAdded, nullptr, 0);
                            if (FAILED(_hr))
                                return _hr;
                        }
                        else
                        {
                            YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                            auto _hr = _Item.pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                            if (FAILED(_hr))
                                return _hr;

                            auto _Id = ElementAccessibleProvider::GetElementRuntimeId(_pChild);
                            _hr = UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildAdded, _Id.UiaIds, std::size(_Id.UiaIds));
                            if (FAILED(_hr))
                                return _hr;
                        }
                    }
                }
                else
                {
                    YY::RefPtr<ElementAccessibleProvider> _pAccessibleProvider;
                    auto _hr = _Item.pElem->GetAccessibleProvider(_pAccessibleProvider.ReleaseAndGetAddressOf());
                    if (FAILED(_hr))
                        return _hr;
                    
                    _hr = UiaRaiseStructureChangedEvent(_pAccessibleProvider, StructureChangeType_ChildrenInvalidated, nullptr, 0);
                    if (FAILED(_hr))
                        return _hr;

                }
            }

            return S_OK;
        }
    }
}
