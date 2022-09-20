#pragma once
#include "..\base\MegaUITypeInt.h"
#include "../base/StringBase.h"
#include "value.h"
#include "../base/DynamicArray.h"
#include "../base/ArrayView.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class IClassInfo;
        struct Cond;
        struct Decl;
        class Element;
        struct PropertyInfo;
        struct DepRecs;
        class DeferCycle;

        struct Decl
        {
            // 0
            const PropertyInfo* pProp;
            // 4
            Value Value;
        };

        struct Cond
        {
            // 0
            const PropertyInfo* pProp;
            // 0x4
            //int Type;
            ValueCmpOperation OperationType;

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
            DynamicArray<Cond, true> CondArray;
            // A

            CondMap(const Value& _CondValue, uint32_t _uSpecif, const DynamicArray<Cond, true>& _CondArray)
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

            CondMap(CondMap&& _Other)
                : CondValue(std::move(_Other.CondValue))
                , uSpecif(_Other.uSpecif)
                , CondArray(std::move(_Other.CondArray))
            {
                _Other.uSpecif = 0;
            }

            CondMap& __MEGA_UI_API operator=(const CondMap& _Other)
            {
                if (this != &_Other)
                {
                    CondValue = _Other.CondValue;
                    uSpecif = _Other.uSpecif;
                    CondArray.SetArray(_Other.CondArray);
                }

                return *this;
            }
            
            CondMap& __MEGA_UI_API operator=(CondMap&& _Other)
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
            DynamicArray<CondMap> CondMapList;
            // 0x12
            DynamicArray<const PropertyInfo*> DependencyPropList;

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

            PropertyData& __MEGA_UI_API operator=(const PropertyData& _Other)
            {
                if (this != &_Other)
                {
                    pRefProp = _Other.pRefProp;
                    CondMapList.SetArray(_Other.CondMapList);
                    DependencyPropList.SetArray(_Other.DependencyPropList);
                }
                return *this;
            }
            
            PropertyData& __MEGA_UI_API operator=(PropertyData&& _Other) noexcept
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

            __inline HRESULT __MEGA_UI_API AddCondMapping(const DynamicArray<Cond, true>& _CondArray, uint32_t _uSpecifId, const Value& _Value)
            {
                auto _pCondMap = CondMapList.EmplacePtr(_Value, _uSpecifId, _CondArray);
                if (!_pCondMap)
                    return E_OUTOFMEMORY;

                return S_OK;
            }
        };

        class ClassData
        {
        public:
            // 0
            IClassInfo* pClassInfo;

            // 4
            DynamicArray<const PropertyInfo*> DependencyPropList;
            // 0xC
            DynamicArray<PropertyData> PropertyDataRef;
            // 0x44

        public:
            ClassData(IClassInfo* _pClassInfo = nullptr)
                : pClassInfo(_pClassInfo)
            {
            }

            PropertyData* __MEGA_UI_API GetPropertyData(const PropertyInfo* _pRefProp)
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

            HRESULT __thiscall AddPropertyData(PropertyData** _ppPropertyData, const PropertyInfo* _pProp)
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
            DynamicArray<ClassData> ClassArray;
            // 0x50
            // DynamicArray<DynamicArray<Cond>> CondData;
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

            uint32_t __MEGA_UI_API AddRef();

            uint32_t __MEGA_UI_API Release();

            // 1
            HRESULT __MEGA_UI_API AddRule(uString szRule, IClassInfo* pClassInfo, DynamicArray<Cond, true> CondArray, const ArrayView<Decl>& DeclArray);
            // 2
            void __MEGA_UI_API MakeImmutable();
            // 3
            Value __MEGA_UI_API GetSheetValue(Element* _pElement, const PropertyInfo* _pProp);
            // 4
            HRESULT __MEGA_UI_API GetSheetDependencies(Element* _pElement, const PropertyInfo* _pProp, DepRecs* _pdr, DeferCycle* _pDeferCycle);
            // 5
            HRESULT __MEGA_UI_API GetSheetScope(Element* _pElement, DepRecs* pdr, DeferCycle* _pDeferCycle);
            // 6
            u8String __MEGA_UI_API GetSheetResourceID();

            // 7
            HRESULT __MEGA_UI_API SetSheetResourceID(u8String _szSheetResourceID);
            
        private:
            ClassData* __MEGA_UI_API GetClassData(IClassInfo* pClassInfo);

            HRESULT __MEGA_UI_API AddClassData(ClassData** ppData, IClassInfo* pClassInfo);
            
            static uint32_t __MEGA_UI_API ComputeSpecif(const DynamicArray<Cond, true>& CondArray, IClassInfo* _pClassInfo, uint32_t _uRuleId);
        };
    }
} // namespace YY

#pragma pack(pop)
