#pragma once
#include <YY/Base/YY.h>
#include <MegaUI/Core/Value.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        enum class ValueType;
        class IControlInfo;
        class Value;
        class Element;
        struct PropertyInfo;
        struct DepRecs;
        class DeferCycle;

        struct EnumMap
        {
            raw_const_u8string_t pszEnum;
            int32_t nEnum;
        };

        enum class PropertyIndicies
        {
            // 本地值，当前 Element 直接拥有的值，也就是来自于 _LocalPropValue
            PI_Local = 0,
            // 特定值，优先尝试返回 _LocalPropValue 的值。如果未设置，那么按以下优先级返回：
            // 1. 如果属性标记 PF_Cascade，那么尝试返回来自于属性表的值。
            // 2. 如果属性标记 PF_Inherit，那么尝试返回来自与父Element的值。
            // 3. 如果还有没有，那么返回 Property 中的 pvDefault
            PI_Specified,
            // 实际计算后的值，大多与场合，它等价于 PI_Specified，因为大多数属性都没有 PF_TriLevel 标记
            // 如果属性标记 PF_TriLevel，那么它将返回一个递归计算后的值。
            // 比如说，Visible属性，如果父Element它是false，即使子为true，它也无济于事。
            // Visible是一个典型的需要递归评估的属性。
            PI_Computed,
            PI_MAX,
        };

        enum PropertyFlag
        {
            PF_HasLocal     = 0x00,
            PF_HasSpecified = 0x01,
            PF_HasComputed  = 0x02,

            // 仅支持 PI_Local
            PF_LocalOnly = PF_HasLocal,
            // 支持 PI_Local 以及 PI_Specified
            PF_Normal    = PF_HasLocal | PF_HasSpecified,
            // 支持 PI_Local、PI_Specified 以及 PI_Computed
            PF_TriLevel  = PF_HasLocal | PF_HasSpecified | PF_HasComputed,
            // 他是掩位，
            PF_TypeBits  = 0x03,

            // 如果值为Unset，那么尝试属性表（Property Sheet）种获取
            PF_Cascade   = 0x04,
            // 如果值为Unset，那么尝试从父节点种获取
            PF_Inherit   = 0x08,
            // 属性是只读的，不可更改。
            PF_ReadOnly  = 0x10,
            PF_20 = 0x20,
            PF_40 = 0x40,
            // 当DPI更新时，同步更新此属性
            PF_UpdateDpi = 0x80,
            // pFunDefaultValueEx 生效
            PF_GetDefaultValueEx = 0x100,
        };

        inline PropertyFlag __YYAPI PropertyIndiciesMapToPropertyFlag(PropertyIndicies _eIndicies)
        {
            switch (_eIndicies)
            {
            default:
            case PropertyIndicies::PI_Local:
                return PF_HasLocal;
                break;
            case PropertyIndicies::PI_Specified:
                return PropertyFlag(PF_HasLocal | PF_HasSpecified);
                break;
            case PropertyIndicies::PI_Computed:
                return PropertyFlag(PF_HasLocal | PF_HasSpecified | PF_HasComputed);
                break;
            }
        }

        inline PropertyIndicies __YYAPI PropertyFlagMapToMaxPropertyIndicies(PropertyFlag _fFlags)
        {
            if (_fFlags & PF_HasComputed)
                return PropertyIndicies::PI_Computed;
            else if (_fFlags & PF_HasSpecified)
                return PropertyIndicies::PI_Specified;
            else
                return PropertyIndicies::PI_Local;
        }

        enum PropertyGroup
        {
            // Normal priority
            PG_AffectsDesiredSize = 0x00000001,
            PG_AffectsParentDesiredSize = 0x00000002,
            PG_AffectsLayout = 0x00000004,
            PG_AffectsParentLayout = 0x00000008,

            PG_NormalPriMask = 0x0000FFFF,

            // Low priority
            PG_AffectsBounds = 0x00010000,
            PG_AffectsDisplay = 0x00020000,

            PG_LowPriMask = 0xFFFF0000,
        };

        enum class CustomPropertyHandleType
        {
            OnPropertyChanged,
            GetDependencies,
            GetValue,
            SetValue,
            // 快速Spec值比较，可以不支持。
            FastSpecValueCompare,
        };
        
        struct CustomPropertyBaseHandleData
        {
            const PropertyInfo* pProp;
            PropertyIndicies eIndicies;
        };

        enum class GetValueStyle : uint32_t
        {
            SkipNone = 0x00000000,
            // 跳过 _LocalPropValue 的查询。
            SkipLocalPropValue = 0x00000001,
            // 跳过从属性表的查询。
            SkipCascade = 0x00000002,
            // 跳过从父节点的查询
            SkipInherit = 0x00000004,
            SkipAll = 0xFFFFFFFF,
        };

        YY_APPLY_ENUM_CALSS_BIT_OPERATOR(GetValueStyle)

        struct OnPropertyChangedHandleData : public CustomPropertyBaseHandleData
        {
            static constexpr auto HandleType = CustomPropertyHandleType::OnPropertyChanged;

            Value OldValue;
            Value NewValue;
        };
        
        struct GetDependenciesHandleData : public CustomPropertyBaseHandleData
        {
            static constexpr auto HandleType = CustomPropertyHandleType::GetDependencies;

            DepRecs* pdr = nullptr;
            Value NewValue;
            DeferCycle* pDeferCycle = nullptr;
            int iPCSrcRoot = 0;

            struct
            {
                HRESULT hr = S_OK;
            } Output;
        };

        struct GetValueHandleData : public CustomPropertyBaseHandleData
        {
            static constexpr auto HandleType = CustomPropertyHandleType::GetValue;

            bool bUsingCache = true;
            
            struct
            {
                // 需要跳过的缓存类型
                GetValueStyle CacheResult = GetValueStyle::SkipNone;
                // 获取的值
                Value RetValue;
            } Output;
        };

        struct SetValueHandleData : public CustomPropertyBaseHandleData
        {
            static constexpr auto HandleType = CustomPropertyHandleType::SetValue;

            Value InputNewValue;
            struct
            {
                // 如果设置，成功，那么 >= 0。
                HRESULT hr = S_OK;
            } Output;
        };

        struct FastSpecValueCompareHandleData : public CustomPropertyBaseHandleData
        {
            static constexpr auto HandleType = CustomPropertyHandleType::FastSpecValueCompare;

            Element* pOther = nullptr;
            struct
            {
                // 返回1表示相等，返回0表示不相等，返回 -1 表示比较失败。
                int iResult = -1;
            } Output;
        };
        
        typedef Value(__YYAPI *GetPropertyDefaultValue)();
        typedef Value(__YYAPI *GetPropertyDefaultValueEx)(Element* _pElem, const PropertyInfo& _Prop);
        // 与 GetPropertyDefaultValueEx ABI兼容
        typedef Value(__YYAPI Element::*GetPropertyDefaultValueEx2)(const PropertyInfo& _Prop);

        typedef bool (__YYAPI Element::*FunTypeCustomPropertyHandle)(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

        typedef void (__YYAPI Element::*FunTypeOnPropertyChanged)(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

        struct DepRecs;
        class DeferCycle;

        typedef HRESULT (__YYAPI Element::*FunTypeGetDependencies)(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle);

        struct PropertyInfo
        {
            // 属性的名称，给XML解析器使用
            raw_const_u8string_t pszName;
            // PropertyFlag标记组合
            uint32_t fFlags;
            // PropertyGroup标记组合
            uint32_t fGroups;
            // 一串列表，以 eNull 结束
            const ValueType* ppValidValues;
            // 一串枚举值，以 { nullptr, 0 } 结束
            const EnumMap* pEnumMaps;
            // 默认值的初始化函数
            union
            {
                uintptr_t RawDefaultValue;
                GetPropertyDefaultValue pFunDefaultValue;
                GetPropertyDefaultValueEx pFunDefaultValueEx;
            };
            // 自定义属性处理器，比如接受事件更改、读取值等功能
            FunTypeCustomPropertyHandle pfnCustomPropertyHandle;
            // 此属性依赖的属性，以 nullptr 结束
            const PropertyInfo** ppDependencies;
            // 该属性绑定的缓存属性，如果没有绑定，那么直接使用 _LocalPropValue
            union
            {
                //占位
                uint64_t uRaw;
                struct
                {
                    // 当前缓存区的类型，等价于 ValueType
                    uint32_t eType : 7;
                    // 当 _eType 类型为 bool 时，表示bit中有效的标记位数
                    // 当 _eType 类型为 ValueType::Unit 时，0代表缓冲区类型为Unit，任何非0值都代表float类型
                    // 当 _eType 类型为 ValueType::UnitSize 时，0代表缓冲区类型为UnitSize，任何非0值都代表Size类型
                    // 当 _eType 类型为 ValueType::UnitRect 时，0代表缓冲区类型为UnitRect，任何非0值都代表Rect类型
                    uint32_t LocalValueBit : 3;
                    // 指向 Local 值缓冲区偏移
                    uint32_t OffsetToLocalValue : 11;
                    // 指向 Specified 值缓冲区偏移
                    uint32_t OffsetToSpecifiedValue : 11;
                    // 当 _eType 类型为 bool 时，表示bit中有效的标记位数
                    // 当 _eType 类型为 ValueType::Unit 时，0代表缓冲区类型为Unit，任何非0值都代表float类型
                    // 当 _eType 类型为 ValueType::UnitSize 时，0代表缓冲区类型为UnitSize，任何非0值都代表Size类型
                    // 当 _eType 类型为 ValueType::UnitRect 时，0代表缓冲区类型为UnitRect，任何非0值都代表Rect类型
                    uint32_t SpecifiedValueBit : 3;
                    // 指向 Computed 值缓冲区偏移
                    uint32_t OffsetToComputedValue : 11;
                    // 当 _eType 类型为 bool 时，表示bit中有效的标记位数
                    // 当 _eType 类型为 ValueType::Unit 时，0代表缓冲区类型为Unit，任何非0值都代表float类型
                    // 当 _eType 类型为 ValueType::UnitSize 时，0代表缓冲区类型为UnitSize，任何非0值都代表Size类型
                    // 当 _eType 类型为 ValueType::UnitRect 时，0代表缓冲区类型为UnitRect，任何非0值都代表Rect类型
                    uint32_t ComputedValueBit : 3;
                    // 如果为 0，那么值表示函数指针 _pFunPropertyCustomCache
                    // 如果为 1，那么表示 这个 struct 生效
                    uint32_t bValueMapOrCustomPropFun : 1;
                };
            } BindCacheInfo;
        };

        inline bool operator==(const PropertyInfo& _Left, const PropertyInfo& _Right)
        {
            return &_Left == &_Right;
        }

        template<class _T1, class _T2>
        __inline constexpr _T1 __GetMegaUICallback(_T2 p2)
        {
            union
            {
                _T2 p2;
                _T1 p1;
            } __data{ p2 };

            return __data.p1;
        }
        template<class _Type>
        __inline constexpr uint32_t __GetMegaUIDefaultValueFlag(_Type p2);
        
        template<>
        __inline constexpr uint32_t __GetMegaUIDefaultValueFlag(std::nullptr_t)
        {
            return 0;
        }

        template<>
        __inline constexpr uint32_t __GetMegaUIDefaultValueFlag(GetPropertyDefaultValueEx _pfn)
        {
            return PF_GetDefaultValueEx;
        }

        template<>
        __inline constexpr uint32_t __GetMegaUIDefaultValueFlag(GetPropertyDefaultValueEx2 _pfn)
        {
            return PF_GetDefaultValueEx;
        }

        template<>
        __inline constexpr uint32_t __GetMegaUIDefaultValueFlag(GetPropertyDefaultValue _pfn)
        {
            return 0;
        }

#define _MEGA_UI_PROP_BIND_VALUE(_VALUE_TYPE, _LOCAL_BIT, _SPECIFIED_BIT, _COMPUTED_BIT) \
        { (uint64_t(_VALUE_TYPE) << 0) | (uint64_t((_LOCAL_BIT) % 8) << 7) | (uint64_t((_LOCAL_BIT) / 8) << 10)               \
          | (uint64_t((_SPECIFIED_BIT) / 8) << 21) | (uint64_t((_SPECIFIED_BIT) % 8) << 32) | (uint64_t((_COMPUTED_BIT) / 8) << 35) \
          | (uint64_t((_COMPUTED_BIT) % 8) << 46) | (uint64_t(1) << 49)}

#define _MEGA_UI_PROP_BIND_INT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::int32_t, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_FLOAT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::float_t, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_UNIT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::Unit, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

        // 如果XXX_IS_FLOAT 为0，那么表示是Unit类型，否则是float类型
#define _MEGA_UI_PROP_BIND_UNIT_EX(_LOCAL, _LOCAL_IS_FLOAT, _SPECIFIED, _SPECIFIED_IS_FLOAT, _COMPUTED, _COMPUTED_IS_FLOAT) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::Unit, _LOCAL * 8 | _LOCAL_IS_FLOAT, _SPECIFIED * 8 | _SPECIFIED_IS_FLOAT, _COMPUTED * 8 | _COMPUTED_IS_FLOAT)

#define _MEGA_UI_PROP_BIND_STRING(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::String, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_ELEMENT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::Element, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_POINT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::Point, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_SIZE(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::UnitSize, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

        // 如果XXX_IS_SIZE 为0，那么表示是UnitSize类型，否则是Size类型
#define _MEGA_UI_PROP_BIND_SIZE_EX(_LOCAL, _LOCAL_IS_SIZE, _SPECIFIED, _SPECIFIED_IS_SIZE, _COMPUTED, _COMPUTED_IS_SIZE) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::UnitSize, _LOCAL * 8 | _LOCAL_IS_SIZE, _SPECIFIED * 8 | _SPECIFIED_IS_SIZE, _COMPUTED * 8 | _COMPUTED_IS_SIZE)

#define _MEGA_UI_PROP_BIND_RECT(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::UnitRect, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

        // 如果XXX_IS_RECT 为0，那么表示是UnitRect类型，否则是Rect类型
#define _MEGA_UI_PROP_BIND_RECT_EX(_LOCAL, _LOCAL_IS_RECT, _SPECIFIED, _SPECIFIED_IS_RECT, _COMPUTED, _COMPUTED_IS_RECT) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::UnitRect, _LOCAL * 8 | _LOCAL_IS_RECT, _SPECIFIED * 8 | _SPECIFIED_IS_RECT, _COMPUTED * 8 | _COMPUTED_IS_RECT)

#define _MEGA_UI_PROP_BIND_BOOL(_LOCAL_BIT, _SPECIFIED_BIT, _COMPUTED_BIT) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::boolean, _LOCAL_BIT, _SPECIFIED_BIT, _COMPUTED_BIT)

#define _MEGA_UI_PROP_BIND_ATOM(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::ATOM, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_SHEET(_LOCAL, _SPECIFIED, _COMPUTED) \
    _MEGA_UI_PROP_BIND_VALUE(ValueType::StyleSheet, _LOCAL * 8, _SPECIFIED * 8, _COMPUTED * 8)

#define _MEGA_UI_PROP_BIND_NONE() {}

#define _MEGA_UI_PROP_BIND_CUSTOM(_FUN) (__GetMegaUICallback<uint64_t>(_FUN))


//#define _APPLY_MEGA_UI_PROPERTY_EXTERN(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _ENUM, ...) static PropertyInfo _YY_CONCATENATE(_PRO_NAME, Prop);
#define _APPLY_MEGA_UI_PROPERTY_EXTERN(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _CUSTOM_PRO_HANDLE, _PROP_DEPENDENCIES_DATA, _ENUM, _BIND_INT, ...) const PropertyInfo _YY_CONCATENATE(_PRO_NAME, Prop);
#define _APPLY_MEGA_UI_PROPERTY_COUNT(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _CUSTOM_PRO_HANDLE, _PROP_DEPENDENCIES_DATA, _ENUM, _BIND_INT, ...) + 1
        //		PropertyInfo _CLASS::_YY_CONCATENATE(_PRO_NAME, Prop) =                                                

#define _APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _CUSTOM_PRO_HANDLE, _PROP_DEPENDENCIES_DATA, _ENUM, _BIND_INT, ...) \
        static constexpr const ValueType _YY_CONCATENATE(vv, _PRO_NAME)[] = {__VA_ARGS__, ValueType::Null};

#define _APPLY_MEGA_UI_PROPERTY(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _CUSTOM_PRO_HANDLE, _PROP_DEPENDENCIES_DATA, _ENUM, _BIND_INT, ...) \
        {                                                                                                         \
            u8## #_PRO_NAME,                                                                                          \
            _FLAGS | __GetMegaUIDefaultValueFlag(_DEF_VALUE_FUN),                                                 \
            _GROUPS,                                                                                              \
            _YY_CONCATENATE(vv, _PRO_NAME),                                                                       \
            _ENUM,                                                                                                \
            { __GetMegaUICallback<uintptr_t>(_DEF_VALUE_FUN) },                                                   \
            __GetMegaUICallback<FunTypeCustomPropertyHandle>(_CUSTOM_PRO_HANDLE),                                 \
            _PROP_DEPENDENCIES_DATA,                                                                              \
            _BIND_INT,                                                                                            \
        },

#define _APPLY_MEGA_UI_BITMAP_INFO(_BIT_NAME, _BIT_COUNT) uint8_t _BIT_NAME[_BIT_COUNT];
#define _APPLY_MEGA_UI_BITMAP_NAME(_BIT_NAME, _BIT_COUNT) uint32_t _BIT_NAME : _BIT_COUNT;

#define _APPLY_MEGA_UI_BITMAP_TABLE(_BITMAP_NAME, _BITMAP_TABLE) \
        struct _YY_CONCATENATE(_BITMAP_NAME, BitInfo)            \
        {                                                        \
            _BITMAP_TABLE(_APPLY_MEGA_UI_BITMAP_INFO)            \
        };                                                       \
        union                                                    \
        {                                                        \
            struct                                               \
            {                                                    \
                _BITMAP_TABLE(_APPLY_MEGA_UI_BITMAP_NAME)        \
            };                                                   \
            uint8_t _BITMAP_NAME[1];                             \
        };

        #define UFIELD_BITMAP_OFFSET(_TYPE, _FIELD, _BIT_NAME) (YY_UFIELD_OFFSET(_TYPE, _FIELD) * 8 + YY_UFIELD_OFFSET(_TYPE::_YY_CONCATENATE(_FIELD, BitInfo), _BIT_NAME))
    } //namespace MegaUI
}//namespace YY

#pragma pack(pop)
