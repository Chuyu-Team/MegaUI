#include "pch.h"
#include "StyleSheet.h"

#include <MegaUI/Core/Element.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/RefPtr.h>

#include <MegaUI/Parser/UIParser.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {
        static Array<RefPtr<StyleSheet>> g_GlobalStyleSheet;

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

        StyleSheet::StyleSheet(StyleSheet* _pInheritStyle)
            : uRuleId(0u)
            , pInheritStyle(_pInheritStyle)
            , bMakeImmutable(false)
        {
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
                    if (FAILED(pPropertyData->AddCondMapping(_CondArray, _uSpecifId, _DeclItem.oValue)))
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

            do
            {
                auto pClassData = GetControlStyleData(_pElement->GetControlInfo());

                if (!pClassData)
                    break;

                auto pPropertyData = pClassData->GetPropertyData(_pProp);
                if (!pPropertyData)
                    break;

                for (auto& _ConMap : pPropertyData->CondMapList)
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

                        if (_ElementValue.GetType() != pCond->CondValue.GetType())
                        {
                            if (pCond->OperationType != ValueCmpOperation::NotEqual)
                                break;
                        }
                        else
                        {
                            if (!_ElementValue.CmpValue(pCond->CondValue, pCond->OperationType, false))
                                break;
                        }
                    }
                }
            } while (false);

            if (pInheritStyle)
                return pInheritStyle->GetSheetValue(_pElement, _pProp);

            return Value::CreateUnset();
        }

        HRESULT __YYAPI StyleSheet::GetSheetDependencies(Element* _pElement, const PropertyInfo* _pProp, DepRecs* _pdr, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pProp == nullptr || _pdr == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;

            do
            {
                auto _pControlStyleData = GetControlStyleData(_pElement->GetControlInfo());
                if (!_pControlStyleData)
                    break;

                auto _pPropData = _pControlStyleData->GetPropertyData(_pProp);
                if (!_pPropData)
                    break;

                for (auto _pDependencyProp : _pPropData->DependencyPropList)
                {
                    auto _hr = Element::AddDependency(_pElement, *_pDependencyProp, PropertyIndicies::PI_Specified, _pdr, _pDeferCycle);
                    if (FAILED(_hr))
                        return _hr;
                }
            } while (false);

            if (pInheritStyle)
                return pInheritStyle->GetSheetDependencies(_pElement, _pProp, _pdr, _pDeferCycle);

            return S_OK;
        }

        HRESULT __YYAPI StyleSheet::GetSheetScope(Element* _pElement, DepRecs* _pDepRecs, DeferCycle* _pDeferCycle)
        {
            if (_pElement == nullptr || _pDepRecs == nullptr || _pDeferCycle == nullptr)
                return E_INVALIDARG;

            do
            {
                auto _pControlStyleData = GetControlStyleData(_pElement->GetControlInfo());
                if (!_pControlStyleData)
                    break;

                for (auto _pDependencyProp : _pControlStyleData->DependencyPropList)
                {
                    auto _hr = Element::AddDependency(_pElement, *_pDependencyProp, PropertyIndicies::PI_Specified, _pDepRecs, _pDeferCycle);
                    if (FAILED(_hr))
                        return _hr;
                }
            } while (false);

            if (pInheritStyle)
                return pInheritStyle->GetSheetScope(_pElement, _pDepRecs, _pDeferCycle);

            return S_OK;
        }

        u8String __YYAPI StyleSheet::GetSheetResourceID()
        {
            return szSheetResourceID;
        }

        HRESULT __YYAPI StyleSheet::SetSheetResourceID(u8String _szSheetResourceID)
        {
            return szSheetResourceID.SetString(_szSheetResourceID);
        }

        HRESULT StyleSheet::SetInherit(StyleSheet* _pInheritStyle)
        {
            if (_pInheritStyle)
                _pInheritStyle->AddRef();

            if (pInheritStyle)
                pInheritStyle->Release();

            pInheritStyle = _pInheritStyle;

            return S_OK;
        }

        HRESULT StyleSheet::GetGlobalStyleSheet(u8StringView _szSheetResourceID, StyleSheet** _ppSheet)
        {
            *_ppSheet = nullptr;

            for (StyleSheet* _pSheet : g_GlobalStyleSheet)
            {
                auto _szDstId = _pSheet->GetSheetResourceID();
                if (_szDstId.GetSize() != _szSheetResourceID.GetSize())
                    continue;

                if (_szDstId.CompareI(_szSheetResourceID) == 0)
                {
                    _pSheet->AddRef();
                    *_ppSheet = _pSheet;
                    return S_OK;
                }
            }

            return E_NOT_SET;
        }

        HRESULT StyleSheet::AddGlobalStyleSheet(u8String&& _szXmlString)
        {
            UIParser _Tmp;
            auto _hr = _Tmp.ParserByXmlString(std::move(_szXmlString));
            if (FAILED(_hr))
                return _hr;

            auto _AppendStyleSheets = _Tmp.GetAllStyleSheet();

            if (g_GlobalStyleSheet.GetSize() == 0)
            {
                g_GlobalStyleSheet = std::move(_AppendStyleSheets);
                return S_OK;
            }
            else
            {
                return g_GlobalStyleSheet.Add(_AppendStyleSheets.GetData(), _AppendStyleSheets.GetSize());
            }
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

            if (_uWeight > (std::numeric_limits<uint16_t>::max)())
                _uWeight = (std::numeric_limits<uint16_t>::max)();

            return _uRuleId | (_uWeight << 16);
        }
    } // namespace MegaUI
} // namespace YY