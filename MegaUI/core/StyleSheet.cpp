#include "pch.h"
#include "StyleSheet.h"

#include "Element.h"
#include "../base/Interlocked.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        template<typename ArrayType>
        bool __MEGA_UI_API IsPIInList(const PropertyInfo* pProp, const ArrayType& Array)
        {
            for (auto pItem : Array)
            {
                if (pItem == pProp)
                    return true;
            }
            
            return false;
        }

        template<typename ArrayType>
        HRESULT __MEGA_UI_API AddDeps(ArrayType& Array, const ArrayView<Decl>& DeclArray)
        {
            for (auto& DeclItem : DeclArray)
            {
                if (!IsPIInList(DeclItem.pProp, Array))
                {
                    auto _pItem = Array.EmplacePtr(DeclItem.pProp);
                    if (!_pItem)
                        return E_OUTOFMEMORY;
                }
            }

            return S_OK;
        }

        StyleSheet::StyleSheet()
            : uRef(1u)
            , uRuleId(0u)
            , bMakeImmutable(false)
        {
        }
        
        uint32_t __MEGA_UI_API StyleSheet::AddRef()
        {
            return Interlocked::Increment(&uRef);
        }
        
        uint32_t __MEGA_UI_API StyleSheet::Release()
        {
            auto _uNewRef = Interlocked::Decrement(&uRef);
            if (_uNewRef == 0)
            {
                HFree(this);
            }
            return _uNewRef;
        }

        HRESULT __MEGA_UI_API StyleSheet::AddRule(uString _szRule, IClassInfo* _pClassInfo, DynamicArray<Cond, true> _CondArray, const ArrayView<Decl>& _DeclArray)
        {
            // 添加规则必须要有匹配条件
            if (_pClassInfo == nullptr || _DeclArray.GetSize() == 0)
                return S_FALSE;

            auto _pClassData = GetClassData(_pClassInfo);

            if (!_pClassData)
            {
                auto _hr = AddClassData(&_pClassData, _pClassInfo);
                if (FAILED(_hr))
                    return _hr;
            }

            const auto _uSpecifId = ComputeSpecif(_CondArray, _pClassInfo, uRuleId);
            bool bFaild = false;

            for (auto& _DeclItem : _DeclArray)
            {
                auto pPropertyData = _pClassData->GetPropertyData(_DeclItem.pProp);

                if (!pPropertyData)
                {
                    if (FAILED(_pClassData->AddPropertyData(&pPropertyData, _DeclItem.pProp)))
                        bFaild = true;
                }

                if (pPropertyData)
                {
                    if (FAILED(pPropertyData->AddCondMapping(_CondArray, _uSpecifId, _DeclItem.Value)))
                        bFaild = true;
                }
            }

            if (FAILED(AddDeps(_pClassData->DependencyPropList, _DeclArray)))
                bFaild = true;

            for (auto& _CondItem : _CondArray)
            {
                auto _pPropData = _pClassData->GetPropertyData(_CondItem.pProp);

                if (!_pPropData)
                {
                    if (FAILED(_pClassData->AddPropertyData(&_pPropData, _CondItem.pProp)))
                        bFaild = true;
                }

                if (_pPropData)
                {
                    if (FAILED(AddDeps(_pPropData->DependencyPropList, _DeclArray)))
                        bFaild = true;
                }
            }

            ++uRuleId;
            bMakeImmutable = true;
            return bFaild ? 0x800403EB : S_OK;
        }
        
        void __MEGA_UI_API StyleSheet::MakeImmutable()
        {
            if (!bMakeImmutable)
                return;

            bMakeImmutable = true;

            for (auto& _Class : ClassArray)
            {
                for (auto& PropertyData : _Class.PropertyDataRef)
                {
                    PropertyData.CondMapList.Sort([](const CondMap* _pLeft, const CondMap* _pRigth) -> int
                        {
                            // 故意颠倒，让大的在前，小的在后。
                            return _pRigth->uSpecif - _pLeft->uSpecif;
                        });
                }
            }
        }

        Value __MEGA_UI_API StyleSheet::GetSheetValue(Element* _pElement, const PropertyInfo* _pProp)
        {
            if (_pElement == nullptr || _pProp == nullptr)
                return Value::GetNull();

            auto pClassData = GetClassData(_pElement->GetControlClassInfo());

            if (!pClassData)
                return Value::GetUnset();

            auto pPropertyData = pClassData->GetPropertyData(_pProp);
            if (!pPropertyData)
                return Value::GetUnset();

            for(auto& _ConMap : pPropertyData->CondMapList)
            {
                //CondData.GetItemPtr(_ConMap.m8)
                const auto _uSize = _ConMap.CondArray.GetSize();
                for (uint_t _uIndex = 0u;; ++_uIndex)
                {
                    if (_uIndex == _uSize)
                    {
                        // 如果扫描到结束，那么说明命中条件。
                        // 这些规则只有 && 逻辑，引入 || 逻辑破坏性较大。
                        return _ConMap.CondValue;
                    }

                    
                    auto pCond = _ConMap.CondArray.GetItemPtr(_uIndex);
                    if (!pCond)
                        break;

                    auto _ElementValue = _pElement->GetValue(
                        *pCond->pProp,
                        PropertyFlagMapToMaxPropertyIndicies(PropertyFlag(pCond->pProp->fFlags & PropertyFlag::PF_TypeBits)),
                        false);

                    if (_ElementValue.GetType() != pCond->Value.GetType())
                    {
                        if (pCond->OperationType != ValueCmpOperation::NotEqual)
                            break;
                    }
                    else
                    {
                        if (!_ElementValue.CmpValue(pCond->Value, pCond->OperationType))
                            break;
                    }
                }
            }

            return Value::GetUnset();
        }

        HRESULT __MEGA_UI_API StyleSheet::GetSheetDependencies(Element* _pElement, const PropertyInfo* _pProp, DepRecs* _pdr, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pProp == nullptr || _pdr == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;
            auto _pClassInfo = GetClassData(_pElement->GetControlClassInfo());
            if (!_pClassInfo)
                return S_OK;

            auto _pPropData = _pClassInfo->GetPropertyData(_pProp);
            if (!_pPropData)
                return S_OK;
            
            HRESULT _hr = S_OK;

            for (auto _pDependencyProp : _pPropData->DependencyPropList)
            {
                auto _hrAdd = Element::AddDependency(_pElement, *_pDependencyProp, PropertyIndicies::PI_Specified, _pdr, _pDeferCycle);
                if (FAILED(_hrAdd))
                    _hr = _hrAdd;
            }

            return _hr;
        }

        HRESULT __MEGA_UI_API StyleSheet::GetSheetScope(Element* _pElement, DepRecs* _pDepRecs, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pDepRecs == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;

            auto _pClassInfo = GetClassData(_pElement->GetControlClassInfo());
            if (!_pClassInfo)
                return S_OK;

            HRESULT _hr = S_OK;

            for (auto _pDependencyProp : _pClassInfo->DependencyPropList)
            {
                auto _hrAdd = Element::AddDependency(_pElement, *_pDependencyProp, PropertyIndicies::PI_Specified, _pDepRecs, _pDeferCycle);
                if (FAILED(_hrAdd))
                    _hr = _hrAdd;
            }

            return _hr;
        }

        u8String __MEGA_UI_API StyleSheet::GetSheetResourceID()
        {
            return szSheetResourceID;
        }

        HRESULT __MEGA_UI_API StyleSheet::SetSheetResourceID(u8String _szSheetResourceID)
        {
            return szSheetResourceID.SetString(_szSheetResourceID);
        }
        
        ClassData* __MEGA_UI_API StyleSheet::GetClassData(IClassInfo* pClassInfo)
        {
            if (!pClassInfo)
                return nullptr;

            for (auto& ClassInfo : ClassArray)
            {
                if (ClassInfo.pClassInfo == pClassInfo)
                    return &ClassInfo;
            }

            return nullptr;
        }
        
        HRESULT __MEGA_UI_API StyleSheet::AddClassData(ClassData** ppData, IClassInfo* pClassInfo)
        {
            if (!ppData)
                return E_INVALIDARG;
            *ppData = nullptr;

            if (!pClassInfo)
                return E_INVALIDARG;

            auto pInfo = ClassArray.EmplacePtr(pClassInfo);
            if (!pInfo)
                return E_OUTOFMEMORY;

            *ppData = pInfo;

            return S_OK;
        }
        
        uint32_t __MEGA_UI_API StyleSheet::ComputeSpecif(const DynamicArray<Cond, true>& CondArray, IClassInfo* _pClassInfo, uint32_t _uRuleId)
        {
            union
            {
                WORD ID = 0;

                struct
                {
                    BYTE l;
                    BYTE h;
                };
            } Data;

            for (auto& CondItem : CondArray)
            {
                if (CondItem.pProp == &Element::g_ClassInfoData.IDProp)
                    ++Data.h;

                ++Data.l;
            }

            return MAKELONG(_uRuleId, Data.ID);
        }
    } // namespace MegaUI
} // namespace YY