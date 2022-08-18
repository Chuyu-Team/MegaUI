#pragma once
#include <Windows.h>
#include <d2d1.h>


#include "value.h"
#include "Property.h"
#include "DeferCycle.h"

#include "../Render/Render.h"

// Global layout positions
#define LP_None         -3
#define LP_Absolute     -2
#define LP_Auto         -1


// Layout cycle queue modes
#define LC_Pass 0
#define LC_Unknow1 1
#define LC_Normal 2
#define LC_Optimize 3

// BorderStyleProp
#define BDS_Solid 0
#define BDS_Raised 1
#define BDS_Sunken 2
#define BDS_Rounded 3


#define DIRECTION_LTR 0
#define DIRECTION_RTL 1

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
#define _APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(_CLASS_NAME, _BASE_CLASS, _CLASS_INFO_TYPE,                 \
                                                _DEFAULT_CREATE_FLAGS, _PROPERTY_TABLE)                     \
		public:                                                                                             \
            friend ClassInfoBase<_CLASS_NAME>;                                                              \
            struct StaticClassInfo                                                                          \
            {                                                                                               \
	            using ClassInfoType = _CLASS_INFO_TYPE;                                                     \
				using BaseElement =_BASE_CLASS;                                                             \
                constexpr static uint32_t fDefaultCreate = _DEFAULT_CREATE_FLAGS;                           \
                constexpr static raw_const_astring_t szClassName = #_CLASS_NAME;                           \
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
		_APPLY(Visible,        PF_TriLevel | PF_Cascade | PF_Inherit, 0,                                        Value::GetBoolFalse,              nullptr, ValueType::eBool)
#endif

    // clang-format off
	//     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                  ChangedFun                        pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_ELEMENT_PROPERTY_TABLE(_APPLY) \
    _APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetElementNull,            &Element::OnParentPropertyChanged, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, pLocParent), 0, 0, 0, 0, 0), ValueType::Element) \
    _APPLY(Children,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       nullptr,                           nullptr,                           nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, vecLocChildren), 0, 0, 0, 0, 0), ValueType::ElementList) \
    _APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::GetInt32ConstValue<LP_Auto>, nullptr,                         nullptr, _MEGA_UI_PROP_BIND_INT(0, 0, 0, UFIELD_OFFSET(Element, iSpecLayoutPos), 0, 0), ValueType::int32_t) \
    _APPLY(Width,          PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                          &Value::GetInt32ConstValue<-1>,    nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Height,         PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                          &Value::GetInt32ConstValue<-1>,    nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(X,              PF_Normal,                             0,                                              &Value::GetInt32Zero,              nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Y,              PF_Normal,                             0,                                              &Value::GetInt32Zero,              nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Location,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsBounds,                               &Value::GetPointZero,              nullptr,                           nullptr, _MEGA_UI_PROP_BIND_CUSTOM(&Element::GetLocationProperty),                                                     ValueType::POINT  ) \
    _APPLY(Extent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsBounds,            &Value::GetSizeZero,               nullptr,                           nullptr, _MEGA_UI_PROP_BIND_CUSTOM(&Element::GetExtentProperty),                        ValueType::SIZE   ) \
    _APPLY(PosInLayout,    PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::GetPointZero,              nullptr,                           nullptr, _MEGA_UI_PROP_BIND_POINT(UFIELD_OFFSET(Element, LocPosInLayout), 0, 0, 0, 0, 0), ValueType::POINT  ) \
    _APPLY(SizeInLayout,   PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::GetSizeZero,               nullptr,                           nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocSizeInLayout), 0, 0, 0, 0, 0), ValueType::SIZE   ) \
    _APPLY(DesiredSize,    PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsParentLayout,      &Value::GetSizeZero,               nullptr,                           nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocDesiredSize), 0, 0, 0, 0, 0), ValueType::SIZE   ) \
    _APPLY(LastDesiredSizeConstraint, PF_LocalOnly | PF_ReadOnly, 0,                                              &Value::GetSizeZero,               nullptr,                           nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocLastDesiredSizeConstraint), 0, 0, 0, 0, 0), ValueType::SIZE   ) \
    _APPLY(Layout,         PF_Normal,                             PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetLayoutNull,             nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Layout   ) \
    _APPLY(Background,     PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::GetColorTransparant,       nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Color   ) \
    _APPLY(MinSize,        PF_Normal | PF_Cascade, PG_AffectsLayout | PG_AffectsParentLayout | PG_AffectsBounds | PG_AffectsDisplay,  &Value::GetSizeZero, nullptr,                     nullptr, _MEGA_UI_PROP_BIND_SIZE(0, 0, 0, UFIELD_OFFSET(Element, SpecMinSize), 0, 0), ValueType::SIZE   ) \
    _APPLY(BorderThickness, PF_Normal | PF_Cascade,               PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::GetRectZero,               nullptr,                           nullptr, _MEGA_UI_PROP_BIND_RECT(0, 0, 0, UFIELD_OFFSET(Element, SpecBorderThickness), 0, 0), ValueType::Rect   ) \
    _APPLY(BorderStyle,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::GetInt32Zero,              nullptr,                BorderStyleEnumMap, _MEGA_UI_PROP_BIND_NONE(), ValueType::int32_t   ) \
    _APPLY(BorderColor,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              nullptr,                           nullptr,                           nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Color   ) \
    _APPLY(Direction,      PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsLayout | PG_AffectsDisplay,           nullptr,                           nullptr,                  DirectionEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, 0, UFIELD_OFFSET(Element, iSpecDirection), 0, 0), ValueType::int32_t   ) 
    // clang-format on

		template<typename _Class>
		class ClassInfoBase;


        struct NavReference
        {
            UINT cbSize;
            Element* pElement;
            RECT* pRect;

            NavReference(Element* _pElement, RECT* _pRect = nullptr)
                : cbSize(sizeof(NavReference))
                , pElement(_pElement)
                , pRect(_pRect)
            {
            }
        };

		class Element
		{
			_APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(Element, void, ClassInfoBase<Element>, 0u, _MEGA_UI_ELEMENT_PROPERTY_TABLE);
		private:
            // _pvmLocal
			// 所有 Local 值的
			DynamicArray<Value, false, true> LocalPropValue;
            
            ElementList vecLocChildren;
            
            // 0x8 Index
            int32_t _iIndex = -1;

            // 正常优先级的组 0x10
            int32_t _iGCSlot;
            // 低优先级的组 0x14
            int32_t _iGCLPSlot;
            // v18
            int32_t _iPCTail;
            
            // StartDefer 时保存的内容
            DeferCycle* pDeferObject;
            
            // 顶层 Element，所有的Defer将发送到顶层 Element
            Element* pTopLevel;
			
            // Local Parent
			Element* pLocParent;

            // 0x2C
            // Position in layout local
            POINT LocPosInLayout;
            // 0x34 LayoutSize
            // Size in layout local
            SIZE LocSizeInLayout;
            // 0x3C
            // Desired size local
            SIZE LocDesiredSize;
            // 0x44 DesiredSize
            // Last desired size constraint local
            SIZE LocLastDesiredSizeConstraint;
            
            // 0x4C LayoutPos
            // Cached Layout Position
            int32_t iSpecLayoutPos;
            

            //bits
            uint32_t bSelfLayout : 1;
            uint32_t bNeedsDSUpdate : 1;
            
            // UINT8 LayoutType : 2;
            uint32_t fNeedsLayout : 2;

            // 边框宽度，四个方向，左上右下
            Rect SpecBorderThickness;
            // 内边距
            Rect SpecPadding;

            //Layout* pLayout = nullptr;

            // 最小限制
            SIZE SpecMinSize = {};
            int32_t iSpecDirection = 0;
		public:
            Element();

			Element(const Element&) = delete;

			virtual ~Element();

			Element& operator=(const Element&) = delete;

            static HRESULT __fastcall Create(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike, _Outptr_ Element** _ppOut);

            HRESULT __fastcall Initialize(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike);

			/// <summary>
			/// 根据属性获取Value
			/// </summary>
			/// <param name="_Prop"></param>
			/// <param name="_eIndicies"></param>
			/// <param name="_bUpdateCache">如果为true，那么重新获取值并刷新缓存，如果为 false则直接从缓存返回数据。</param>
			/// <returns>如果返回，则返回 Unavailable。
			/// 如果未设置，则返回 Unset</returns>
			_Ret_notnull_ Value __fastcall GetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ bool _bUpdateCache);
			
            /// <summary>
            /// 修改 Local Value
            /// </summary>
            /// <param name="_Prop">元数据</param>
            /// <param name="_eIndicies">只能为 PI_Local</param>
            /// <param name="_pValue">需要设置的新值</param>
            /// <returns></returns>
            HRESULT __fastcall SetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _Value);

			_Ret_maybenull_ Element* __fastcall GetParent();
            
            int32_t __fastcall GetLayoutPos();
            HRESULT __fastcall SetLayoutPos(int32_t _iLayoutPos);

            int32_t __fastcall GetWidth();
            HRESULT __fastcall SetWidth(int32_t _iWidth);

            int32_t __fastcall GetHeight();
            HRESULT __fastcall SetHeight(int32_t _iHeight);

            int32_t __fastcall GetX();
            HRESULT __fastcall SetX(int32_t _iX);
            
            int32_t __fastcall GetY();
            HRESULT __fastcall SetY(int32_t _iY);

            POINT __fastcall GetLocation();

            SIZE __fastcall GetExtent();

            ValueIs<ValueType::Layout> __fastcall GetLayout();

            int32_t __fastcall GetBorderStyle();

            bool __fastcall IsRTL();


			virtual void __fastcall OnPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

            /// <summary>
            /// 用于通知 PropertyGroup 的状态
            /// </summary>
            /// <param name="_fGroups">PropertyGroup的组合</param>
            void __fastcall OnGroupChanged(uint32_t _fGroups);

            /// <summary>
            /// 获取顶层 Element，便于 StartDefer，如果未设置 pTopLevel，则自身为顶层 Element
            /// </summary>
            /// <returns></returns>
            _Ret_notnull_ Element* __fastcall GetTopLevel();
			_Ret_maybenull_ DeferCycle* __fastcall GetDeferObject(_In_ bool _bAllowCreate = true);
            void __fastcall StartDefer(_Out_ intptr_t* _pCooike);
            void __fastcall EndDefer(_In_ intptr_t _Cookie);
			
            ElementList __fastcall GetChildren();

            virtual HRESULT __fastcall Insert(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren, _In_ uint32_t _uInsert);

            __inline HRESULT __fastcall Add(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren)
            {
                return Insert(_ppChildren, _cChildren, vecLocChildren.GetSize());
            }

            __inline HRESULT __fastcall Add(_In_ Element* _ppChildren)
            {
                return Insert(&_ppChildren, 1, vecLocChildren.GetSize());
            }

            virtual HRESULT __fastcall Remove(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren);
            
            __inline HRESULT __fastcall Remove(_In_ Element* _pChild)
            {
                return Remove(&_pChild, 1);
            }

            __inline HRESULT __fastcall RemoveAll()
            {
                return Remove(vecLocChildren.GetData(), vecLocChildren.GetSize());
            }

            virtual void __fastcall Paint(_In_ Render* _pRenderTarget, _In_ const Rect& _Bounds);

            void __fastcall PaintBorder(_In_ Render* _pRenderTarget, _In_ int32_t _iBorderStyle, _In_ const Rect& _BorderThickness, const Value& _BorderColor, _Inout_ Rect& _Bounds);

            void __fastcall PaintBackground(_In_ Render* _pRenderTarget, const Value& _Background, _In_ const Rect& _Bounds);

            virtual SIZE __fastcall GetContentSize(SIZE _ConstraintSize);
            virtual SIZE __fastcall SelfLayoutUpdateDesiredSize(SIZE _ConstraintSize);
            virtual void __fastcall SelfLayoutDoLayout(SIZE _ConstraintSize);

            void __fastcall Detach(DeferCycle* _pDeferCycle);

            Rect __fastcall ApplyRTL(const Rect& _Src);

		protected:
			// Value Update
            HRESULT __fastcall PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);
			HRESULT __fastcall PostSourceChange();
            HRESULT __fastcall GetDependencies(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _NewValue, DeferCycle* _pDeferCycle);

            static HRESULT __fastcall AddDependency(Element* _pElement, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, DeferCycle* _pDeferCycle);

            static void __fastcall VoidPCNotifyTree(int, DeferCycle*);

			PropertyCustomCacheResult __fastcall PropertyGeneralCache(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);

			void __fastcall OnParentPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _NewValue);
            
            void __fastcall FlushDesiredSize(DeferCycle* _pDeferCycle);

            void __fastcall FlushLayout(DeferCycle* _pDeferCycle);

            static bool __fastcall SetGroupChanges(Element* pElement, uint32_t _fGroups, DeferCycle* pDeferCycle);

            static void __fastcall TransferGroupFlags(Element* pElement, uint32_t _fGroups);
            
            static bool __fastcall MarkElementForDesiredSize(Element* _pElement);

            static bool __fastcall MarkElementForLayout(Element* _pElement, uint32_t _fNeedsLayoutNew);

            bool __fastcall SetNeedsLayout(uint32_t _fNeedsLayoutNew);

            SIZE __fastcall UpdateDesiredSize(SIZE _ConstraintSize);

            void __fastcall UpdateLayoutPosition(POINT _LayoutPosition);
            
            void __fastcall UpdateLayoutSize(SIZE _LayoutSize);

            PropertyCustomCacheResult __fastcall GetExtentProperty(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);

            PropertyCustomCacheResult __fastcall GetLocationProperty(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);
		};
	}
}

#pragma pack(pop)
