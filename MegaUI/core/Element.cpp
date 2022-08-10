#include "pch.h"
#include "Element.h"
#include "Property.h"
#include "value.h"
#include "ClassInfo.h"
#include "Layout.h"

#include <atlcomcli.h>

#pragma warning(disable : 28251)
#pragma warning(disable : 26812)

namespace YY
{
	namespace MegaUI
	{
		_APPLY_MEGA_UI_STATIC_CALSS_INFO(Element, _MEGA_UI_ELEMENT_PROPERTY_TABLE);

        Element::Element()
            : _iGCSlot(-1)
            , _iGCLPSlot(-1)
            , _iPCTail(-1)
            , pDeferObject(nullptr)
            , pTopLevel(nullptr)
            , pLocParent(nullptr)
            , LocPosInLayout {}
            , LocSizeInLayout {}
            , LocDesiredSize {}
            , LocLastDesiredSizeConstraint {}
            , iSpecLayoutPos(0)
            , fNeedsLayout(0)
            , bNeedsDSUpdate(0)
        {
        }

        Element::~Element()
        {
        }

        HRESULT __fastcall Element::Create(uint32_t _fCreate, Element* _pTopLevel, intptr_t* _pCooike, Element** _ppOut)
        {
            if (!_ppOut)
                return E_INVALIDARG;

            *_ppOut = nullptr;

            auto _pElement = HNew<Element>();
            if (!_pElement)
                return E_OUTOFMEMORY;

            auto _hr = _pElement->Initialize(_fCreate, _pTopLevel, _pCooike);
            if (SUCCEEDED(_hr))
            {
                *_ppOut = _pElement;
            }
            else
            {
                HDelete(_pElement);
            }
            return _hr;
        }

        HRESULT __fastcall Element::Initialize(uint32_t _fCreate, Element* _pTopLevel, intptr_t* _pCooike)
        {
            pTopLevel = _pTopLevel;

            if (_pCooike)
                StartDefer(_pCooike);

            return S_OK;
        }

		Value __fastcall Element::GetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, bool _bUpdateCache)
        {
            if (_eIndicies >= PropertyIndicies::PI_MAX)
                return Value::GetUnavailable();

            auto _uIndicies = uint32_t(_eIndicies);

            for (; _uIndicies && (_Prop.fFlags & (1u << (_uIndicies - 1))) == 0; --_uIndicies)
            {
                _eIndicies = PropertyIndicies(_uIndicies);
            }

            const auto _iIndex = GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return Value::GetUnavailable();

            Value _pValue = Value::GetUnset();

            FunTypePropertyCustomCache _pFunPropertyCache = nullptr;

            do
            {

                PropertyCustomCacheResult _CacheResult = SkipNone;
                if (_Prop.BindCacheInfo.uRaw)
                {
                    _pFunPropertyCache = _Prop.BindCacheInfo.bValueMapOrCustomPropFun ? &Element::PropertyGeneralCache : _Prop.BindCacheInfo.pFunPropertyCustomCache;

                    PropertyCustomCacheGetValueAction _Info;
                    _Info.pProp = &_Prop;
                    _Info.eIndicies = _eIndicies;
                    _Info.bUsingCache = _bUpdateCache == false;

                    _CacheResult = (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::GetValue, &_Info);

                    if (_Info.RetValue != nullptr)
                    {
                        _pValue = std::move(_Info.RetValue);

                        if (_pValue.GetType() != ValueType::Unset)
                            break;
                    }
                }

                // 走 _LocalPropValue，这是一种通用逻辑
                if ((_CacheResult & SkipLocalPropValue) == 0)
                {
                    auto _ppValue = LocalPropValue.GetItemPtr(_iIndex);
                    if (_ppValue && *_ppValue != nullptr)
                    {
                        _pValue = *_ppValue;
                        break;
                    }
                }

                // 如果值是本地的，那么最多就取一下 _LocalPropValue，我们就需要停止
                if (_eIndicies == PropertyIndicies::PI_Local)
                    break;

                // 尝试获取来自属性表的值
                if ((_Prop.fFlags & PF_Cascade) && (_CacheResult & SkipCascade) == 0)
                {
                }

                // 尝试从父节点继承
                if ((_Prop.fFlags & PF_Inherit) && (_CacheResult & SkipInherit) == 0)
                {
                    if (auto _pParent = GetParent())
                    {
                        auto pValueByParent = _pParent->GetValue(_Prop, _eIndicies, false);

                        if (pValueByParent != nullptr && _pValue.GetType() >= ValueType::Null)
                        {
                            _pValue = std::move(pValueByParent);
                            break;
                        }
                    }
                }

                // 最终还是没有，那么继承Default 值
                _pValue = _Prop.pFunDefaultValue();

            } while (false);

            if (_pFunPropertyCache && _pValue.GetType() >= ValueType::Null && (_Prop.fFlags & PF_ReadOnly) == 0 && _bUpdateCache)
            {
                PropertyCustomCacheUpdateValueAction _Info;
                _Info.pProp = &_Prop;
                _Info.eIndicies = _eIndicies;
                _Info.InputNewValue = _pValue;

                (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::UpdateValue, &_Info);
            }

            if (_pValue == nullptr)
                _pValue = Value::GetUnset();

            return _pValue;
        }

        HRESULT __fastcall Element::SetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _pValue)
        {
            if (_pValue == nullptr)
                return E_INVALIDARG;

            if (_eIndicies != PropertyIndicies::PI_Local)
                return E_NOTIMPL;

            if (_Prop.fFlags & PF_ReadOnly)
                return E_NOTIMPL;

            const auto _iIndex = GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return E_NOT_SET;

            const auto _uIndex = (size_t)_iIndex;

            auto _pvOld = GetValue(_Prop, _eIndicies, false);
            if (_pvOld == nullptr)
                return E_OUTOFMEMORY;

            if (_pvOld == _pValue)
                return S_OK;

            PreSourceChange(_Prop, _eIndicies, _pvOld, _pValue);

            auto _hr = S_OK;
            if (LocalPropValue.GetSize() <= _uIndex)
                _hr = LocalPropValue.Resize(_uIndex + 1);

            if (SUCCEEDED(_hr))
                _hr = LocalPropValue.SetItem(_uIndex, _pValue);

            PostSourceChange();

            return _hr;
        }

        Element* __fastcall Element::GetParent()
        {
            return pLocParent;
        }

        int32_t __fastcall Element::GetLayoutPos()
        {
            return iSpecLayoutPos;
        }

        HRESULT __fastcall Element::SetLayoutPos(int32_t _iLayoutPos)
        {
            auto _pValue = Value::CreateInt32(_iLayoutPos);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ClassInfoData.LayoutPosProp, PropertyIndicies::PI_Local, _pValue);
        }

        int32_t __fastcall Element::GetWidth()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.WidthProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }
            return _pValue.GetInt32();
        }

        HRESULT __fastcall Element::SetWidth(int32_t _iWidth)
        {
            auto _pValue = Value::CreateInt32(_iWidth);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ClassInfoData.WidthProp, PropertyIndicies::PI_Local, _pValue);
        }

        int32_t __fastcall Element::GetHeight()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.HeightProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }
            return _pValue.GetInt32();
        }

        HRESULT __fastcall Element::SetHeight(int32_t _iHeight)
        {
            auto _pValue = Value::CreateInt32(_iHeight);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ClassInfoData.HeightProp, PropertyIndicies::PI_Local, _pValue);
        }

        int32_t __fastcall Element::GetX()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.XProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }

            return _pValue.GetInt32();
        }

        HRESULT __fastcall Element::SetX(int32_t _iX)
        {
            auto _pValue = Value::CreateInt32(_iX);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ClassInfoData.XProp, PropertyIndicies::PI_Local, _pValue);
        }

        int32_t __fastcall Element::GetY()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.YProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }

            return _pValue.GetInt32();
        }

        HRESULT __fastcall Element::SetY(int32_t _iY)
        {
            auto _pValue = Value::CreateInt32(_iY);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ClassInfoData.YProp, PropertyIndicies::PI_Local, _pValue);
        }

        POINT __fastcall Element::GetLocation()
        {
            POINT _Location = {};
            auto _pValue = GetValue(Element::g_ClassInfoData.LocationProp, PropertyIndicies::PI_Local, false);

            if (_pValue != nullptr)
            {
                _Location = _pValue.GetPoint();
            }

            return _Location;
        }

        SIZE __fastcall Element::GetExtent()
        {
            SIZE _Extent = {};

            auto _pValue = GetValue(Element::g_ClassInfoData.ExtentProp, PropertyIndicies::PI_Local, false);
            if (_pValue != nullptr)
            {
                _Extent = _pValue.GetSize();
            }
            return _Extent;
        }

        ValueIs<ValueType::Layout> __fastcall Element::GetLayout()
        {
            return GetValue(Element::g_ClassInfoData.LayoutProp, PropertyIndicies::PI_Specified, false);
        }

		void __fastcall Element::OnPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _OldValue, const Value& _NewValue)
		{
            if (_Prop.pFunOnPropertyChanged)
                (this->*_Prop.pFunOnPropertyChanged)(_Prop, _eIndicies, _OldValue, _NewValue);
        }

        void __fastcall Element::OnGroupChanged(uint32_t _fGroups)
        {
            if (_fGroups == 0)
                return;
            
            auto _pDeferObject = GetDeferObject();
            if (!_pDeferObject)
                return;

            if ((PropertyGroup::PG_NormalPriMask & _fGroups) && (PropertyGroup::PG_LowPriMask & _fGroups))
            {
                throw Exception();
                return;
            }

            // Affects Desired Size or Affects Layout
            if (_fGroups & (PG_AffectsDesiredSize | PG_AffectsLayout))
            {
                // 找到通知树的根节点
                auto _pRoot = this;
                while (_pRoot->GetLayoutPos() != LP_Absolute)
                {
                    auto _pParent = _pRoot->GetParent();
                    if (!_pParent)
                        break;

                    _pRoot = _pParent;
                }

                if (_fGroups & PG_AffectsDesiredSize)
                {
                    _pDeferObject->UpdateDesiredSizeRootPendingSet.Insert(_pRoot);
                }

                if (_fGroups & PG_AffectsLayout)
                {
                    _pDeferObject->LayoutRootPendingSet.Insert(_pRoot);
                }
            }

            if (_fGroups & (PG_AffectsParentDesiredSize | PG_AffectsParentLayout))
            {
                // Locate Layout/DS root and queue tree as needing a layout pass
                // Root doesn't have parent or is absolute positioned
                Element* _pRoot;
                Element* _pParent = nullptr;

                for (Element* _pTmp = this;;)
                {
                    _pRoot = _pTmp;

                    _pTmp = _pRoot->GetParent();
                    if (_pTmp && !_pParent)
                        _pParent = _pTmp;

                    if (_pTmp == nullptr || _pRoot->GetLayoutPos() == LP_Absolute)
                        break;
                }

                if (_pParent)
                {
                    if (_fGroups & PG_AffectsParentDesiredSize)
                    {
                        _pDeferObject->UpdateDesiredSizeRootPendingSet.Insert(_pRoot);
                    }

                    if (_fGroups & PG_AffectsParentLayout)
                    {
                        _pDeferObject->LayoutRootPendingSet.Insert(_pRoot);
                    }
                }
            }

        }

        Element* Element::GetTopLevel()
        {
            auto _pTopLevel = this;

            for (; _pTopLevel->pTopLevel; _pTopLevel = _pTopLevel->pTopLevel);

            return _pTopLevel;
        }

        DeferCycle* __fastcall Element::GetDeferObject(bool _bAllowCreate)
        {
            if (pDeferObject)
                return pDeferObject;

            auto _pTopLevel = GetTopLevel();
            if (_pTopLevel->pDeferObject)
                return _pTopLevel->pDeferObject;

            if (!_bAllowCreate)
                return nullptr;

            auto _pDeferObject = HNew<DeferCycle>();
            if (!_pDeferObject)
                return nullptr;

            _pDeferObject->AddRef();
            _pTopLevel->pDeferObject = _pDeferObject;
            return _pDeferObject;
        }

        void __fastcall Element::StartDefer(intptr_t* _pCooike)
		{
            if (!_pCooike)
			{
				throw std::exception("pCooike == nullptr", 0);
				return;
			}
            
			if (auto _pDeferCycle = GetDeferObject())
			{
                // 随便写一个值，看起来比较特殊就可以了
                *_pCooike = 0x12345;

                _pDeferCycle->AddRef();
                ++_pDeferCycle->uEnter;
			}
		}

		void __fastcall Element::EndDefer(intptr_t _Cookie)
		{
            auto _pDeferCycle = GetDeferObject(false);
            if (!_pDeferCycle)
                return;

            if (_Cookie != 0x12345)
			{
				throw std::exception("Cookie Error", 0);
				return;
			}

            if (_pDeferCycle->uEnter == 1)
            {
                // StartDefer计数归零，开始应用更改。
                _pDeferCycle->bFiring = true;

                auto& vecGroupChangeNormalPriority = _pDeferCycle->vecGroupChangeNormalPriority;
                auto& vecGroupChangeLowPriority = _pDeferCycle->vecGroupChangeLowPriority;
                auto& vecPropertyChanged = _pDeferCycle->vecPropertyChanged;
                auto& LayoutRootPendingSet = _pDeferCycle->LayoutRootPendingSet;
                auto& UpdateDesiredSizeRootPendingSet = _pDeferCycle->UpdateDesiredSizeRootPendingSet;

                auto& uGroupChangeNormalPriorityFireCount = _pDeferCycle->uGroupChangeNormalPriorityFireCount;
                auto& uGroupChangeLowPriorityFireCount = _pDeferCycle->uGroupChangeLowPriorityFireCount;

                for (;;)
                {
                    // 首先通知正常优先级的 Group 队列
                    if (uGroupChangeNormalPriorityFireCount < vecGroupChangeNormalPriority.GetSize())
                    {
                        auto pGroupChangeRecord = vecGroupChangeNormalPriority.GetItemPtr(uGroupChangeNormalPriorityFireCount);
                        ++uGroupChangeNormalPriorityFireCount;

                        if (pGroupChangeRecord && pGroupChangeRecord->pElement)
                        {
                            pGroupChangeRecord->pElement->_iGCSlot = -1;
                            _pDeferCycle->Release();

                            pGroupChangeRecord->pElement->OnGroupChanged(pGroupChangeRecord->fGroups & PropertyGroup::PG_NormalPriMask);
                        }
                        continue;
                    }

                    // 然后更新 DesiredSize
                    if(auto pFistPending = UpdateDesiredSizeRootPendingSet.Pop())
                    {
                        pFistPending->FlushDesiredSize(_pDeferCycle);
                        continue;
                    }

                    // 然后更新布局
                    if (auto pFistPending = LayoutRootPendingSet.Pop())
                    {
                        pFistPending->FlushLayout(_pDeferCycle);
                        continue;
                    }

                    // 最后更新低优先级的 Group
                    if (uGroupChangeLowPriorityFireCount < vecGroupChangeLowPriority.GetSize())
                    {
                        auto pGroupChangeRecord = vecGroupChangeLowPriority.GetItemPtr(uGroupChangeLowPriorityFireCount);
                        ++uGroupChangeLowPriorityFireCount;

                        if (pGroupChangeRecord && pGroupChangeRecord->pElement)
                        {
                            pGroupChangeRecord->pElement->_iGCLPSlot = -1;
                            _pDeferCycle->Release();

                            pGroupChangeRecord->pElement->OnGroupChanged(pGroupChangeRecord->fGroups & PropertyGroup::PG_LowPriMask);
                        }
                        continue;
                    }

                    // 处理完成
                    vecGroupChangeNormalPriority.Clear();
                    uGroupChangeNormalPriorityFireCount = 0;
                    vecGroupChangeLowPriority.Clear();
                    uGroupChangeLowPriorityFireCount = 0;

                    _pDeferCycle->bFiring = false;
                    break;
                }
            }

            --_pDeferCycle->uEnter;
		}
        
        void __fastcall Element::Paint(_In_ ID2D1RenderTarget* _pRenderTarget, _In_ const Rect& _Bounds)
        {
            auto Background = GetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Specified, false);

            if (Background == nullptr)
                return;

            if (Background.GetType() == ValueType::Color)
            {
                ATL::CComPtr<ID2D1SolidColorBrush> m_pBlackBrush;
                auto hr = _pRenderTarget->CreateSolidColorBrush(
                   /* D2D1::ColorF(D2D1::ColorF::DarkRed)*/
                    Background.GetColor(),
                    &m_pBlackBrush);

                if (FAILED(hr))
                    return;

                _pRenderTarget->FillRectangle(_Bounds, m_pBlackBrush);
            }
        }

        SIZE __fastcall Element::GetContentSize(SIZE _ConstraintSize)
        {
            // todo
            return _ConstraintSize;
        }

        SIZE __fastcall Element::SelfLayoutUpdateDesiredSize(SIZE _ConstraintSize)
        {
            // 仅给子类留一个口，什么也不用做
            return SIZE{};
        }

        void __fastcall Element::SelfLayoutDoLayout(SIZE _ConstraintSize)
        {
            // 仅给子类留一个口，什么也不用做
        }

        void __fastcall Element::Detach(DeferCycle* _pDeferCycle)
        {
            _pDeferCycle->UpdateDesiredSizeRootPendingSet.Remove(this);
            _pDeferCycle->LayoutRootPendingSet.Remove(this);

            if (_iGCSlot != -1)
            {
                _pDeferCycle->vecGroupChangeNormalPriority.GetItemPtr(_iGCSlot)->pElement = nullptr;
                _pDeferCycle->Release();
                _iGCSlot = -1;
            }

            if (_iGCLPSlot != -1)
            {
                _pDeferCycle->vecGroupChangeLowPriority.GetItemPtr(_iGCLPSlot)->pElement = nullptr;
                _pDeferCycle->Release();
                _iGCLPSlot = -1;
            }

            if (_iPCTail != -1)
            {
                for (size_t index = _iPCTail; index >= _pDeferCycle->uPropertyChangedFireCount;)
                {
                    auto pItem = _pDeferCycle->vecPropertyChanged.GetItemPtr(index);

                    if (pItem->iRefCount)
                    {
                        pItem->pElement = nullptr;
                        pItem->iRefCount = 0;
                        pItem->pNewValue = nullptr;
                        pItem->pOldValue = nullptr;

                        _pDeferCycle->Release();
                    }

                    index = pItem->iPrevElRec;
                }

                _iPCTail = -1;
            }
        }

        ElementList __fastcall Element::GetChildren()
        {
            return vecLocChildren;
        }

        HRESULT __fastcall Element::Insert(Element* const* _ppChildren, uint32_t _cChildren, uint32_t _uInsert)
        {
            if (_cChildren == 0)
                return S_OK;

            if (_ppChildren == nullptr)
                return E_INVALIDARG;

            auto _OldChildrenList = GetChildren();
            const auto _cOldChildrenList = _OldChildrenList.GetSize();

            if (_cOldChildrenList < _uInsert)
                return E_INVALIDARG;

            ElementList _NewChildrenList;
            auto _hr = _NewChildrenList.Reserve(_cOldChildrenList + _cChildren);
            if (FAILED(_hr))
                return _hr;

            auto _NewParentValue = Value::CreateElementRef(this);
            if (_NewParentValue == nullptr)
                return E_OUTOFMEMORY;

            HREFTYPE hr = S_OK;

            do
            {
                for (uint32_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    if (_pTmp->pLocParent)
                    {
                        _pTmp->pLocParent->Remove(_pTmp);
                    }
                    else if (GetDeferObject(false) != _pTmp->pDeferObject && _pTmp->pDeferObject)
                    {
                        _pTmp->Detach(_pTmp->pDeferObject);
                        _pTmp->pDeferObject->Release();
                        _pTmp->pDeferObject = nullptr;
                    }
                }

                _NewChildrenList.Add(_OldChildrenList.GetData(), _uInsert);

                uint32_t _uLastIndex = _uInsert;

                for (uint32_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    _pTmp->_iIndex = _uLastIndex;
                    _NewChildrenList.Add(_pTmp);
                    ++_uLastIndex;
                }

                for (uint32_t _uIndex = _uInsert; _uIndex != _cOldChildrenList; ++_uIndex)
                {
                    auto _pTmp = _OldChildrenList[_uIndex];
                    _pTmp->_iIndex = _uLastIndex;
                    _NewChildrenList.Add(_pTmp);
                    ++_uLastIndex;
                }

                if (_NewChildrenList.GetSize() == _OldChildrenList.GetSize())
                    break;

                auto _NewChildrenValue = Value::CreateElementList(_NewChildrenList);
                if (_NewChildrenValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _OldChildrenValue = Value::CreateElementList(_OldChildrenList);
                if (_OldChildrenValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                intptr_t Cooike = 0;

                StartDefer(&Cooike);
                
                // 更新 ChildrenProp
                PreSourceChange(Element::g_ClassInfoData.ChildrenProp, PropertyIndicies::PI_Local, _OldChildrenValue, _NewChildrenValue);
                vecLocChildren.SetArray(std::move(_NewChildrenList));
                PostSourceChange();

                auto _pDeferObject = GetDeferObject();

                for (uint32_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    if (auto p = _pTmp->pDeferObject)
                    {
                        p->Release();
                    }

                    _pTmp->pDeferObject = _pDeferObject;
                    _pDeferObject->AddRef();
                    
                    _pTmp->PreSourceChange(Element::g_ClassInfoData.ParentProp, PropertyIndicies::PI_Local, Value::GetElementNull(), _NewParentValue);

                    _pTmp->pLocParent = this;
                    _pTmp->pTopLevel = this;

                    _pTmp->PostSourceChange();

                    _pTmp->pDeferObject->Release();
                    _pTmp->pDeferObject = nullptr;
                }
                
                EndDefer(Cooike);

            } while (false);

            if (FAILED(hr))
            {
                auto _OldSize = _OldChildrenList.GetSize();
                for (uint32_t _uIndex = _uInsert; _uIndex != _OldSize; ++_uIndex)
                {
                    _OldChildrenList[_uIndex]->_iIndex = _uIndex;
                }
            }

            return hr;
        }
        
        HRESULT __fastcall Element::Remove(Element* const* _ppChildren, uint32_t _cChildren)
        {
            if (_cChildren == 0)
                return S_OK;

            if (_ppChildren == nullptr)
                return E_INVALIDARG;
            
            auto _ChildrenOld = GetChildren();
            if (_ChildrenOld.GetSize() == 0)
                return S_FALSE;

            uint32_t _uRemoveCount = 0u;

            for (auto _Index = 0u; _Index != _cChildren; ++_Index)
            {
                auto _pItem = _ppChildren[_Index];
                if (_pItem == nullptr)
                    continue;

                if (_pItem->GetParent() != this)
                    continue;

                _pItem->_iIndex = -1;
                ++_uRemoveCount;
            }

            if (_uRemoveCount == 0)
                return S_FALSE;

            HRESULT hr = S_OK;

            do
            {
                ElementList _ChildrenNew;
                auto _uSizeNew = _ChildrenOld.GetSize() - _uRemoveCount;

                if (_uSizeNew)
                {
                    auto _pBuffer = _ChildrenNew.LockBufferAndSetSize(_uSizeNew);
                    if (!_pBuffer)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    for (auto _pItem : _ChildrenOld)
                    {
                        if (_pItem->_iIndex == -1)
                            continue;

                         _pItem->_iIndex = _pBuffer - _ChildrenNew.GetData();
                        *_pBuffer = _pItem;
                        ++_pBuffer;
                    }

                    _ChildrenNew.UnlockBuffer();
                }

                auto _ChildrenValueOld = Value::CreateElementList(_ChildrenOld);
                if (_ChildrenValueOld == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _ChildrenValueNew = Value::CreateElementList(_ChildrenNew);
                if (_ChildrenValueNew == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _OldParentValue = Value::CreateElementRef(this);
                if (_OldParentValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                intptr_t _CookiePtr;
                StartDefer(&_CookiePtr);

                PreSourceChange(Element::g_ClassInfoData.ChildrenProp, PropertyIndicies::PI_Local, _ChildrenValueOld, _ChildrenValueNew);
                vecLocChildren.SetArray(std::move(_ChildrenNew));
                PostSourceChange();


                auto _pDeferObject = GetDeferObject();

                for (auto _Index = 0u; _Index != _cChildren; ++_Index)
                {
                    auto _pItem = _ppChildren[_Index];
                    if (_pItem == nullptr)
                        continue;

                    if (_pItem->GetParent() != this)
                        continue;

                    if (_pItem->pDeferObject == nullptr)
                    {
                        _pItem->pDeferObject = _pDeferObject;
                        _pDeferObject->AddRef();
                    }
                    
                    _pItem->PreSourceChange(Element::g_ClassInfoData.ParentProp, PropertyIndicies::PI_Local, _OldParentValue, Value::GetElementNull());
                    _pItem->pLocParent = nullptr;
                    _pItem->pTopLevel = nullptr;
                    PostSourceChange();
                }

                EndDefer(_CookiePtr);
            } while (false);

            if (FAILED(hr))
            {
                // 回滚操作
                for (auto& _pItem : _ChildrenOld)
                {
                    _pItem->_iIndex = &_pItem - _ChildrenOld.GetData();
                }
            }

            return hr;
        }

        HRESULT __fastcall Element::PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _pNewValue)
        {
            if (_pOldValue == nullptr || _pNewValue == nullptr)
                return E_INVALIDARG;

            auto _pDeferObject = GetDeferObject();

            if (!_pDeferObject)
                return 0x800403EA;

            ++_pDeferObject->uPropertyChangeEnter;

            PCRecord* _pPCRecord = _pDeferObject->vecPropertyChanged.AddAndGetPtr();
            if (!_pPCRecord)
                return E_OUTOFMEMORY;

            auto TempInt = _pDeferObject->uPropertyChangedPostSourceCount;

            _pDeferObject->AddRef();
            _pPCRecord->iRefCount = 1;
            _pPCRecord->vC = -1;
            _pPCRecord->pElement = this;
            _pPCRecord->pProp = &_Prop;
            _pPCRecord->iIndex = _eIndicies;
            _pPCRecord->iPrevElRec = _pPCRecord->pElement->_iPCTail;
            _pPCRecord->pElement->_iPCTail = TempInt;

            _pPCRecord->pOldValue = _pOldValue;

            if (_eIndicies == PropertyIndicies::PI_Specified)
            {
                _pPCRecord->pNewValue = _pNewValue;
            }
            else
            {
                _pPCRecord->pNewValue = nullptr;
            }

            auto bFaild = false;

            DepRecs DepRecs;

            for (auto i = _pDeferObject->uPropertyChangedPostSourceCount; i < _pDeferObject->vecPropertyChanged.GetSize(); ++i)
            {
                auto pc = _pDeferObject->vecPropertyChanged.GetItemPtr(i);

                if (FAILED(pc->pElement->GetDependencies(*pc->pProp, pc->iIndex, &DepRecs, TempInt, _pNewValue, _pDeferObject)))
                    bFaild = true;

                pc = _pDeferObject->vecPropertyChanged.GetItemPtr(i);

                pc->dr = DepRecs;
            }

            for (auto i = _pDeferObject->uPropertyChangedPostSourceCount + 1; i < _pDeferObject->vecPropertyChanged.GetSize(); ++i)
            {
                auto pc = _pDeferObject->vecPropertyChanged.GetItemPtr(i);

                if (pc->iRefCount && pc->vC == -1 && pc->pOldValue == nullptr)
                {
                    auto p2a = -1;

                    auto pValue = pc->pElement->GetValue(*pc->pProp, pc->iIndex, false);

                    auto j = pc->pElement->_iPCTail;

                    PCRecord* pctmp = nullptr;

                    do
                    {
                        auto pcj = _pDeferObject->vecPropertyChanged.GetItemPtr(j);

                        if (pcj->iIndex == pc->iIndex && pcj->pProp == pc->pProp)
                        {
                            if (p2a == -1)
                            {
                                pctmp = pcj;
                                p2a = j;
                                pcj->pOldValue = pValue;
                            }
                            else
                            {
                                pcj->vC = p2a;
                                ++pctmp->iRefCount;

                                for (auto i = 0; i < pcj->dr.cDepCnt; ++i)
                                {
                                    VoidPCNotifyTree(i + pcj->dr.iDepPos, _pDeferObject);
                                }

                                pcj->dr.cDepCnt = 0;
                            }
                        }

                        j = pcj->iPrevElRec;

                    } while (j >= i);
                }
            }

            return bFaild != 0 ? 0x800403EB : S_OK;
        }
        
        HRESULT __fastcall Element::PostSourceChange()
        {
            auto pDeferObject = GetDeferObject();
            if (!pDeferObject)
                return 0x800403EA;

            bool bSuccess = false;
            intptr_t Cookie;

            StartDefer(&Cookie);

            for (; pDeferObject->uPropertyChangedPostSourceCount < pDeferObject->vecPropertyChanged.GetSize(); ++pDeferObject->uPropertyChangedPostSourceCount)
            {
                auto pc = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedPostSourceCount);

                if (pc->iRefCount)
                {
                    if ((pc->vC & 0x80000000) == 0)
                    {
                        pc->iRefCount = 0;

                        pDeferObject->Release();
                    }
                    else
                    {
                        if (pc->pNewValue == nullptr)
                            pc->pNewValue = pc->pElement->GetValue(*pc->pProp, pc->iIndex, true);

                        if (pc->pOldValue == pc->pNewValue)
                        {
                            pc->iRefCount = 1;

                            VoidPCNotifyTree(pDeferObject->uPropertyChangedPostSourceCount, pDeferObject);
                        }
                        /*else if (!pc->pProp->v18->_iGlobalIndex)
                        {
                            if (auto pElement = pc->pNewValue->GetElement())
                            {
                                auto pTop = pElement->GetTopLevel();

                                auto pClassInfo = pTop->GetClassInfoW();

                                if (pClassInfo->IsSubclassOf(HWNDElement::GetClassInfoPtr()))
                                    pc->pe->_InheritProperties();
                            }
                        }*/
                    }
                }
            }

            if (pDeferObject->uPropertyChangeEnter == 1)
            {
                for (; pDeferObject->uPropertyChangedFireCount < pDeferObject->vecPropertyChanged.GetSize(); ++pDeferObject->uPropertyChangedFireCount)
                {
                    auto pPCRecord = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedFireCount);

                    if (pPCRecord->iRefCount)
                    {
                        if ((pPCRecord->pProp->fFlags & PropertyFlag::PF_TypeBits) == PropertyIndiciesMapToPropertyFlag(pPCRecord->iIndex))
                        {
                            bSuccess = SetGroupChanges(pPCRecord->pElement, pPCRecord->pProp->fGroups, pDeferObject) == 0;
                        }

                        //pPCRecord->pElement->HandleUiaPropertyListener(pPCRecord->pProp, pPCRecord->iIndex, pPCRecord->pOldValue, pPCRecord->pNewValue);

                        pPCRecord->pElement->OnPropertyChanged(*pPCRecord->pProp, pPCRecord->iIndex, pPCRecord->pOldValue, pPCRecord->pNewValue);

                        pPCRecord = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedFireCount);

                        pPCRecord->pOldValue = nullptr;

                        pPCRecord->pNewValue = nullptr;

                        pPCRecord->iRefCount = 0;

                        pDeferObject->Release();
                    }

                    if (pPCRecord->pElement && pDeferObject->uPropertyChangedFireCount == pPCRecord->pElement->_iPCTail)
                    {
                        pPCRecord->pElement->_iPCTail = -1;
                    }
                }

                pDeferObject->uPropertyChangedFireCount = 0;
                pDeferObject->uPropertyChangedPostSourceCount = 0;

                pDeferObject->vecPropertyChanged.Clear();
            }

            //LABEL_15

            --pDeferObject->uPropertyChangeEnter;
            EndDefer(Cookie);

            return bSuccess != 0 ? 0x800403EB : 0;
        }

        HRESULT __fastcall Element::GetDependencies(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle)
        {
            HRESULT _hrLast = S_OK;

            switch (_eIndicies)
            {
            case YY::MegaUI::PropertyIndicies::PI_Local:
                if (_Prop.fFlags & PropertyFlag::PF_HasSpecified)
                {
                    auto hr = AddDependency(this, _Prop, PropertyIndicies::PI_Specified, pdr, _pDeferCycle);
                    if (FAILED(hr))
                        _hrLast = hr;
                }
                break;
            case YY::MegaUI::PropertyIndicies::PI_Specified:
                if (_Prop.fFlags & PropertyFlag::PF_Inherit)
                {
                    for (auto _pChild : vecLocChildren)
                    {
                        auto _iPropIndex = _pChild->GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
                        if (_iPropIndex < 0)
                            continue;
                        
                        auto ppValue = _pChild->LocalPropValue.GetItemPtr(_iPropIndex);
                        if (ppValue && *ppValue != nullptr)
                            continue;

                        auto hr = AddDependency(_pChild, _Prop, PropertyIndicies::PI_Specified, pdr, _pDeferCycle);
                        if (FAILED(hr))
                            _hrLast = hr;
                    }
                }
                
                if (_Prop.fFlags & PropertyFlag::PF_HasComputed)
                {
                    auto hr = AddDependency(this, _Prop, PropertyIndicies::PI_Computed, pdr, _pDeferCycle);
                    if (FAILED(hr))
                        _hrLast = hr;
                }
                break;
            case YY::MegaUI::PropertyIndicies::PI_Computed:
                break;
            }
            return _hrLast;
        }

        HRESULT __fastcall Element::AddDependency(Element* _pElement, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, DeferCycle* _pDeferCycle)
        {
            uint_t _uIndex;
            PCRecord* pItem = _pDeferCycle->vecPropertyChanged.AddAndGetPtr(&_uIndex);

            if (!pItem)
            {
                return E_OUTOFMEMORY;
            }

            _pDeferCycle->AddRef();

            pItem->iRefCount = 1;
            pItem->vC = -1;
            pItem->pElement = _pElement;
            pItem->pProp = &_Prop;
            pItem->iIndex = _eIndicies;
            pItem->pOldValue = nullptr;
            pItem->pNewValue = nullptr;
            pItem->iPrevElRec = _pElement->_iPCTail;
            _pElement->_iPCTail = _uIndex;

            if (!pdr->cDepCnt)
            {
                pdr->iDepPos = _uIndex;
            }

            ++(pdr->cDepCnt);

            return S_OK;
        }

		void __fastcall Element::VoidPCNotifyTree(int p1, DeferCycle* p2)
        {
            auto pr = p2->vecPropertyChanged.GetItemPtr(p1);
            if (!pr)
                return;
            if (pr->iRefCount)
            {
                if (pr->vC >= 0)
                {
                    VoidPCNotifyTree(pr->vC, p2);
                    pr->iRefCount = 0;
                }
                else
                {
                    if (--pr->iRefCount)
                        return;
                    
                    pr->pOldValue = nullptr;
                    pr->pNewValue = nullptr;

                    for (int i = 0; i != pr->dr.cDepCnt; ++i)
                        VoidPCNotifyTree(pr->dr.iDepPos + i, p2);
                }

                p2->Release();
            }
        }

        bool __fastcall Element::MarkElementForLayout(Element* _pElement, uint32_t _fNeedsLayoutNew)
        {
            if (_pElement && _pElement->SetNeedsLayout(_fNeedsLayoutNew))
            {
                for (;;)
                {
                    if (_pElement->pLocParent == nullptr || _pElement->iSpecLayoutPos == LP_Absolute)
                    {
                        return true;
                    }

                    _pElement = _pElement->pLocParent;

                    if (!_pElement->SetNeedsLayout(LC_Unknow1))
                    {
                        break;
                    }
                }
            }

            return false;
        }

        bool __fastcall Element::MarkElementForDesiredSize(Element* _pElement)
        {
            for (; _pElement; _pElement = _pElement->pLocParent)
            {
                if (_pElement->bNeedsDSUpdate)
                    break;

                _pElement->bNeedsDSUpdate = true;

                if (_pElement->pLocParent == nullptr || _pElement->iSpecLayoutPos == LP_Absolute)
                    return 1;
            }

            return 0;
        }

        bool __fastcall Element::SetGroupChanges(Element* pElement, uint32_t _fGroups, DeferCycle* pDeferCycle)
        {
            if (pElement->fNeedsLayout == LC_Optimize)
            {
                _fGroups &= ~PG_AffectsLayout;
            }

            bool bResult = true;

            if (_fGroups & PG_NormalPriMask)
            {
                TransferGroupFlags(pElement, _fGroups);

                GCRecord* pItem = nullptr;

                if (pElement->_iGCSlot == -1)
                {
                    uint_t uAddIndex;
                    pItem = pDeferCycle->vecGroupChangeNormalPriority.AddAndGetPtr(&uAddIndex);
                    if (!pItem)
                    {
                        bResult = false;
                    }
                    else
                    {
                        pDeferCycle->AddRef();
                        pItem->pElement = pElement;
                        pItem->fGroups = 0;

                        pElement->_iGCSlot = uAddIndex;
                    }
                }
                else
                {
                    pItem = pDeferCycle->vecGroupChangeNormalPriority.GetItemPtr(pElement->_iGCSlot);
                }

                if (pItem)
                    pItem->fGroups |= _fGroups;
            }

            if (_fGroups & PG_LowPriMask)
            {
                GCRecord* pItem = nullptr;

                if (pElement->_iGCLPSlot == -1)
                {
                    uint_t uAddIndex;
                    auto pItem = pDeferCycle->vecGroupChangeLowPriority.AddAndGetPtr(&uAddIndex);

                    if (!pItem)
                    {
                        bResult = false;
                    }
                    else
                    {
                        pDeferCycle->AddRef();
                        pItem->pElement = pElement;
                        pItem->fGroups = 0;

                        pElement->_iGCLPSlot = uAddIndex;
                    }
                }
                else
                {
                    pItem = pDeferCycle->vecGroupChangeLowPriority.GetItemPtr(pElement->_iGCLPSlot);
                }

                if (pItem)
                    pItem->fGroups |= _fGroups;
            }

            return bResult;
        }
        
        bool __fastcall Element::SetNeedsLayout(uint32_t _fNeedsLayoutNew)
        {
            if (_fNeedsLayoutNew > fNeedsLayout)
            {
                fNeedsLayout = _fNeedsLayoutNew;
                return true;
            }
            return false;
        }

        void __fastcall Element::TransferGroupFlags(Element* pElement, uint32_t _fGroups)
        {
            if (_fGroups & PG_AffectsLayout)
                MarkElementForLayout(pElement, LC_Normal);

            if (_fGroups & PG_AffectsDesiredSize)
                MarkElementForDesiredSize(pElement);

            if (_fGroups & PG_AffectsParentLayout)
                MarkElementForLayout(pElement->iSpecLayoutPos != LP_Absolute ? pElement->pLocParent : pElement, LC_Normal);

            if (_fGroups & PG_AffectsParentDesiredSize)
                MarkElementForDesiredSize(pElement->iSpecLayoutPos != LP_Absolute ? pElement->pLocParent : pElement);
        }

        PropertyCustomCacheResult __fastcall Element::PropertyGeneralCache(
            PropertyCustomCacheActionMode _eMode,
            PropertyCustomCachenBaseAction* _pBaseInfo)
		{
            uint16_t _uOffsetToCache = 0;
            uint16_t _uOffsetToHasCache = 0;
            uint8_t _uCacheBit;
            uint8_t _uHasCacheBit;

			auto _pProp = _pBaseInfo->pProp;

			switch (_pBaseInfo->eIndicies)
			{
			case PropertyIndicies::PI_Computed:
			case PropertyIndicies::PI_Specified:
                if (_pProp->BindCacheInfo.OffsetToSpecifiedValue)
				{
                    _uOffsetToCache = _pProp->BindCacheInfo.OffsetToSpecifiedValue;
                    _uCacheBit = _pProp->BindCacheInfo.SpecifiedValueBit;
                    _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasSpecifiedValueCache;
                    _uHasCacheBit = _pProp->BindCacheInfo.HasSpecifiedValueCacheBit;
					break;
				}
			case PropertyIndicies::PI_Local:
                _uOffsetToCache = _pProp->BindCacheInfo.OffsetToLocalValue;
                _uCacheBit = _pProp->BindCacheInfo.LocalValueBit;
                _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasLocalCache;
                _uHasCacheBit = _pProp->BindCacheInfo.HasLocalValueCacheBit;
				break;
			default:
				return PropertyCustomCacheResult::SkipNone;
				break;
			}

			if (_eMode == PropertyCustomCacheActionMode::GetValue)
			{
                auto _pInfo = (PropertyCustomCacheGetValueAction*)_pBaseInfo;
                Value _pRetValue;

				do
				{
                    if (_uOffsetToCache == 0)
						break;

					// 如果属性是 PF_ReadOnly，那么它必然不会实际走到 _LocalPropValue 里面去，必须走 缓存
					// 如果 _UsingCache == true，那么我们可以走缓存
                    if ((_pProp->fFlags & PF_ReadOnly) || _pInfo->bUsingCache)
					{
						// 检测实际是否存在缓存，如果检测到没有缓存，那么直接返回
                        if (_uOffsetToHasCache)
						{
                            const auto _uHasValue = *((uint8_t*)this + _uOffsetToHasCache);
                            if ((_uHasValue & (1 << _uHasCacheBit)) == 0)
							{
                                _pRetValue = Value::GetUnset();
								break;
							}
						}

						auto _pCache = (char*)this + _uOffsetToCache;

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            _pRetValue = Value::CreateInt32(*(int32_t*)_pCache);
							break;
						case ValueType::boolean:
                            _pRetValue = Value::CreateBool((*(uint8_t*)_pCache) & (1 << _uCacheBit));
							break;
                        case ValueType::Color:
                            _pRetValue = Value::CreateColor(*(Color*)_pCache);
                            break;
                        case ValueType::POINT:
                            _pRetValue = Value::CreatePoint(*(POINT*)_pCache);
                            break;
                        case ValueType::SIZE:
                            _pRetValue = Value::CreateSize(*(SIZE*)_pCache);
                            break;
                        case ValueType::Rect:
                            _pRetValue = Value::CreateRect(*(Rect*)_pCache);
                            break;
						default:
                            _pRetValue = Value::GetNull();
							break;
						}

                        if (_pRetValue == nullptr)
                            _pRetValue = Value::GetUnset();
					}
				} while (false);
				
				_pInfo->RetValue = std::move(_pRetValue);

				if (_pProp->fFlags & PF_ReadOnly)
				{
					return PropertyCustomCacheResult::SkipLocalPropValue;
				}

				return PropertyCustomCacheResult::SkipNone;
			}
            else if (_eMode == PropertyCustomCacheActionMode::UpdateValue)
			{
                auto _pInfo = (PropertyCustomCacheUpdateValueAction*)_pBaseInfo;

				do
				{
                    if (_uOffsetToCache == 0)
						break;

					auto& _pNewValue = _pInfo->InputNewValue;

					if (_pNewValue == nullptr)
						break;

					if (_pNewValue.GetType() == ValueType::Unset)
					{
                        if (_uOffsetToHasCache == 0)
							break;

						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);

						_uHasCache &= ~(1 << _uHasCacheBit);
					}
                    else if (_pNewValue.GetType() == (ValueType)_pProp->BindCacheInfo.eType)
					{
						// 标记缓存已经被设置
                        if (_uOffsetToHasCache)
                        {
                            auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);
                            _uHasCache |= (1 << _uHasCacheBit);
                        }

						auto _pCache = (char*)this + _uOffsetToCache;

                        auto _pDataBuyffer = _pNewValue.GetRawBuffer();

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            *(int32_t*)_pCache = *(int32_t*)_pDataBuyffer;
							break;
						case ValueType::boolean:
                            if (*(bool*)_pDataBuyffer)
							{
                                *_pCache |= (1 << _uCacheBit);
							}
							else
							{
                                *_pCache &= ~(1 << _uCacheBit);
							}
							break;
                        case ValueType::Color:
                            *(Color*)_pCache = *(Color*)_pDataBuyffer;
                            break;
                        case ValueType::POINT:
                            *(POINT*)_pCache = *(POINT*)_pDataBuyffer;
                            break;
                        case ValueType::SIZE:
                            *(SIZE*)_pCache = *(SIZE*)_pDataBuyffer;
                            break;
                        case ValueType::Rect:
                            *(Rect*)_pCache = *(Rect*)_pDataBuyffer;
                            break;
						default:
							break;
						}
					}

				} while (false);
			}

			return PropertyCustomCacheResult::SkipNone;
		}
		
		void __fastcall Element::OnParentPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _OldValue, const Value& _NewValue)
		{
			
		}

        void __fastcall Element::FlushDesiredSize(DeferCycle* _pDeferCycle)
        {
            if ((pLocParent == nullptr || iSpecLayoutPos == LP_Absolute))
            {
                SIZE _ConstraintSize = { GetWidth(), GetHeight() };

                if (_ConstraintSize.cx == -1)
                    _ConstraintSize.cx = int16_max;

                if (_ConstraintSize.cy == -1)
                    _ConstraintSize.cy = int16_max;

                UpdateDesiredSize(_ConstraintSize);

                /*auto hDC = GetDC(nullptr);

                {
                    DCSurface DCS(hDC);

                    if (y == -1)
                        y = 0x7FFF;

                    if (x == -1)
                        x = 0x7FFF;

                    UpdateDesiredSize(x, y, &DCS);
                }

                ReleaseDC(nullptr, hDC);*/
            }
        }

        void __fastcall Element::FlushLayout(DeferCycle* _pDeferCycle)
        {
            auto _fNeedsLayout = fNeedsLayout;

            if (fNeedsLayout == LC_Pass)
                return;

            fNeedsLayout = LC_Pass;

            if (_fNeedsLayout == LC_Normal)
            {
                auto _Layout = GetLayout();
                if ((_Layout.HasValue() && _Layout.GetValue()) || bSelfLayout)
                {
                    auto Extent = GetExtent();

                    Extent.cx -= SpecBorderThickness.left + SpecBorderThickness.right;
                    Extent.cy -= SpecBorderThickness.top + SpecBorderThickness.bottom;

                    Extent.cx -= SpecPadding.left + SpecPadding.right;
                    Extent.cy -= SpecPadding.top + SpecPadding.bottom;

                    if (Extent.cx < 0)
                        Extent.cx = 0;

                    if (Extent.cy < 0)
                        Extent.cy = 0;

                    if (bSelfLayout)
                        SelfLayoutDoLayout(Extent);
                    else
                        _Layout.GetValue()->DoLayout(this, Extent.cx, Extent.cy);
                }

                for (auto pChildren : GetChildren())
                {
                    auto _iLayoutPos = pChildren->GetLayoutPos();
                    if (_iLayoutPos == LP_None)
                    {
                        pChildren->UpdateLayoutPosition(POINT{ 0, 0 });
                        pChildren->UpdateLayoutSize(SIZE {0, 0});
                    }
                    else if (_iLayoutPos != LP_Absolute)
                    {
                        pChildren->FlushLayout(_pDeferCycle);
                    }
                }
            }
        }
        
        SIZE __fastcall Element::UpdateDesiredSize(SIZE _ConstraintSize)
        {
            SIZE sizeDesired = {};
            if (_ConstraintSize.cx < 0)
                _ConstraintSize.cx = 0;

            if (_ConstraintSize.cy < 0)
                _ConstraintSize.cy = 0;

            const auto _bChangedConst = LocDesiredSize.cx != _ConstraintSize.cx || LocDesiredSize.cy != _ConstraintSize.cy;

            if (bNeedsDSUpdate || _bChangedConst)
            {
                bNeedsDSUpdate = false;

                if (_bChangedConst)
                {
                    auto pSizeOld = Value::CreateSize(LocDesiredSize);
                    auto pSizeNew = Value::CreateSize(_ConstraintSize);

                    if (pSizeNew != nullptr)
                    {
                        PreSourceChange(Element::g_ClassInfoData.LastDesiredSizeConstraintProp, PropertyIndicies::PI_Local, pSizeOld, pSizeNew);

                        LocDesiredSize.cx = _ConstraintSize.cx;
                        LocDesiredSize.cy = _ConstraintSize.cy;

                        PostSourceChange();
                    }
                }

                auto nWidth = GetWidth();

                if (nWidth > _ConstraintSize.cx)
                    nWidth = _ConstraintSize.cx;

                auto nHeight = GetHeight();
                if (nHeight > _ConstraintSize.cy)
                    nHeight = _ConstraintSize.cy;

                sizeDesired.cx = nWidth == -1 ? _ConstraintSize.cx : nWidth;
                sizeDesired.cy = nHeight == -1 ? _ConstraintSize.cy : nHeight;

                auto BorderX = SpecBorderThickness.left + SpecBorderThickness.right;
                auto BorderY = SpecBorderThickness.top + SpecBorderThickness.bottom;

                BorderX += SpecPadding.left + SpecPadding.right;
                BorderY += SpecPadding.top + SpecPadding.bottom;

                SIZE _ConstraintContentSize;
                _ConstraintContentSize.cx = sizeDesired.cx - BorderX;

                if (_ConstraintContentSize.cx < 0)
                {
                    BorderX += _ConstraintContentSize.cx;
                    _ConstraintContentSize.cx = 0;
                }

                _ConstraintContentSize.cy = sizeDesired.cy - BorderY;
                if (_ConstraintContentSize.cy < 0)
                {
                    BorderY += _ConstraintContentSize.cy;
                    _ConstraintContentSize.cy = 0;
                }

                SIZE TmpSize;

                if (bSelfLayout)
                {
                    TmpSize = SelfLayoutUpdateDesiredSize(_ConstraintContentSize);
                }
                else
                {
                    #if 0
                    if (pLayout)
                    {
                        TmpSize = pLayout->UpdateDesiredSize(this, x, y);
                    }
                    else
                    #endif
                    {
                        TmpSize = GetContentSize(_ConstraintContentSize);
                    }
                }

                if (TmpSize.cx < 0)
                {
                    TmpSize.cx = 0;
                }
                else if (TmpSize.cx > _ConstraintContentSize.cx)
                {
                    TmpSize.cx = _ConstraintContentSize.cx;
                }
                if (TmpSize.cy < 0)
                {
                    TmpSize.cy = 0;
                }
                else if (TmpSize.cy > _ConstraintContentSize.cy)
                {
                    TmpSize.cy = _ConstraintContentSize.cy;
                }

                if (nWidth == -1)
                {
                    if (TmpSize.cx + BorderX < sizeDesired.cx)
                        sizeDesired.cx = TmpSize.cx + BorderX;
                }

                if (nHeight == -1)
                {
                    if (TmpSize.cy + BorderY < sizeDesired.cy)
                        sizeDesired.cy = TmpSize.cy + BorderY;
                }

                if (sizeDesired.cx < SpecMinSize.cx)
                {
                    sizeDesired.cx = min(_ConstraintSize.cx, SpecMinSize.cx);
                }

                if (sizeDesired.cy < SpecMinSize.cy)
                {
                    sizeDesired.cy = min(_ConstraintSize.cy, SpecMinSize.cy);
                }

                auto pSizeOld = Value::CreateSize(LocLastDesiredSizeConstraint);
                auto pSizeNew = Value::CreateSize(sizeDesired);

                PreSourceChange(g_ClassInfoData.DesiredSizeProp, PropertyIndicies::PI_Local, pSizeOld, pSizeNew);

                LocLastDesiredSizeConstraint = sizeDesired;

                PostSourceChange();
            }
            else
            {
                sizeDesired = LocLastDesiredSizeConstraint;
            }
            return sizeDesired;
        }
        
        PropertyCustomCacheResult __fastcall Element::GetExtentProperty(PropertyCustomCacheActionMode _eMode, PropertyCustomCachenBaseAction* _pInfo)
        {
            if (_eMode == PropertyCustomCacheActionMode::GetValue)
            {
                auto _pGetValueInfo = (PropertyCustomCacheGetValueAction*)_pInfo;

                auto& pExtentValue = _pGetValueInfo->RetValue;

                if (pLocParent && iSpecLayoutPos != LP_Absolute)
                {
                    pExtentValue = Value::CreateSize(LocSizeInLayout);
                }
                else
                {
                    pExtentValue = Value::CreateSize(LocDesiredSize);
                }
            }
            return PropertyCustomCacheResult::SkipAll;
        }

        PropertyCustomCacheResult __fastcall Element::GetLocationProperty(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo)
        {
            if (_eMode == PropertyCustomCacheActionMode::GetValue)
            {
                auto _pGetValueInfo = (PropertyCustomCacheGetValueAction*)_pInfo;

                auto& _Location = _pGetValueInfo->RetValue;

                if (pLocParent && iSpecLayoutPos != LP_Absolute)
                {
                    _Location = Value::CreatePoint(LocPosInLayout);
                }
                else
                {
                    _Location = Value::CreatePoint(GetX(), GetY());
                }
            }
            return PropertyCustomCacheResult::SkipAll;
        }

        void __fastcall Element::UpdateLayoutPosition(POINT _LayoutPosition)
        {
            if (LocPosInLayout.x == _LayoutPosition.x && LocPosInLayout.y == _LayoutPosition.y)
                return;

            auto _pPointOld = Value::CreatePoint(LocPosInLayout);
            if (_pPointOld == nullptr)
                return;

            auto _pPointNew = Value::CreatePoint(_LayoutPosition);
            if (_pPointNew == nullptr)
                return;

            PreSourceChange(Element::g_ClassInfoData.PosInLayoutProp, PropertyIndicies::PI_Local, _pPointOld, _pPointNew);

            LocPosInLayout = _LayoutPosition;

            PostSourceChange();
        }

        void __fastcall Element::UpdateLayoutSize(SIZE _LayoutSize)
        {
            if (LocSizeInLayout.cx == _LayoutSize.cx && LocSizeInLayout.cy == _LayoutSize.cy)
                return;
            
            auto _pSizeOld = Value::CreateSize(LocSizeInLayout);
            if (_pSizeOld == nullptr)
                return;

            auto _pSizeNew = Value::CreateSize(_LayoutSize);
            if (_pSizeNew == nullptr)
                return;

            PreSourceChange(Element::g_ClassInfoData.SizeInLayoutProp, PropertyIndicies::PI_Local, _pSizeOld, _pSizeNew);

            LocSizeInLayout = _LayoutSize;

            PostSourceChange();
        }
	}
}
