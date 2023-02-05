#pragma once
#include "..\base\MegaUITypeInt.h"
#include "../base/StringBase.h"
#include "value.h"
#include <Base/Containers/Array.h>
#include <Base/Containers/ArrayView.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class IControlInfo;
        struct Cond;
        struct Decl;
        class Element;
        struct PropertyInfo;
        struct DepRecs;
        class DeferCycle;

        struct Decl
        {
            // 0
            const PropertyInfo* pProp = nullptr;
            // 4
            Value Value;
        };

        struct Cond
        {
            // 0
            const PropertyInfo* pProp = nullptr;
            // 0x4
            //int Type;
            ValueCmpOperation OperationType = ValueCmpOperation::Invalid;

            // 0x8 表达式的值
            Value Value;
            // 0xC
        };
        
        struct CondMap
        {
            // 0
            // 表达式存储的值
            Value CondValue;
            // 4
            uint32_t uSpecif;
            // 8
            // WORD m8; 
            // 与微软原版不同，因为 我们的DynamicArray支持引用计数，额外一次寻址很显然是没必要的。
            Array<Cond> CondArray;
            // A

            CondMap(const Value& _CondValue, uint32_t _uSpecif, const Array<Cond>& _CondArray)
                : CondValue(_CondValue)
                , uSpecif(_uSpecif)
                , CondArray(_CondArray)
            {
            }

            CondMap(const CondMap& _Other)
                : CondValue(_Other.CondValue)
                , uSpecif(_Other.uSpecif)
                , CondArray(_Other.CondArray)
            {
            }

            CondMap(CondMap&& _Other) noexcept
                : CondValue(std::move(_Other.CondValue))
                , uSpecif(_Other.uSpecif)
                , CondArray(std::move(_Other.CondArray))
            {
                _Other.uSpecif = 0;
            }

            CondMap& __YYAPI operator=(const CondMap& _Other)
            {
                if (this != &_Other)
                {
                    CondValue = _Other.CondValue;
                    uSpecif = _Other.uSpecif;
                    CondArray.SetArray(_Other.CondArray);
                }

                return *this;
            }
            
            CondMap& __YYAPI operator=(CondMap&& _Other) noexcept
            {
                if (this != &_Other)
                {
                    CondValue = std::move(_Other.CondValue);
                    uSpecif = _Other.uSpecif;
                    _Other.uSpecif = 0;

                    CondArray.SetArray(std::move(_Other.CondArray));
                }

                return *this;
            }
        };

        class PropertyData
        {
        public:
            // int nValue;
            const PropertyInfo* pRefProp;

            // 4
            Array<CondMap> CondMapList;
            // 0x12
            Array<const PropertyInfo*, AllocPolicy::SOO> DependencyPropList;

            PropertyData(const PropertyInfo* _pRefProp)
                : pRefProp(_pRefProp)
            {
            }

            PropertyData(const PropertyData& _Other) = default;

            PropertyData(PropertyData&& _Other) noexcept
                : pRefProp(_Other.pRefProp)
                , CondMapList(std::move(_Other.CondMapList))
                , DependencyPropList(std::move(_Other.DependencyPropList))
            {
                _Other.pRefProp = nullptr;
            }

            PropertyData& __YYAPI operator=(const PropertyData& _Other)
            {
                if (this != &_Other)
                {
                    pRefProp = _Other.pRefProp;
                    CondMapList.SetArray(_Other.CondMapList);
                    DependencyPropList.SetArray(_Other.DependencyPropList);
                }
                return *this;
            }
            
            PropertyData& __YYAPI operator=(PropertyData&& _Other) noexcept
            {
                if (this != &_Other)
                {
                    pRefProp = _Other.pRefProp;
                    _Other.pRefProp = nullptr;

                    CondMapList.SetArray(std::move(_Other.CondMapList));
                    DependencyPropList.SetArray(std::move(_Other.DependencyPropList));
                }
                return *this;
            }

            __inline HRESULT __YYAPI AddCondMapping(const Array<Cond>& _CondArray, uint32_t _uSpecifId, const Value& _Value)
            {
                auto _pCondMap = CondMapList.EmplacePtr(_Value, _uSpecifId, _CondArray);
                if (!_pCondMap)
                    return E_OUTOFMEMORY;

                return S_OK;
            }
        };

        class ControlStyleData
        {
        public:
            // 0
            IControlInfo* pControlInfo;

            // 4
            Array<const PropertyInfo*> DependencyPropList;
            // 0xC
            Array<PropertyData, AllocPolicy::SOO> PropertyDataRef;
            // 0x44

        public:
            ControlStyleData(IControlInfo* _pControlInfo = nullptr)
                : pControlInfo(_pControlInfo)
            {
            }

            PropertyData* __YYAPI GetPropertyData(_In_ const PropertyInfo* _pRefProp)
            {
                if (!_pRefProp)
                    return nullptr;

                for (auto& Data : PropertyDataRef)
                {
                    if (Data.pRefProp == _pRefProp)
                        return &Data;
                }

                return nullptr;
            }

            HRESULT __thiscall AddPropertyData(_Outptr_ PropertyData** _ppPropertyData, _In_ const PropertyInfo* _pProp)
            {
                if (!_ppPropertyData)
                    return E_INVALIDARG;
                *_ppPropertyData = nullptr;

                if (!_pProp)
                    return E_INVALIDARG;

                auto _pPropertyData = PropertyDataRef.EmplacePtr(_pProp);
                if (!_pPropertyData)
                    return E_OUTOFMEMORY;

                *_ppPropertyData = _pPropertyData;
                return S_OK;
            }
        };

        class StyleSheet
        {
        protected:
            // MegaUI 扩展
            uint32_t uRef;

            // 0
            uint32_t uRuleId;
            // 8
            Array<ControlStyleData, AllocPolicy::SOO> ControlStyleDataArray;
            // 0x50
            // Array<Array<Cond>> CondData;
            // 0x5C
            //BYTE m5C;
            //BYTE m5D;
            //BYTE m5E;
            //BYTE m5F;
            // 0x60
            u8String szSheetResourceID;

            bool bMakeImmutable;
        public:
            // void* _vftable_
            StyleSheet();

            StyleSheet(StyleSheet const&) = delete;
            StyleSheet& operator=(StyleSheet const&) = delete;

            uint32_t __YYAPI AddRef();

            uint32_t __YYAPI Release();

            // 1
            HRESULT __YYAPI AddRule(uString szRule, IControlInfo* _pControlInfo, Array<Cond> CondArray, const ArrayView<Decl>& DeclArray);
            // 2
            void __YYAPI MakeImmutable();
            // 3
            Value __YYAPI GetSheetValue(_In_ Element* _pElement, _In_ const PropertyInfo* _pProp);
            // 4
            HRESULT __YYAPI GetSheetDependencies(_In_ Element* _pElement, _In_ const PropertyInfo* _pProp, _Inout_ DepRecs* _pdr, _In_ DeferCycle* _pDeferCycle);
            // 5
            HRESULT __YYAPI GetSheetScope(_In_ Element* _pElement, _Inout_ DepRecs* pdr, _In_ DeferCycle* _pDeferCycle);
            // 6
            u8String __YYAPI GetSheetResourceID();

            // 7
            HRESULT __YYAPI SetSheetResourceID(u8String _szSheetResourceID);
            
        private:
            _Ret_maybenull_ ControlStyleData* __YYAPI GetControlStyleData(_In_ IControlInfo* _pControlInfo);

            HRESULT __YYAPI AddControlStyleData(_Outptr_ ControlStyleData** _ppData, _In_ IControlInfo* _pControlInfo);
            
            /// <summary>
            /// 计算某条规则的总体权重值。
            /// </summary>
            /// <param name="CondArray">规则需要匹配的条件，每个条件之间均为 AND 逻辑。
            /// 注意：ID权重是 0x8000，Class 权重是 0x4000，其他均为 1
            /// </param>
            /// <param name="_pControlInfo"></param>
            /// <param name="_uRuleId">规则序号，数值越大，优先级越高。</param>
            /// <returns>权重</returns>
            static uint32_t __YYAPI ComputeSpecif(_In_ const Array<Cond>& CondArray, _In_ IControlInfo* _pControlInfo, _In_ uint16_t _uRuleId);
        };
    }
} // namespace YY

#pragma pack(pop)
