#pragma once
#include <vcruntime.h>
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
	namespace MegaUI
	{
		enum class ValueType;
		class IClassInfo;
		class Value;
		class Element;

		struct EnumMap
		{
			raw_const_astring_t pszEnum;
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
			// 仅支持 PI_Local
			PF_LocalOnly = 0x00,
			// 支持 PI_Local 以及 PI_Specified
			PF_Normal    = 0x01,
			// 支持 PI_Local、PI_Specified 以及 PI_Computed
			PF_TriLevel  = 0x02,
			// 他是掩位，
			PF_TypeBits  = 0x03,

			// 如果值为Unset，那么尝试属性表（Property Sheet）种获取
			PF_Cascade   = 0x04,
			// 如果值为Unset，那么尝试从父节点种获取
			PF_Inherit   = 0x08,
			// 属性是只读的，不可更改。
			PF_ReadOnly  = 0x10,

		};

		enum PropertyGroup
		{
			// Normal priority
			PG_AffectsDesiredSize       = 0x00000001,
			PG_AffectsParentDesiredSize = 0x00000002,
			PG_AffectsLayout            = 0x00000004,
			PG_AffectsParentLayout      = 0x00000008,

			PG_NormalPriMask            = 0x0000FFFF,

			// Low priority
			PG_AffectsBounds            = 0x00010000,    
			PG_AffectsDisplay           = 0x00020000,

			PG_LowPriMask               = 0xFFFF0000,
		};

		enum class PropertyCustomCacheActionMode
		{
			GetValue = 0,
			UpdateValue,
		};

		struct PropertyInfo;

		struct PropertyCustomCacheActionInfo
		{
			const PropertyInfo* pProp;
			PropertyIndicies eIndicies;
			union
			{
				// GetValue
				struct
				{
					Value* pRetValue;
					bool bUsingCache;
				} GetValueInfo;

				// UpdateValue
				struct
				{
					Value* pNewValue;
				} UpdateValueInfo;
			};
		};

		enum PropertyCustomCacheResult
		{
			SkipNone           = 0x00000000,
			// 跳过 _LocalPropValue 的查询。
			SkipLocalPropValue = 0x00000001,
			// 跳过从属性表的查询。
			SkipCascade        = 0x00000002,
			// 跳过从父节点的查询
			SkipInherit        = 0x00000004,
			SkipAll            = 0xFFFFFFFF,
		};

		typedef void (__fastcall Element::*FunTypeOnPropertyChanged)(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pOldValue, _In_ Value* _pNewValue);
        typedef PropertyCustomCacheResult (__fastcall Element::*FunTypePropertyCustomCache)(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCacheActionInfo* _pInfo);

		struct PropertyInfo
		{
			// 属性的名称，给XML解析器使用
			raw_const_astring_t pszName;
			// PropertyFlag标记组合
			uint32_t   fFlags;
			// PropertyGroup标记组合
			uint32_t   fGroups;
			// 一串列表，以 eNull 结束
			const ValueType* ppValidValues;
			// 一串枚举值，以 { nullptr, 0 } 结束
			const EnumMap* pEnumMaps;
			// 默认值的初始化函数
			Value* (__fastcall* pFunDefaultValue)();
			// 当值发生更改时，会调用此函数
			FunTypeOnPropertyChanged pFunOnPropertyChanged;
			// 该属性绑定的缓存属性，如果没有绑定，那么直接使用 _LocalPropValue
			union
			{
				//占位
				uint64_t uRaw;
				// 自定义的属性值缓存函数，当需要高度定制时，可以使用
				FunTypePropertyCustomCache pFunPropertyCustomCache;
				struct
				{
					// 如果为 0，那么值表示函数指针 _pFunPropertyCustomCache
					// 如果为 1，那么表示 这个 struct 生效
					uint16_t bValueMapOrCustomPropFun : 1;
					// 当前缓存区的类型，等价于 ValueType
					uint16_t eType : 7;
					// 当 _eType 类型为 bool 时使用，bit中有效的标记位数
					uint16_t LocalValueBit : 3;
					// 指向 Local 值缓冲区偏移
					uint16_t OffsetToLocalValue : 11;
					uint16_t HasLocalValueCacheBit : 3;
					// 指向一个bool值缓冲区，如果已经缓存了Local的结果，那么为 true
					uint16_t OffsetToHasLocalCache : 11;
					// 当 _eType 类型为 bool 时使用
					uint16_t SpecifiedValueBit : 3;
					// 指向 Specified 值缓冲区偏移
					uint16_t OffsetToSpecifiedValue : 11;
					uint16_t HasSpecifiedValueCacheBit : 3;
					// 指向一个bool值缓冲区，如果已经缓存了Specified的结果，那么为 true
					uint16_t OffsetToHasSpecifiedValueCache : 11;
				};
			} BindCacheInfo;
		};

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



#define _MEGA_UI_PROP_BIND_VALUE(_VALUE_TYPE, _LOCAL, _LOCAL_BIT, _HAS_LOCAL, _HAS_LOCAL_BIT, _SPECIFIED, _SPECIFIED_BIT, _HAS_SPECIFIED, _HAS_SPECIFIED_BIT) \
        { uint64_t(1) | (uint64_t((uint32_t)_VALUE_TYPE) << 1) | (uint64_t((uint32_t)_LOCAL_BIT) << 8) | (uint64_t((uint32_t)_LOCAL) << 11)               \
		  | (uint64_t((uint32_t)_HAS_LOCAL_BIT) << 22) | (uint64_t((uint32_t)_HAS_LOCAL) << 25) | (uint64_t((uint32_t)_SPECIFIED_BIT) << 36) \
          | (uint64_t((uint32_t)_SPECIFIED) << 39) | (uint64_t((uint32_t)_HAS_SPECIFIED_BIT) << 50) | (uint64_t((uint32_t)_HAS_SPECIFIED) << 53) }

#define _MEGA_UI_PROP_BIND_INT(_LOCAL, _HAS_LOCAL, _HAS_LOCAL_BIT, _SPECIFIED, _HAS_SPECIFIED, _HAS_SPECIFIED_BIT) \
		_MEGA_UI_PROP_BIND_VALUE(ValueType::int32_t, _LOCAL, 0, _HAS_LOCAL, _HAS_LOCAL_BIT, _SPECIFIED, 0, _HAS_SPECIFIED, _HAS_SPECIFIED_BIT)

#define _MEGA_UI_PROP_BIND_ELEMENT(_LOCAL, _HAS_LOCAL, _HAS_LOCAL_BIT, _SPECIFIED, _HAS_SPECIFIED, _HAS_SPECIFIED_BIT) \
		_MEGA_UI_PROP_BIND_VALUE(ValueType::Element, _LOCAL, 0, _HAS_LOCAL, _HAS_LOCAL_BIT, _SPECIFIED, 0, _HAS_SPECIFIED, _HAS_SPECIFIED_BIT)

		
//#define _APPLY_MEGA_UI_PROPERTY_EXTERN(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _ENUM, ...) static PropertyInfo _CRT_CONCATENATE(_PRO_NAME, Prop);
#define _APPLY_MEGA_UI_PROPERTY_EXTERN(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _PROP_CHANGED, _ENUM, _BIND_INT, ...) const PropertyInfo _CRT_CONCATENATE(_PRO_NAME, Prop);
#define _APPLY_MEGA_UI_PROPERTY_COUNT(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _PROP_CHANGED, _ENUM, _BIND_INT, ...) + 1
		//		PropertyInfo _CLASS::_CRT_CONCATENATE(_PRO_NAME, Prop) =                                                

#define _APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _PROP_CHANGED, _ENUM, _BIND_INT, ...) \
		static constexpr const ValueType _CRT_CONCATENATE(vv, _PRO_NAME)[] = { __VA_ARGS__, ValueType::Null };

#define _APPLY_MEGA_UI_PROPERTY(_PRO_NAME, _FLAGS, _GROUPS, _DEF_VALUE_FUN, _PROP_CHANGED, _ENUM, _BIND_INT, ...) \
		{                                                                                                         \
			# _PRO_NAME,                                                                                          \
			_FLAGS,                                                                                               \
			_GROUPS,                                                                                              \
			_CRT_CONCATENATE(vv, _PRO_NAME),                                                                      \
			_ENUM,                                                                                                \
			_DEF_VALUE_FUN,                                                                                       \
			__GetMegaUICallback<FunTypeOnPropertyChanged>(_PROP_CHANGED),                                         \
			_BIND_INT,                                                                                            \
		},




	} //namespace MegaUI
}//namespace YY

#pragma pack(pop)
