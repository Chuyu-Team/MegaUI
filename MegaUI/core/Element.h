﻿#pragma once
#include <Windows.h>
#include <d2d1.h>


#include "value.h"
#include "Property.h"
#include "DeferCycle.h"
#include "ClassInfo.h"
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
            virtual IClassInfo* __MEGA_UI_API GetControlClassInfo();                                           \
            static IClassInfo* __MEGA_UI_API GetStaticControlClassInfo();                                      \
            static HRESULT __MEGA_UI_API Register();                                                           \
            static HRESULT __MEGA_UI_API UnRegister();


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
		IClassInfo* __MEGA_UI_API _CLASS_NAME::GetControlClassInfo()        \
		{                                                                \
			return g_ClassInfoData.pClassInfoPtr;                        \
		}                                                                \
		IClassInfo* __MEGA_UI_API _CLASS_NAME::GetStaticControlClassInfo()  \
		{                                                                \
			return g_ClassInfoData.pClassInfoPtr;                        \
        }                                                                \
        HRESULT __MEGA_UI_API _CLASS_NAME::Register()                       \
        {                                                                \
            return _CLASS_NAME::StaticClassInfo::ClassInfoType::Register();               \
        }                                                                \
        HRESULT __MEGA_UI_API _CLASS_NAME::UnRegister()                     \
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
	//     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                  ChangedFun                                                                       pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_ELEMENT_PROPERTY_TABLE(_APPLY) \
    _APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetElementNull,            &Element::OnParentPropertyChanged, &Element::GetParentDependenciesThunk, nullptr, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, pLocParent), 0, 0, 0), ValueType::Element) \
    _APPLY(Children,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       nullptr,                           nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, vecLocChildren), 0, 0, 0), ValueType::ElementList) \
    _APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::GetInt32ConstValue<LP_Auto>, nullptr,                         nullptr, nullptr, LayoutPosEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, iSpecLayoutPos), 0), ValueType::int32_t) \
    _APPLY(Width,          PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                          &Value::GetInt32ConstValue<-1>,    nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Height,         PF_Normal | PF_Cascade,                PG_AffectsDesiredSize,                          &Value::GetInt32ConstValue<-1>,    nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(X,              PF_Normal,                             0,                                              &Value::GetInt32Zero,              nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Y,              PF_Normal,                             0,                                              &Value::GetInt32Zero,              nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::int32_t) \
    _APPLY(Location,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsBounds,                               &Value::GetPointZero,              nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_CUSTOM(&Element::GetLocationProperty),                      ValueType::POINT  ) \
    _APPLY(Extent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsBounds,            &Value::GetSizeZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_CUSTOM(&Element::GetExtentProperty),                        ValueType::SIZE   ) \
    _APPLY(PosInLayout,    PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::GetPointZero,              nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_POINT(UFIELD_OFFSET(Element, LocPosInLayout), 0, 0, 0),     ValueType::POINT  ) \
    _APPLY(SizeInLayout,   PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::GetSizeZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocSizeInLayout), 0, 0, 0),     ValueType::SIZE   ) \
    _APPLY(DesiredSize,    PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsParentLayout,      &Value::GetSizeZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocDesiredSize), 0, 0, 0),      ValueType::SIZE   ) \
    _APPLY(LastDesiredSizeConstraint, PF_LocalOnly | PF_ReadOnly, 0,                                              &Value::GetSizeZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocLastDesiredSizeConstraint), 0, 0, 0), ValueType::SIZE   ) \
    _APPLY(Layout,         PF_Normal,                             PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::GetLayoutNull,             nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Layout   ) \
    _APPLY(Background,     PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::GetColorTransparant,       nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Color   ) \
    _APPLY(MinSize,        PF_Normal | PF_Cascade, PG_AffectsLayout | PG_AffectsParentLayout | PG_AffectsBounds | PG_AffectsDisplay,  &Value::GetSizeZero, nullptr,                     nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(0, 0, UFIELD_OFFSET(Element, SpecMinSize), 0), ValueType::SIZE   ) \
    _APPLY(BorderThickness, PF_Normal | PF_Cascade,               PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::GetRectZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_RECT(0, 0, UFIELD_OFFSET(Element, SpecBorderThickness), 0), ValueType::Rect   ) \
    _APPLY(BorderStyle,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::GetInt32Zero,              nullptr,                           nullptr, nullptr, BorderStyleEnumMap, _MEGA_UI_PROP_BIND_NONE(), ValueType::int32_t   ) \
    _APPLY(BorderColor,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              nullptr,                           nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::Color   ) \
    _APPLY(Direction,      PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsLayout | PG_AffectsDisplay,           nullptr,                           nullptr,                           nullptr, nullptr, DirectionEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, iSpecDirection), 0), ValueType::int32_t   ) \
    _APPLY(MouseWithin,    PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::GetBoolFalse,              nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(UFIELD_BITMAP_OFFSET(Element, ElementBits, bLocMouseWithin), 0, 0, 0), ValueType::boolean   ) \
    _APPLY(ID,             PF_Normal,                             0,                                              &Value::GetAtomZero,               nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_ATOM(0, 0, UFIELD_OFFSET(Element, SpecID), 0), ValueType::ATOM   ) \
    _APPLY(Sheet,          PF_Normal|PF_Inherit,                  0,                                              &Value::GetSheetNull,              nullptr,                           &Element::GetSheetDependenciesThunk, nullptr, nullptr, _MEGA_UI_PROP_BIND_SHEET(0, 0, UFIELD_OFFSET(Element, pSheet), 0), ValueType::StyleSheet   ) \
    _APPLY(Class,          PF_Normal,                             0,                                              &Value::GetStringNull,             nullptr,                           nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::uString   ) 

    // clang-format on

        class NativeWindowHost;
        class Window;

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
            friend NativeWindowHost;
            friend StyleSheet;
            friend Window;

        protected:
            ElementRenderNode RenderNode;
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
            
            // 0x50
            StyleSheet* pSheet;

            // 0x58 ID
            ATOM SpecID;
            
            //bits
#define _MEGA_UI_ELEMENT_BITS_TABLE(_APPLY) \
    _APPLY(bSelfLayout, 1)                  \
    _APPLY(bNeedsDSUpdate, 1)               \
    /* UINT8 LayoutType : 2;*/              \
    _APPLY(fNeedsLayout, 2)                 \
    _APPLY(bLocMouseWithin, 1)              \
    _APPLY(bDestroy, 1)

            _APPLY_MEGA_UI_BITMAP_TABLE(ElementBits, _MEGA_UI_ELEMENT_BITS_TABLE);
            //union
            //{
            //    struct
            //    {
            //        // 0
            //        uint32_t bSelfLayout : 1;
            //        // 1
            //        uint32_t bNeedsDSUpdate : 1;

            //        // UINT8 LayoutType : 2;
            //        // 2
            //        uint32_t fNeedsLayout : 2;
            //        // 4
            //        uint32_t bLocMouseWithin : 1;
            //    };

            //    uint8_t BitsBuffer[1];
            //};
            

            // 边框宽度，四个方向，左上右下
            Rect SpecBorderThickness;
            // 内边距
            Rect SpecPadding;

            //Layout* pLayout = nullptr;

            // 最小限制
            SIZE SpecMinSize = {};
            int32_t iSpecDirection = 0;

            // 承载控件的窗口
            Window* pWindow = nullptr;
		public:
            Element();

			Element(const Element&) = delete;

			virtual ~Element();

			Element& __MEGA_UI_API operator=(const Element&) = delete;

            static HRESULT __MEGA_UI_API Create(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike, _Outptr_ Element** _ppOut);

            HRESULT __MEGA_UI_API Initialize(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike);

			/// <summary>
			/// 根据属性获取Value
			/// </summary>
			/// <param name="_Prop"></param>
			/// <param name="_eIndicies"></param>
			/// <param name="_bUpdateCache">如果为true，那么重新获取值并刷新缓存，如果为 false则直接从缓存返回数据。</param>
			/// <returns>如果返回，则返回 Unavailable。
			/// 如果未设置，则返回 Unset</returns>
			_Ret_notnull_ Value __MEGA_UI_API GetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ bool _bUpdateCache);
			
            /// <summary>
            /// 修改 Local Value
            /// </summary>
            /// <param name="_Prop">元数据</param>
            /// <param name="_eIndicies">只能为 PI_Local</param>
            /// <param name="_pValue">需要设置的新值</param>
            /// <returns></returns>
            HRESULT __MEGA_UI_API SetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _Value);

			_Ret_maybenull_ Element* __MEGA_UI_API GetParent();
            
            int32_t __MEGA_UI_API GetLayoutPos();
            HRESULT __MEGA_UI_API SetLayoutPos(int32_t _iLayoutPos);

            int32_t __MEGA_UI_API GetWidth();
            HRESULT __MEGA_UI_API SetWidth(int32_t _iWidth);

            int32_t __MEGA_UI_API GetHeight();
            HRESULT __MEGA_UI_API SetHeight(int32_t _iHeight);

            int32_t __MEGA_UI_API GetX();
            HRESULT __MEGA_UI_API SetX(int32_t _iX);
            
            int32_t __MEGA_UI_API GetY();
            HRESULT __MEGA_UI_API SetY(int32_t _iY);

            POINT __MEGA_UI_API GetLocation();

            SIZE __MEGA_UI_API GetExtent();

            ValueIs<ValueType::Layout> __MEGA_UI_API GetLayout();

            int32_t __MEGA_UI_API GetBorderStyle();

            HRESULT __MEGA_UI_API SetBorderStyle(int32_t _iBorderStyle);

            HRESULT __MEGA_UI_API SetBorderColor(Color _BorderColor);

            bool __MEGA_UI_API IsRTL();

            bool __MEGA_UI_API IsMouseWithin();
            
            /// <summary>
            /// 返回控件绑定的 Class，它用于 Sheet 表达式匹配。
            /// </summary>
            /// <returns>Class</returns>
            uString __MEGA_UI_API GetClass();

            HRESULT __MEGA_UI_API SetClass(uString _szClass);

            /// <summary>
            /// 当属性正在更改时调用，可以终止属性更改。
            /// </summary>
            /// <returns>如果返回 true，那么允许更改。如果返回false，更改将被撤销。</returns>
            virtual bool __MEGA_UI_API OnPropertyChanging(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

			virtual void __MEGA_UI_API OnPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

            /// <summary>
            /// 用于通知 PropertyGroup 的状态
            /// </summary>
            /// <param name="_fGroups">PropertyGroup的组合</param>
            void __MEGA_UI_API OnGroupChanged(uint32_t _fGroups);

            /// <summary>
            /// 获取顶层 Element，便于 StartDefer，如果未设置 pTopLevel，则自身为顶层 Element
            /// </summary>
            /// <returns></returns>
            _Ret_notnull_ Element* __MEGA_UI_API GetTopLevel();
			_Ret_maybenull_ DeferCycle* __MEGA_UI_API GetDeferObject(_In_ bool _bAllowCreate = true);
            void __MEGA_UI_API StartDefer(_Out_ intptr_t* _pCooike);
            void __MEGA_UI_API EndDefer(_In_ intptr_t _Cookie);
			
            ElementList __MEGA_UI_API GetChildren();

            virtual HRESULT __MEGA_UI_API Insert(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren, _In_ uint32_t _uInsert);

            __inline HRESULT __MEGA_UI_API Add(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren)
            {
                return Insert(_ppChildren, _cChildren, vecLocChildren.GetSize());
            }

            __inline HRESULT __MEGA_UI_API Add(_In_ Element* _ppChildren)
            {
                return Insert(&_ppChildren, 1, vecLocChildren.GetSize());
            }

            virtual HRESULT __MEGA_UI_API Remove(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint32_t _cChildren);
            
            __inline HRESULT __MEGA_UI_API Remove(_In_ Element* _pChild)
            {
                return Remove(&_pChild, 1);
            }

            __inline HRESULT __MEGA_UI_API RemoveAll()
            {
                return Remove(vecLocChildren.GetData(), vecLocChildren.GetSize());
            }

            /// <summary>
            /// 当销毁控制时被调用。
            /// </summary>
            /// <returns></returns>
            virtual void __MEGA_UI_API OnDestroy();

            /// <summary>
            /// 释放控件相关资源，包含子控件。
            /// </summary>
            /// <param name="_fDelayed">是否延迟删除。</param>
            /// <returns></returns>
            HRESULT __MEGA_UI_API Destroy(_In_ bool _fDelayed = true);

            /// <summary>
            /// 释放子控件的相关资源（不会释放控件自己）。
            /// </summary>
            /// <param name="_fDelayed">是否延迟删除。</param>
            /// <returns></returns>
            HRESULT __MEGA_UI_API DestroyAllChildren(_In_ bool _fDelayed = true);

            virtual void __MEGA_UI_API Paint(_In_ Render* _pRenderTarget, _In_ const Rect& _Bounds);

            void __MEGA_UI_API PaintBorder(_In_ Render* _pRenderTarget, _In_ int32_t _iBorderStyle, _In_ const Rect& _BorderThickness, const Value& _BorderColor, _Inout_ Rect& _Bounds);

            void __MEGA_UI_API PaintBackground(_In_ Render* _pRenderTarget, const Value& _Background, _In_ const Rect& _Bounds);

            virtual SIZE __MEGA_UI_API GetContentSize(SIZE _ConstraintSize);
            virtual SIZE __MEGA_UI_API SelfLayoutUpdateDesiredSize(SIZE _ConstraintSize);
            virtual void __MEGA_UI_API SelfLayoutDoLayout(SIZE _ConstraintSize);

            void __MEGA_UI_API Detach(DeferCycle* _pDeferCycle);

            Rect __MEGA_UI_API ApplyRTL(const Rect& _Src);

            void __MEGA_UI_API Invalidate();


            template<typename _Type>
            _Type* __MEGA_UI_API TryCast()
            {
                auto _pClassInfo = GetControlClassInfo();
                if (!_pClassInfo)
                    return nullptr;

                if (!_pClassInfo->IsSubclassOf(_Type::GetStaticControlClassInfo()))
                    return nullptr;

                return (_Type*)this;
            }
            
            /// <summary>
            /// 只比较二个控件的SpecCache值是否相等。注意这个函数只是为了性能需要，并且经常失败……
            /// </summary>
            /// <param name="_pElement1"></param>
            /// <param name="_pElement2"></param>
            /// <param name="_Prop"></param>
            /// <returns>返回1表示相等，返回0表示不相等，返回 -1 表示比较失败。</returns>
            static int32_t __MEGA_UI_API SpecCacheIsEqual(
                _In_ Element* _pElement1,
                _In_ Element* _pElement2,
                _In_ const PropertyInfo& _Prop
                );

		protected:
			// Value Update
            HRESULT __MEGA_UI_API PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);
			HRESULT __MEGA_UI_API PostSourceChange();
            HRESULT __MEGA_UI_API GetDependencies(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _NewValue, DeferCycle* _pDeferCycle);

            static HRESULT __MEGA_UI_API AddDependency(Element* _pElement, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, DeferCycle* _pDeferCycle);
            
            HRESULT __MEGA_UI_API GetBuriedSheetDependencies(const PropertyInfo* _pProp, Element* _pElement, DepRecs* _pDR, DeferCycle* _pDeferCycle);

            static void __MEGA_UI_API VoidPCNotifyTree(int, DeferCycle*);

			PropertyCustomCacheResult __MEGA_UI_API PropertyGeneralCache(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);

			void __MEGA_UI_API OnParentPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _NewValue);
            
            void __MEGA_UI_API FlushDesiredSize(DeferCycle* _pDeferCycle);

            void __MEGA_UI_API FlushLayout(DeferCycle* _pDeferCycle);

            static bool __MEGA_UI_API SetGroupChanges(Element* pElement, uint32_t _fGroups, DeferCycle* pDeferCycle);

            static void __MEGA_UI_API TransferGroupFlags(Element* pElement, uint32_t _fGroups);
            
            static bool __MEGA_UI_API MarkElementForDesiredSize(Element* _pElement);

            static bool __MEGA_UI_API MarkElementForLayout(Element* _pElement, uint32_t _fNeedsLayoutNew);

            bool __MEGA_UI_API SetNeedsLayout(uint32_t _fNeedsLayoutNew);

            SIZE __MEGA_UI_API UpdateDesiredSize(SIZE _ConstraintSize);

            void __MEGA_UI_API UpdateLayoutPosition(POINT _LayoutPosition);
            
            void __MEGA_UI_API UpdateLayoutSize(SIZE _LayoutSize);

            PropertyCustomCacheResult __MEGA_UI_API GetExtentProperty(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);

            PropertyCustomCacheResult __MEGA_UI_API GetLocationProperty(_In_ PropertyCustomCacheActionMode _eMode, _Inout_ PropertyCustomCachenBaseAction* _pInfo);

            HRESULT __MEGA_UI_API GetParentDependenciesThunk(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle);

            virtual HRESULT __MEGA_UI_API GetParentDependencies(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle);
            
            HRESULT __MEGA_UI_API GetSheetDependenciesThunk(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle);

            virtual HRESULT __MEGA_UI_API GeSheetDependencies(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle);

            virtual HRESULT __MEGA_UI_API OnHosted(Window* _pNewWindow);

            virtual HRESULT __MEGA_UI_API OnUnHosted(Window* _pOldWindow);
		};
	}
}

#pragma pack(pop)
