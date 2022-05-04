#pragma once
#include <Windows.h>
#include "value.h"
#include "Property.h"

// Global layout positions
#define LP_None         -3
#define LP_Absolute     -2
#define LP_Auto         -1

namespace YY
{
	namespace MegaUI
	{
#define _APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(_CLASS_NAME, _BASE_CLASS, _CALSS_INFO_TYPE, _PROPERTY_TABLE)\
		protected:                                                                                          \
			friend ClassInfoBase<_CLASS_NAME>;                                                              \
            typedef _CALSS_INFO_TYPE ClassInfoType;                                                         \
			struct StaticClassInfo                                                                          \
			{                                                                                               \
				using BaseElement = _BASE_CLASS;                                                            \
				constexpr static raw_const_string pszClassInfoName = # _CLASS_NAME;                         \
				constexpr static unsigned uPropsCount = 0 _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_COUNT);   \
                                                                                                            \
				_CALSS_INFO_TYPE* pClassInfoPtr;                                                            \
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
			static StaticClassInfo ClassInfoData;                                                           \
		public:                                                                                             \
			virtual IClassInfo* __fastcall GetClassInfo();                                                  \
			static IClassInfo* __fastcall GetStaticClassInfo();                                             \
			static HRESULT __fastcall Register();                                                           \
			static HRESULT __fastcall UnRegister();


#define _APPLY_MEGA_UI_STATIC_CALSS_INFO(_CLASS_NAME, _PROPERTY_TABLE)   \
		_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST);        \
                                                                         \
		_CLASS_NAME::StaticClassInfo _CLASS_NAME::ClassInfoData =        \
		{                                                                \
			nullptr,                                                     \
			{                                                            \
				{                                                        \
					_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY)             \
				}                                                        \
			}                                                            \
		};                                                               \
		IClassInfo* __fastcall _CLASS_NAME::GetClassInfo()               \
		{                                                                \
			return ClassInfoData.pClassInfoPtr;                          \
		}                                                                \
		IClassInfo* __fastcall _CLASS_NAME::GetStaticClassInfo()         \
		{                                                                \
			return ClassInfoData.pClassInfoPtr;                          \
		}                                                                \
		HRESULT __fastcall _CLASS_NAME::Register()                       \
		{                                                                \
			return _CLASS_NAME::ClassInfoType::Register();               \
		}                                                                \
		HRESULT __fastcall _CLASS_NAME::UnRegister()                     \
		{                                                                \
			if (!ClassInfoData.pClassInfoPtr)                            \
				return S_FALSE;                                          \
			return ClassInfoData.pClassInfoPtr->UnRegister();            \
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
	_APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::GetInt32ConstValue<LP_Auto>, nullptr,                            nullptr, _MEGA_UI_PROP_BIND_INT(0, 0, 0, UFIELD_OFFSET(Element, _dSpecLayoutPos), 0, 0),  ValueType::int32)


		template<typename _Class>
		class ClassInfoBase;


		class Element
		{
			_APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(Element, void, ClassInfoBase<Element>, _MEGA_UI_ELEMENT_PROPERTY_TABLE);
		private:
			// 所有 Local 值的
			DynamicArray<Value*> _LocalPropValue;

			// Local Parent
			Element* _peLocParent;

			// Cached layout position
			int32_t _dSpecLayoutPos;
		public:
			Value* __fastcall GetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, bool bUpdateCache);

			//只能修改 Local Value
			HRESULT __fastcall SetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pValue);

			Element* GetParent();

			virtual void __fastcall OnPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);

			HRESULT __fastcall Initialize(uint32_t fCreate, Element* pTopLevel, intptr_t* pCooike);

		protected:
			// Value Update
			HRESULT __fastcall _PreSourceChange(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);
			HRESULT __fastcall _PostSourceChange();

			PropertyCustomCacheResult __fastcall _PropertyGeneralCache(PropertyCustomCacheActionMode eMode, PropertyCustomCacheActionInfo* pInfo);

			void __fastcall _OnParentPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew);
		};
	}
}