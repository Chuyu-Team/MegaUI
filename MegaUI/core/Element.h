﻿#pragma once
#include <Windows.h>
#include "value.h"
#include "Property.h"
#include "DeferCycle.h"

// Global layout positions
#define LP_None         -3
#define LP_Absolute     -2
#define LP_Auto         -1

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
	namespace MegaUI
	{
#define _APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(_CLASS_NAME, _BASE_CLASS, _CLASS_INFO_TYPE,                 \
		                                        _DEFAULT_CREATE_FLAGS, _PROPERTY_TABLE)                     \
		protected:                                                                                          \
			friend ClassInfoBase<_CLASS_NAME>;                                                              \
			struct StaticClassInfo                                                                          \
			{                                                                                               \
	            using ClassInfoType = _CLASS_INFO_TYPE;                                                     \
				using BaseElement =_BASE_CLASS;                                                             \
                constexpr static uint32_t fDefaultCreate = _DEFAULT_CREATE_FLAGS;                           \
				constexpr static raw_const_astring_t szClassName = # _CLASS_NAME;                           \
				constexpr static uint32_t uPropsCount = 0 _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_COUNT);   \
                                                                                                            \
				ClassInfoType* pClassInfoPtr;                                                               \
                                                                                                            \
				union                                                                                       \
				{                                                                                           \
					struct                                                                                  \
					{                                                                                       \
						_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_EXTERN);                                    \
					};                                                                                      \
                                                                                                            \
					const PropertyInfo Props[uPropsCount];                                                  \
				};                                                                                          \
			};                                                                                              \
                                                                                                            \
			static StaticClassInfo g_ClassInfoData;                                                         \
		public:                                                                                             \
			virtual IClassInfo* __fastcall GetElementClassInfo();                                           \
			static IClassInfo* __fastcall GetStaticElementClassInfo();                                      \
			static HRESULT __fastcall Register();                                                           \
			static HRESULT __fastcall UnRegister();


#define _APPLY_MEGA_UI_STATIC_CALSS_INFO(_CLASS_NAME, _PROPERTY_TABLE)   \
		_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST);        \
                                                                         \
		_CLASS_NAME::StaticClassInfo _CLASS_NAME::g_ClassInfoData =      \
		{                                                                \
			nullptr,                                                     \
			{                                                            \
				{                                                        \
					_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY)             \
				}                                                        \
			}                                                            \
		};                                                               \
		IClassInfo* __fastcall _CLASS_NAME::GetElementClassInfo()        \
		{                                                                \
			return g_ClassInfoData.pClassInfoPtr;                        \
		}                                                                \
		IClassInfo* __fastcall _CLASS_NAME::GetStaticElementClassInfo()  \
		{                                                                \
			return g_ClassInfoData.pClassInfoPtr;                        \
		}                                                                \
		HRESULT __fastcall _CLASS_NAME::Register()                       \
		{                                                                \
			return _CLASS_NAME::StaticClassInfo::ClassInfoType::Register();               \
		}                                                                \
		HRESULT __fastcall _CLASS_NAME::UnRegister()                     \
		{                                                                \
			if (!g_ClassInfoData.pClassInfoPtr)                          \
				return S_FALSE;                                          \
			return g_ClassInfoData.pClassInfoPtr->UnRegister();          \
		}


#if 0
		_APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout, Value::GetElementNull,            nullptr, ValueType::eElement) \
		_APPLY(Children,       PF_Normal | PF_ReadOnly,               PG_AffectsDesiredSize | PG_AffectsLayout, Value::GetElListNull,             nullptr, ValueType::eElementList) \
		_APPLY(Visible,        PF_TriLevel | PF_Cascade | PF_Inherit, 0,                                        Value::GetBoolFalse,              nullptr, ValueType::eBool) \
		_APPLY(Width,          PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                    Value::GetIntConstValue<-1>,      nullptr, ValueType::eInt) \
		_APPLY(Height,         PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                    Value::GetIntConstValue<-1>,      nullptr, ValueType::eInt) \
		_APPLY(X,              PF_Normal,                             0,                                        Value::GetIntZero,                nullptr, ValueType::eInt) \
		_APPLY(Y,              PF_Normal,                             0,                                        Value::GetIntZero,                nullptr, ValueType::eInt) \
		_APPLY(Location,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsBounds,                         Value::GetPointZero,              nullptr, ValueType::ePoint) \
		_APPLY(Extent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsBounds,      Value::GetSizeZero,               nullptr, ValueType::eSize) \
		_APPLY(PosInLayout,    PF_LocalOnly | PF_ReadOnly,            0,                                        Value::GetPointZero,              nullptr, ValueType::ePoint) \
		_APPLY(SizeInLayout,   PF_LocalOnly | PF_ReadOnly,            0,                                        Value::GetSizeZero,               nullptr, ValueType::eSize) \
		_APPLY(DesiredSize,    PF_LocalOnly | PF_ReadOnly,           PG_AffectsLayout | PG_AffectsParentLayout, Value::GetSizeZero,               nullptr, ValueType::eSize) \
		_APPLY(LastDesiredSizeConstraint, PF_LocalOnly | PF_ReadOnly, 0,                                        Value::GetSizeZero,               nullptr, ValueType::eSize) 
#endif

	//     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                  ChangedFun                        pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_ELEMENT_PROPERTY_TABLE(_APPLY) \
	_APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetElementNull,            &Element::_OnParentPropertyChanged, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, _peLocParent), 0, 0, 0, 0, 0), ValueType::Element) \
	_APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::GetInt32ConstValue<LP_Auto>, nullptr,                          nullptr, _MEGA_UI_PROP_BIND_INT(0, 0, 0, UFIELD_OFFSET(Element, _dSpecLayoutPos), 0, 0),  ValueType::int32_t)


		template<typename _Class>
		class ClassInfoBase;


		class Element
		{
			_APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(Element, void, ClassInfoBase<Element>, 0u, _MEGA_UI_ELEMENT_PROPERTY_TABLE);
		private:
			// 所有 Local 值的
			DynamicArray<Value*> _LocalPropValue;

			// Local Parent
			Element* _peLocParent;

			// Cached layout position
			int32_t _dSpecLayoutPos;
		public:
			static HRESULT WINAPI Create(uint32_t fCreate, Element* pTopLevel, intptr_t* pCooike, Element** ppOut);

            HRESULT __fastcall Initialize(uint32_t fCreate, Element* pTopLevel, intptr_t* pCooike);


			Value* __fastcall GetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, bool bUpdateCache);

			//只能修改 Local Value
			HRESULT __fastcall SetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pValue);

			Element* GetParent();

			virtual void __fastcall OnPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);

			DeferCycle* __fastcall GetDeferObject();
			void __fastcall StartDefer(intptr_t* pCooike);
			void __fastcall EndDefer(intptr_t Cookie);
		protected:
			// Value Update
			HRESULT __fastcall _PreSourceChange(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);
			HRESULT __fastcall _PostSourceChange();

			PropertyCustomCacheResult __fastcall _PropertyGeneralCache(PropertyCustomCacheActionMode eMode, PropertyCustomCacheActionInfo* pInfo);

			void __fastcall _OnParentPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);
		};
	}
}

#pragma pack(pop)
