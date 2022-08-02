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

		Value* __fastcall Element::GetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, bool _bUpdateCache)
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

            Value* _pValue = Value::GetUnset();

            FunTypePropertyCustomCache _pFunPropertyCache = nullptr;

            do
            {

                PropertyCustomCacheResult _CacheResult = SkipNone;
                if (_Prop.BindCacheInfo.uRaw)
                {
                    _pFunPropertyCache = _Prop.BindCacheInfo.bValueMapOrCustomPropFun ? &Element::PropertyGeneralCache : _Prop.BindCacheInfo.pFunPropertyCustomCache;

                    PropertyCustomCacheActionInfo _Info = { &_Prop, _eIndicies };
                    _Info.GetValueInfo.bUsingCache = _bUpdateCache == false;

                    _CacheResult = (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::GetValue, &_Info);

                    if (_Info.GetValueInfo.pRetValue)
                    {
                        _pValue = _Info.GetValueInfo.pRetValue;

                        if (_pValue->GetType() != ValueType::Unset)
                            break;
                    }
                }

                // 走 _LocalPropValue，这是一种通用逻辑
                if ((_CacheResult & SkipLocalPropValue) == 0)
                {
                    if (auto _ppValue = LocalPropValue.GetItemPtr(_iIndex))
                    {
                        _pValue = *_ppValue;
                        _pValue->AddRef();
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

                        if (pValueByParent && _pValue->GetType() >= ValueType::Null)
                        {
                            _pValue = pValueByParent;
                            break;
                        }
                    }
                }

                // 最终还是没有，那么继承Default 值
                _pValue = _Prop.pFunDefaultValue();

            } while (false);

            if (_pFunPropertyCache && _pValue && _pValue->GetType() >= ValueType::Null && (_Prop.fFlags & PF_ReadOnly) == 0 && _bUpdateCache)
            {
                PropertyCustomCacheActionInfo _Info = { &_Prop, _eIndicies };
                _Info.UpdateValueInfo.pNewValue = _pValue;

                (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::UpdateValue, &_Info);
            }

            if (!_pValue)
                _pValue = Value::GetUnset();

            return _pValue;
        }

        HRESULT __fastcall Element::SetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pValue)
        {
            if (!_pValue)
                return E_POINTER;

            if (_eIndicies != PropertyIndicies::PI_Local)
                return E_NOTIMPL;

            if (_Prop.fFlags & PF_ReadOnly)
                return E_NOTIMPL;

            const auto _iIndex = GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return E_NOT_SET;

            const auto _uIndex = (size_t)_iIndex;

            auto _pvOld = GetValue(_Prop, _eIndicies, false);
            if (!_pvOld)
                return E_OUTOFMEMORY;

            if (_pvOld->IsEqual(_pValue))
                return S_OK;

            PreSourceChange(_Prop, _eIndicies, _pvOld, _pValue);

            auto _hr = S_OK;
            if (LocalPropValue.GetSize() <= _uIndex)
                _hr = LocalPropValue.Resize(_uIndex + 1);

            if (SUCCEEDED(_hr))
                _hr = LocalPropValue.SetItem(_uIndex, _pValue);

            if (SUCCEEDED(_hr))
            {
                _pValue->AddRef();
                _pvOld->Release();
            }

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
            if (!_pValue)
                return E_OUTOFMEMORY;

            auto _hr = SetValue(Element::g_ClassInfoData.LayoutPosProp, PropertyIndicies::PI_Local, _pValue);
            _pValue->Release();
            return _hr;
        }

        int32_t __fastcall Element::GetWidth()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.WidthProp, PropertyIndicies::PI_Specified, false);
            if (!_pValue)
            {
                throw Exception();
                return -1;
            }

            auto _iValue = _pValue->GetInt32();
            _pValue->Release();
            return _iValue;
        }

        HRESULT __fastcall Element::SetWidth(int32_t _iWidth)
        {
            auto _pValue = Value::CreateInt32(_iWidth);
            if (!_pValue)
                return E_OUTOFMEMORY;

            auto _hr = SetValue(Element::g_ClassInfoData.WidthProp, PropertyIndicies::PI_Local, _pValue);
            _pValue->Release();
            return _hr;
        }

        int32_t __fastcall Element::GetHeight()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.HeightProp, PropertyIndicies::PI_Specified, false);
            if (!_pValue)
            {
                throw Exception();
                return -1;
            }

            auto _iValue = _pValue->GetInt32();
            _pValue->Release();
            return _iValue;
        }

        HRESULT __fastcall Element::SetHeight(int32_t _iHeight)
        {
            auto _pValue = Value::CreateInt32(_iHeight);
            if (!_pValue)
                return E_OUTOFMEMORY;

            auto _hr = SetValue(Element::g_ClassInfoData.HeightProp, PropertyIndicies::PI_Local, _pValue);
            _pValue->Release();
            return _hr;
        }

        int32_t __fastcall Element::GetX()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.XProp, PropertyIndicies::PI_Specified, false);
            if (!_pValue)
            {
                throw Exception();
                return -1;
            }

            auto _iValue = _pValue->GetInt32();
            _pValue->Release();
            return _iValue;
        }

        HRESULT __fastcall Element::SetX(int32_t _iX)
        {
            auto _pValue = Value::CreateInt32(_iX);
            if (!_pValue)
                return E_OUTOFMEMORY;

            auto _hr = SetValue(Element::g_ClassInfoData.XProp, PropertyIndicies::PI_Local, _pValue);
            _pValue->Release();
            return _hr;
        }

        int32_t __fastcall Element::GetY()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.YProp, PropertyIndicies::PI_Specified, false);
            if (!_pValue)
            {
                throw Exception();
                return -1;
            }

            auto _iValue = _pValue->GetInt32();
            _pValue->Release();
            return _iValue;
        }

        HRESULT __fastcall Element::SetY(int32_t _iY)
        {
            auto _pValue = Value::CreateInt32(_iY);
            if (!_pValue)
                return E_OUTOFMEMORY;

            auto _hr = SetValue(Element::g_ClassInfoData.YProp, PropertyIndicies::PI_Local, _pValue);
            _pValue->Release();
            return _hr;
        }

        ValueIs<ValueType::POINT>* __fastcall Element::GetLocation()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.LocationProp, PropertyIndicies::PI_Local, false);
            if (!_pValue)
                return nullptr;

            auto _pRet = ValueIs<ValueType::POINT>::BuildByValue(_pValue);
            _pValue->Release();
            return _pRet;
        }

        SIZE __fastcall Element::GetExtent()
        {
            SIZE _Extent = {};

            auto _pValue = GetValue(Element::g_ClassInfoData.ExtentProp, PropertyIndicies::PI_Local, false);
            if (_pValue)
            {
                _Extent = _pValue->GetSize();
                _pValue->Release();
            }
            return _Extent;
        }

        ValueIs<ValueType::Layout>* Element::GetLayout()
        {
            auto _pValue = GetValue(Element::g_ClassInfoData.LayoutProp, PropertyIndicies::PI_Specified, false);
            if (!_pValue)
                return nullptr;
            auto _pRet = ValueIs<ValueType::Layout>::BuildByValue(_pValue);
            _pValue->Release();
            return _pRet;
        }

		void __fastcall Element::OnPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pOldValue, Value* _pNewValue)
		{
            if (_Prop.pFunOnPropertyChanged)
                (this->*_Prop.pFunOnPropertyChanged)(_Prop, _eIndicies, _pOldValue, _pNewValue);
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
        
        void __fastcall Element::Paint(_In_ ID2D1RenderTarget* _pRenderTarget, _In_ const RECT& _Bounds)
        {
            ATL::CComPtr<ID2D1SolidColorBrush> m_pBlackBrush;
            auto hr = _pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::DarkRed),
                &m_pBlackBrush);

            if (FAILED(hr))
                return;

            _pRenderTarget->FillRectangle(D2D1::RectF(_Bounds.left, _Bounds.top, _Bounds.right, _Bounds.bottom), m_pBlackBrush);
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

        ElementList __fastcall Element::GetChildren()
        {
            return vecLocChildren;
        }

        HRESULT __fastcall Element::PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pOldValue, _In_ Value* _pNewValue)
        {
            if (_pOldValue == nullptr)
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

            _pOldValue->AddRef();
            _pPCRecord->pOldValue = _pOldValue;

            if (_eIndicies == PropertyIndicies::PI_Specified)
            {
                _pNewValue->AddRef();
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
                                pValue->AddRef();
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

                    pValue->Release();
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
                        if (!pc->pNewValue)
                            pc->pNewValue = pc->pElement->GetValue(*pc->pProp, pc->iIndex, true);

                        if (pc->pOldValue->IsEqual(pc->pNewValue))
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

                        pPCRecord->pOldValue->Release();
                        pPCRecord->pOldValue = nullptr;

                        pPCRecord->pNewValue->Release();
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

        HRESULT __fastcall Element::GetDependencies(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, Value* _pNewValue, DeferCycle* _pDeferCycle)
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
                        if (ppValue && *ppValue)
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

                    if (pr->pOldValue)
                    {
                        pr->pOldValue->Release();
                        pr->pOldValue = nullptr;
                    }

                    if (pr->pNewValue)
                    {
                        pr->pNewValue->Release();
                        pr->pNewValue = nullptr;
                    }

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
            PropertyCustomCacheActionInfo* _pInfo)
		{
            uint16_t _uOffsetToCache = 0;
            uint16_t _uOffsetToHasCache = 0;
            uint8_t _uCacheBit;
            uint8_t _uHasCacheBit;

			auto _pProp = _pInfo->pProp;

			switch (_pInfo->eIndicies)
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
                Value* _pRetValue = nullptr;

				do
				{
                    if (_uOffsetToCache == 0)
						break;

					// 如果属性是 PF_ReadOnly，那么它必然不会实际走到 _LocalPropValue 里面去，必须走 缓存
					// 如果 _UsingCache == true，那么我们可以走缓存
                    if ((_pProp->fFlags & PF_ReadOnly) || _pInfo->GetValueInfo.bUsingCache)
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
						default:
                            _pRetValue = Value::GetNull();
							break;
						}

                        if (!_pRetValue)
                            _pRetValue = Value::GetUnset();
					}
				} while (false);
				
				_pInfo->GetValueInfo.pRetValue = _pRetValue;

				if (_pProp->fFlags & PF_ReadOnly)
				{
					return PropertyCustomCacheResult::SkipLocalPropValue;
				}

				return PropertyCustomCacheResult::SkipNone;
			}
            else if (_eMode == PropertyCustomCacheActionMode::UpdateValue)
			{
				do
				{
                    if (_uOffsetToCache == 0)
						break;

					auto _pNewValue = _pInfo->UpdateValueInfo.pNewValue;

					if (!_pNewValue)
						break;

					if (_pNewValue->GetType() == ValueType::Unset)
					{
                        if (_uOffsetToHasCache == 0)
							break;

						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);

						_uHasCache &= ~(1 << _uHasCacheBit);
					}
                    else if (_pNewValue->GetType() == (ValueType)_pProp->BindCacheInfo.eType)
					{
						// 标记缓存已经被设置
						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);
						_uHasCache |= (1 << _uHasCacheBit);

						auto _pCache = (char*)this + _uOffsetToCache;

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            *(int32_t*)_pCache = _pNewValue->GetInt32();
							break;
						case ValueType::boolean:
							if (_pNewValue->GetBool())
							{
                                *_pCache |= (1 << _uCacheBit);
							}
							else
							{
                                *_pCache &= ~(1 << _uCacheBit);
							}
							break;
						default:
							break;
						}
					}

				} while (false);
			}

			return PropertyCustomCacheResult::SkipNone;
		}
		
		void __fastcall Element::OnParentPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pOldValue, Value* pNewValue)
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
            /// TODO
            auto _fNeedsLayout = fNeedsLayout;

            if (fNeedsLayout == LC_Pass)
                return;

            fNeedsLayout = LC_Pass;

            if (_fNeedsLayout == LC_Normal)
            {
                auto _pLayout = GetLayout();
                if ((_pLayout && _pLayout->GetValue()) || bSelfLayout)
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
                        _pLayout->GetValue()->DoLayout(this, Extent.cx, Extent.cy);
                }

                if (_pLayout)
                    _pLayout->Release();

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

                    if (pSizeNew)
                    {
                        PreSourceChange(Element::g_ClassInfoData.LastDesiredSizeConstraintProp, PropertyIndicies::PI_Local, pSizeOld, pSizeNew);

                        LocDesiredSize.cx = _ConstraintSize.cx;
                        LocDesiredSize.cy = _ConstraintSize.cy;

                        PostSourceChange();
                    }

                    if (pSizeNew)
                        pSizeNew->Release();

                    if (pSizeOld)
                        pSizeOld->Release();
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

                pSizeNew->Release();
                pSizeOld->Release();
            }
            else
            {
                sizeDesired = LocLastDesiredSizeConstraint;
            }
            return sizeDesired;
        }
        
        PropertyCustomCacheResult __fastcall Element::GetExtentPropertyCustomCache(PropertyCustomCacheActionMode _eMode, PropertyCustomCacheActionInfo* _pInfo)
        {
            if (_eMode == PropertyCustomCacheActionMode::GetValue)
            {
                auto& pExtentValue = _pInfo->GetValueInfo.pRetValue;

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

        void __fastcall Element::UpdateLayoutPosition(POINT _LayoutPosition)
        {
            if (LocPosInLayout.x == _LayoutPosition.x && LocPosInLayout.y == _LayoutPosition.y)
                return;

            auto _pPointOld = Value::CreatePoint(LocPosInLayout);
            if (!_pPointOld)
                return;

            auto _pPointNew = Value::CreatePoint(_LayoutPosition);
            if (_pPointNew)
            {
                PreSourceChange(Element::g_ClassInfoData.PosInLayoutProp, PropertyIndicies::PI_Local, _pPointOld, _pPointNew);

                LocPosInLayout = _LayoutPosition;

                PostSourceChange();

                _pPointNew->Release();
            }

            _pPointOld->Release();
        }

        void __fastcall Element::UpdateLayoutSize(SIZE _LayoutSize)
        {
            if (LocSizeInLayout.cx == _LayoutSize.cx && LocSizeInLayout.cy == _LayoutSize.cy)
                return;
            
            auto _pSizeOld = Value::CreateSize(LocSizeInLayout);
            if (!_pSizeOld)
                return;

            auto _pSizeNew = Value::CreateSize(_LayoutSize);
            if (_pSizeNew)
            {
                PreSourceChange(Element::g_ClassInfoData.SizeInLayoutProp, PropertyIndicies::PI_Local, _pSizeOld, _pSizeNew);

                LocSizeInLayout = _LayoutSize;

                PostSourceChange();

                _pSizeNew->Release();
            }

            _pSizeOld->Release();
        }
	}
}
