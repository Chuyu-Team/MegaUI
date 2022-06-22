#pragma once
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
			virtual IClassInfo* __fastcall GetControlClassInfo();                                           \
			static IClassInfo* __fastcall GetStaticControlClassInfo();                                      \
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
		IClassInfo* __fastcall _CLASS_NAME::GetControlClassInfo()        \
		{                                                                \
			return g_ClassInfoData.pClassInfoPtr;                        \
		}                                                                \
		IClassInfo* __fastcall _CLASS_NAME::GetStaticControlClassInfo()  \
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
	_APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetElementNull,            &Element::OnParentPropertyChanged, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, pLocParent), 0, 0, 0, 0, 0), ValueType::Element) \
	_APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::GetInt32ConstValue<LP_Auto>, nullptr,                          nullptr, _MEGA_UI_PROP_BIND_INT(0, 0, 0, UFIELD_OFFSET(Element, iSpecLayoutPos), 0, 0), ValueType::int32_t)


		template<typename _Class>
		class ClassInfoBase;


		class Element
		{
			_APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(Element, void, ClassInfoBase<Element>, 0u, _MEGA_UI_ELEMENT_PROPERTY_TABLE);
		private:
			// 所有 Local 值的
			DynamicArray<Value*> LocalPropValue;

			// Local Parent
			Element* pLocParent;

			// Cached layout position
			int32_t iSpecLayoutPos;
		public:
            static HRESULT WINAPI Create(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike, _Outptr_ Element** _ppOut);

            HRESULT __fastcall Initialize(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike);

			/// <summary>
			/// 根据属性获取Value
			/// </summary>
			/// <param name="_Prop"></param>
			/// <param name="_eIndicies"></param>
			/// <param name="_bUpdateCache">如果为true，那么重新获取值并刷新缓存，如果为 false则直接从缓存返回数据。</param>
			/// <returns>如果返回，则返回 Unavailable。
			/// 如果未设置，则返回 Unset</returns>
			_Ret_notnull_ Value* __fastcall GetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ bool _bUpdateCache);
			
            /// <summary>
            /// 修改 Local Value
            /// </summary>
            /// <param name="_Prop">元数据</param>
            /// <param name="_eIndicies">只能为 PI_Local</param>
            /// <param name="_pValue">需要设置的新值</param>
            /// <returns></returns>
            HRESULT __fastcall SetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pValue);

			_Ret_maybenull_ Element* __fastcall GetParent();

			virtual void __fastcall OnPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pOldValue, _In_ Value* _pNewValue);

			DeferCycle* __fastcall GetDeferObject();
            void __fastcall StartDefer(_In_ intptr_t* _pCooike);
            void __fastcall EndDefer(_In_ intptr_t _Cookie);
		protected:
			// Value Update
            HRESULT __fastcall PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pOldValue, _In_ Value* _pNewValue);
			HRESULT __fastcall PostSourceChange();

			PropertyCustomCacheResult __fastcall PropertyGeneralCache(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCacheActionInfo* _pInfo);

			void __fastcall OnParentPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value* _pOldValue, _In_ Value* _pNewValue);
		};
	}
}

#pragma pack(pop)
