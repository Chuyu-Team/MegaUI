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
        bool __YYAPI IsPIInList(const PropertyInfo* pProp, const ArrayType& Array)
        {
            for (auto pItem : Array)
            {
                if (pItem == pProp)
                    return true;
            }
            
            return false;
        }

        template<typename ArrayType>
        HRESULT __YYAPI AddDeps(ArrayType& Array, const ArrayView<Decl>& DeclArray)
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
        
        uint32_t __YYAPI StyleSheet::AddRef()
        {
            return Interlocked::Increment(&uRef);
        }
        
        uint32_t __YYAPI StyleSheet::Release()
        {
            auto _uNewRef = Interlocked::Decrement(&uRef);
            if (_uNewRef == 0)
            {
                HFree(this);
            }
            return _uNewRef;
        }

        HRESULT __YYAPI StyleSheet::AddRule(uString _szRule, IControlInfo* _pControlInfo, Array<Cond> _CondArray, const ArrayView<Decl>& _DeclArray)
        {
            // 添加规则必须要有匹配条件
            if (_pControlInfo == nullptr || _DeclArray.GetSize() == 0)
                return S_FALSE;

            auto _pControlStyleData = GetControlStyleData(_pControlInfo);

            if (!_pControlStyleData)
            {
                auto _hr = AddControlStyleData(&_pControlStyleData, _pControlInfo);
                if (FAILED(_hr))
                    return _hr;
            }

            const auto _uSpecifId = ComputeSpecif(_CondArray, _pControlInfo, uRuleId);
            bool bFaild = false;

            for (auto& _DeclItem : _DeclArray)
            {
                auto pPropertyData = _pControlStyleData->GetPropertyData(_DeclItem.pProp);

                if (!pPropertyData)
                {
                    if (FAILED(_pControlStyleData->AddPropertyData(&pPropertyData, _DeclItem.pProp)))
                        bFaild = true;
                }

                if (pPropertyData)
                {
                    if (FAILED(pPropertyData->AddCondMapping(_CondArray, _uSpecifId, _DeclItem.Value)))
                        bFaild = true;
                }
            }

            if (FAILED(AddDeps(_pControlStyleData->DependencyPropList, _DeclArray)))
                bFaild = true;

            for (auto& _CondItem : _CondArray)
            {
                auto _pPropData = _pControlStyleData->GetPropertyData(_CondItem.pProp);

                if (!_pPropData)
                {
                    if (FAILED(_pControlStyleData->AddPropertyData(&_pPropData, _CondItem.pProp)))
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
        
        void __YYAPI StyleSheet::MakeImmutable()
        {
            if (!bMakeImmutable)
                return;

            bMakeImmutable = true;

            for (auto& _Class : ControlStyleDataArray)
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

        Value __YYAPI StyleSheet::GetSheetValue(Element* _pElement, const PropertyInfo* _pProp)
        {
            if (_pElement == nullptr || _pProp == nullptr)
                return Value::CreateNull();

            auto pClassData = GetControlStyleData(_pElement->GetControlInfo());

            if (!pClassData)
                return Value::CreateUnset();

            auto pPropertyData = pClassData->GetPropertyData(_pProp);
            if (!pPropertyData)
                return Value::CreateUnset();

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
                        return _ConMap.CondValue.UpdateDpi(_pElement->GetDpi());
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
                        if (!_ElementValue.CmpValue(pCond->Value, pCond->OperationType, false))
                            break;
                    }
                }
            }

            return Value::CreateUnset();
        }

        HRESULT __YYAPI StyleSheet::GetSheetDependencies(Element* _pElement, const PropertyInfo* _pProp, DepRecs* _pdr, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pProp == nullptr || _pdr == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;
            auto _pControlStyleData = GetControlStyleData(_pElement->GetControlInfo());
            if (!_pControlStyleData)
                return S_OK;

            auto _pPropData = _pControlStyleData->GetPropertyData(_pProp);
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

        HRESULT __YYAPI StyleSheet::GetSheetScope(Element* _pElement, DepRecs* _pDepRecs, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pDepRecs == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;

            auto _pControlStyleData = GetControlStyleData(_pElement->GetControlInfo());
            if (!_pControlStyleData)
                return S_OK;

            HRESULT _hr = S_OK;

            for (auto _pDependencyProp : _pControlStyleData->DependencyPropList)
            {
                auto _hrAdd = Element::AddDependency(_pElement, *_pDependencyProp, PropertyIndicies::PI_Specified, _pDepRecs, _pDeferCycle);
                if (FAILED(_hrAdd))
                    _hr = _hrAdd;
            }

            return _hr;
        }

        u8String __YYAPI StyleSheet::GetSheetResourceID()
        {
            return szSheetResourceID;
        }

        HRESULT __YYAPI StyleSheet::SetSheetResourceID(u8String _szSheetResourceID)
        {
            return szSheetResourceID.SetString(_szSheetResourceID);
        }
        
        ControlStyleData* __YYAPI StyleSheet::GetControlStyleData(IControlInfo* _pControlInfo)
        {
            if (!_pControlInfo)
                return nullptr;

            for (auto& _ControlStyleData : ControlStyleDataArray)
            {
                if (_ControlStyleData.pControlInfo == _pControlInfo)
                    return &_ControlStyleData;
            }

            return nullptr;
        }
        
        HRESULT __YYAPI StyleSheet::AddControlStyleData(ControlStyleData** ppData, IControlInfo* _pControlInfo)
        {
            if (!ppData)
                return E_INVALIDARG;
            *ppData = nullptr;

            if (!_pControlInfo)
                return E_INVALIDARG;

            auto pInfo = ControlStyleDataArray.EmplacePtr(_pControlInfo);
            if (!pInfo)
                return E_OUTOFMEMORY;

            *ppData = pInfo;

            return S_OK;
        }
        
        uint32_t __YYAPI StyleSheet::ComputeSpecif(const Array<Cond>& CondArray, IControlInfo* _pControlInfo, uint16_t _uRuleId)
        {
            uint32_t _uWeight = 0u;

            for (auto& CondItem : CondArray)
            {
                if (CondItem.pProp == &Element::g_ControlInfoData.IdProp)
                {
                    _uWeight += 0x8000u;
                }
                else if (CondItem.pProp == &Element::g_ControlInfoData.ClassProp)
                {
                    _uWeight += 0x4000u;
                }
                else
                {
                    _uWeight += 1u;
                }
            }

            if (_uWeight > uint16_max)
                _uWeight = uint16_max;

            return MAKELONG(_uRuleId, _uWeight);
        }
    } // namespace MegaUI
} // namespace YY