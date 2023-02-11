#pragma once
#include <Windows.h>
#include <d2d1.h>


#include "value.h"
#include "Property.h"
#include "DeferCycle.h"
#include "ControlInfo.h"
#include <MegaUI/Render/Render.h>
#include <MegaUI/core/UIEvent.h>

// Global layout positions
#define LP_None         -3
#define LP_Absolute     -2
#define LP_Auto         -1


// Layout cycle queue modes
#define LC_Pass 0
#define LC_Unknow1 1
#define LC_Normal 2
#define LC_Optimize 3

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // ActiveProp
        namespace Active
        {
            constexpr auto Inactive = 0x00000000;
            constexpr auto Mouse = 0x00000001;
            constexpr auto Keyboard = 0x00000002;
            constexpr auto ActiveMarks = Mouse | Keyboard;
        }

        // BorderStyleProp
        enum class BorderStyle
        {
            // 实心边框
            Solid,
            // 凸起样式边框
            Raised,
            // 凹起样式边框
            Sunken,
            // 圆角样式边框
            Rounded,
        };

        // FlowDirectionProp
        enum class FlowDirection
        {
            // 内容从右到左开始流动。
            LeftToRight,
            // 内容从左到右开始流动。
            RightToLeft,
        };

    // clang-format off
	//     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_ELEMENT_PROPERTY_TABLE(_APPLY) \
    _APPLY(Parent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::CreateElementNull,            &Element::ParentPropHandle,          nullptr, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, pLocParent), 0, 0, 0),       ValueType::Element) \
    _APPLY(Children,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsDesiredSize | PG_AffectsLayout,       nullptr,                              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_ELEMENT(UFIELD_OFFSET(Element, vecLocChildren), 0, 0, 0),   ValueType::ElementList) \
    _APPLY(Visible,        PF_TriLevel | PF_Cascade | PF_Inherit, 0,                                              &Value::CreateBoolFalse,              &Element::VisiblePropHandle,         nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(0, 0, UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecVisible), 0),                                                     ValueType::boolean   ) \
    _APPLY(Enabled,        PF_Normal | PF_Cascade | PF_Inherit,   0,                                              &Value::CreateBoolTrue,               &Element::EnabledPropHandle,         nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(0, 0, UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecEnabled), 0), ValueType::boolean   ) \
    _APPLY(Active,         PF_Normal,                             0,                                              &Value::CreateInt32Zero,              &Element::ActivePropHandle,          nullptr, ActiveEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, uSpecActive), 0),    ValueType::int32_t   ) \
    _APPLY(LayoutPos,      PF_Normal | PF_Cascade,                PG_AffectsDesiredSize | PG_AffectsParentLayout, &Value::CreateInt32<LP_Auto>,         nullptr,                             nullptr, LayoutPosEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, iSpecLayoutPos), 0), ValueType::int32_t) \
    _APPLY(Width,          PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsDesiredSize,                          &Value::CreateFloat<-1>,              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::float_t) \
    _APPLY(Height,         PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsDesiredSize,                          &Value::CreateFloat<-1>,              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::float_t) \
    _APPLY(X,              PF_Normal | PF_UpdateDpi,              0,                                              &Value::CreateFloat<0>,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::float_t) \
    _APPLY(Y,              PF_Normal | PF_UpdateDpi,              0,                                              &Value::CreateFloat<0>,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::float_t) \
    _APPLY(Location,       PF_LocalOnly | PF_ReadOnly,            PG_AffectsBounds,                               &Value::CreatePointZero,              &Element::LocationPropHandle,        nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Point  ) \
    _APPLY(Extent,         PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsBounds,            &Value::CreateSizeZero,               &Element::ExtentPropHandle,          nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Size   ) \
    _APPLY(PosInLayout,    PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::CreatePointZero,              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_POINT(UFIELD_OFFSET(Element, LocPosInLayout), 0, 0, 0),     ValueType::Point  ) \
    _APPLY(SizeInLayout,   PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::CreateSizeZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocSizeInLayout), 0, 0, 0),     ValueType::Size   ) \
    _APPLY(DesiredSize,    PF_LocalOnly | PF_ReadOnly,            PG_AffectsLayout | PG_AffectsParentLayout,      &Value::CreateSizeZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocDesiredSize), 0, 0, 0),      ValueType::Size   ) \
    _APPLY(LastDesiredSizeConstraint, PF_LocalOnly | PF_ReadOnly, 0,                                              &Value::CreateSizeZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(UFIELD_OFFSET(Element, LocLastDesiredSizeConstraint), 0, 0, 0), ValueType::Size   ) \
    _APPLY(Layout,         PF_Normal,                             PG_AffectsDesiredSize | PG_AffectsLayout,       &Value::CreateLayoutNull,             nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Layout   ) \
    _APPLY(Background,     PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::CreateColorTransparant,       nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Color   ) \
    _APPLY(Foreground,     PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsDisplay,                              &Value::CreateColorTransparant,       nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Color   ) \
    _APPLY(MinSize,        PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsLayout | PG_AffectsParentLayout | PG_AffectsBounds | PG_AffectsDisplay,  &Value::CreateSizeZero, nullptr,        nullptr, nullptr, _MEGA_UI_PROP_BIND_SIZE(0, 0, UFIELD_OFFSET(Element, SpecMinSize), 0),         ValueType::Size   ) \
    _APPLY(BorderThickness, PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsDesiredSize|PG_AffectsDisplay,       &Value::CreateRectZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_RECT(0, 0, UFIELD_OFFSET(Element, SpecBorderThickness), 0), ValueType::Rect   ) \
    _APPLY(BorderStyle,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::CreateInt32Zero,              nullptr,                             nullptr, BorderStyleEnumMap, _MEGA_UI_PROP_BIND_NONE(),                                          ValueType::int32_t   ) \
    _APPLY(BorderColor,    PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::CreateColorTransparant,       nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Color   ) \
    _APPLY(Padding,        PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsDisplay | PG_AffectsDesiredSize,      &Value::CreateRectZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_RECT(0, 0, UFIELD_OFFSET(Element, SpecPadding), 0),         ValueType::Rect   ) \
    _APPLY(FocusVisible,   PF_Normal | PF_Cascade,                PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::CreateBoolFalse,              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(0, 0, UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecFocusVisible), 0), ValueType::boolean   ) \
    _APPLY(FocusThickness, PF_Normal | PF_Cascade | PF_UpdateDpi, PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::CreateRectZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_RECT(0, 0, UFIELD_OFFSET(Element, SpecFocusThickness), 0),  ValueType::Rect   ) \
    _APPLY(FocusStyle,     PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::CreateInt32Zero,              nullptr,                             nullptr, BorderStyleEnumMap, _MEGA_UI_PROP_BIND_NONE(),                                          ValueType::int32_t   ) \
    _APPLY(FocusColor,     PF_Normal | PF_Cascade,                PG_AffectsDisplay,                              &Value::CreateColorTransparant,       nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::Color   ) \
    _APPLY(FlowDirection,  PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsLayout | PG_AffectsDisplay,           nullptr,                              nullptr,                             nullptr, FlowDirectionEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, iSpecFlowDirection), 0), ValueType::int32_t   ) \
    _APPLY(MouseFocused,   PF_Normal | PF_ReadOnly | PF_Inherit,  0,                                              &Value::CreateBoolFalse,              &Element::MouseFocusedPropHandle,    nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(UFIELD_BITMAP_OFFSET(Element, ElementBits, bLocMouseFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bHasLocMouseFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecMouseFocused), 0), ValueType::boolean ) \
    _APPLY(MouseFocusWithin, PF_LocalOnly | PF_ReadOnly,          0,                                              &Value::CreateBoolFalse,              &Element::MouseFocusWithinPropHandle,nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::boolean   ) \
    _APPLY(KeyboardFocused,PF_Normal | PF_ReadOnly | PF_Inherit,  0,                                              &Value::CreateBoolFalse,              &Element::KeyboardFocusedPropHandle, nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(UFIELD_BITMAP_OFFSET(Element, ElementBits, bLocKeyboardFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bHasLocKeyboardFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecKeyboardFocused), 0), ValueType::boolean   ) \
    _APPLY(KeyboardFocusWithin, PF_LocalOnly | PF_ReadOnly,       0,                                              &Value::CreateBoolFalse,              &Element::KeyboardFocusWithinPropHandle, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                 ValueType::boolean   ) \
    _APPLY(Focused,        PF_Normal | PF_ReadOnly | PF_Inherit,  0,                                              &Value::CreateBoolFalse,              &Element::FocusedPropHandle,         nullptr, nullptr, _MEGA_UI_PROP_BIND_BOOL(UFIELD_BITMAP_OFFSET(Element, ElementBits, bLocFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bHasLocFocused), UFIELD_BITMAP_OFFSET(Element, ElementBits, bSpecFocused), 0), ValueType::boolean   ) \
    _APPLY(FocusWithin,    PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::CreateBoolFalse,              &Element::FocusWithinPropHandle,     nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::boolean   ) \
    _APPLY(Id,             PF_Normal,                             0,                                              &Value::CreateAtomZero,               nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_ATOM(0, 0, UFIELD_OFFSET(Element, SpecID), 0),              ValueType::ATOM   ) \
    _APPLY(Sheet,          PF_Normal|PF_Inherit,                  0,                                              &Value::CreateSheetNull,              &Element::SheetPropHandle,           nullptr, nullptr, _MEGA_UI_PROP_BIND_SHEET(0, 0, UFIELD_OFFSET(Element, pSheet), 0),             ValueType::StyleSheet   ) \
    _APPLY(Class,          PF_Normal,                             0,                                              &Value::CreateEmptyString,            nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::uString   ) \
    _APPLY(Content,        PF_Normal | PF_Cascade,                PG_AffectsDesiredSize|PG_AffectsDisplay,        nullptr,                              nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(),                                                     ValueType::uString   ) \
    _APPLY(ContentAlign,   PF_Normal | PF_Cascade,                PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::CreateInt32Zero,              nullptr,                             nullptr, ContentAlignEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, fSpecContentAlign), 0), ValueType::int32_t   ) \
    _APPLY(FontFamily,     PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::CreateDefaultFontFamily,      nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_STRING(0, 0, UFIELD_OFFSET(Element, SpecFont.szFace), 0),   ValueType::uString   ) \
    _APPLY(FontSize,       PF_Normal | PF_Cascade | PF_Inherit | PF_UpdateDpi, PG_AffectsDesiredSize|PG_AffectsDisplay, &Value::CreateFloat<20>,        nullptr,                             nullptr, nullptr, _MEGA_UI_PROP_BIND_FLOAT(0, 0, UFIELD_OFFSET(Element, SpecFont.iSize), 0),     ValueType::float_t   ) \
    _APPLY(FontWeight,     PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsDesiredSize|PG_AffectsDisplay,        &Value::CreateInt32<FontWeight::Normal>, nullptr,                          nullptr, FontWeightEnumMaps, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, SpecFont.uWeight), 0), ValueType::int32_t   ) \
    _APPLY(FontStyle,      PF_Normal | PF_Cascade | PF_Inherit,   PG_AffectsDisplay,                              &Value::CreateInt32<FontStyle::None>, nullptr,                             nullptr, FontStyleEnumMap, _MEGA_UI_PROP_BIND_INT(0, 0, UFIELD_OFFSET(Element, SpecFont.fStyle), 0), ValueType::int32_t   ) \
    _APPLY(Dpi,            PF_LocalOnly | PF_ReadOnly,            0,                                              &Value::CreateInt32<96>,              &Element::DpiPropHandle,             nullptr, nullptr, _MEGA_UI_PROP_BIND_INT(UFIELD_OFFSET(Element, iLocDpi), 0, 0, 0),              ValueType::int32_t   ) 

    // clang-format on

        class NativeWindowHost;
        class Window;
        class UIParser;

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
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(Element, void, ControlInfoImp<Element>, 0u, _MEGA_UI_ELEMENT_PROPERTY_TABLE);
            friend NativeWindowHost;
            friend StyleSheet;
            friend Window;

        protected:
            ElementRenderNode RenderNode;
            // _pvmLocal
			// 所有 Local 值的
			Array<Value> LocalPropValue;
            
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
            Point LocPosInLayout;
            // 0x34 LayoutSize
            // Size in layout local
            Size LocSizeInLayout;
            // 0x3C _sizeLocLastDSConst
            // Last desired size constraint local
            Size LocLastDesiredSizeConstraint;
            // 0x44 DesiredSize
            // Desired size local
            Size LocDesiredSize;
            
            // 0x4C LayoutPos
            // Cached Layout Position
            int32_t iSpecLayoutPos;
            
            // 0x50
            StyleSheet* pSheet;

            // 0x58 ID
            ATOM SpecID;
            
            uint32_t uSpecActive = Active::Inactive;
            //bits
#define _MEGA_UI_ELEMENT_BITS_TABLE(_APPLY) \
    _APPLY(bSelfLayout, 1)                  \
    _APPLY(bNeedsDSUpdate, 1)               \
    /* UINT8 LayoutType : 2;*/              \
    _APPLY(fNeedsLayout, 2)                 \
    _APPLY(uLocMouseFocusWithin, 2)         \
    _APPLY(bDestroy, 1)                     \
    _APPLY(bSpecVisible, 1)                 \
    _APPLY(bCmpVisible, 1)                  \
    _APPLY(bSpecEnabled, 1)                 \
    _APPLY(bHasLocMouseFocused, 1)          \
    _APPLY(bLocMouseFocused, 1)             \
    _APPLY(bSpecMouseFocused, 1)            \
    _APPLY(bSpecFocusVisible, 1)            \
    _APPLY(uLocKeyboardFocusWithin, 2)      \
    _APPLY(bHasLocKeyboardFocused, 1)       \
    _APPLY(bLocKeyboardFocused, 1)          \
    _APPLY(bSpecKeyboardFocused, 1)         \
    _APPLY(uLocFocusWithin, 2)              \
    _APPLY(bHasLocFocused, 1)               \
    _APPLY(bLocFocused, 1)                  \
    _APPLY(bSpecFocused, 1)


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
            // 边框宽度，四个方向，左上右下
            Rect SpecFocusThickness;

            //Layout* pLayout = nullptr;

            // 最小限制
            Size SpecMinSize = {};
            FlowDirection iSpecFlowDirection = FlowDirection::LeftToRight;
            int32_t fSpecContentAlign = ContentAlign::Top | ContentAlign::Left;
            // 当前DPI值
            int32_t iLocDpi = 96;
            // 承载控件的窗口
            Window* pWindow = nullptr;

            // 缓存的字体信息
            Font SpecFont;
		public:
            Element();

			virtual ~Element();
            
            // 此函数来自 _APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN
            // static HRESULT __YYAPI Create(_In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike, _Outptr_ Element** _ppOut);

            HRESULT __YYAPI Initialize(_In_ int32_t _iDPI, _In_ uint32_t _fCreate, _In_opt_ Element* _pTopLevel, _Out_opt_ intptr_t* _pCooike);

			/// <summary>
			/// 根据属性获取Value
			/// </summary>
			/// <param name="_Prop"></param>
			/// <param name="_eIndicies"></param>
			/// <param name="_bUpdateCache">如果为true，那么重新获取值并刷新缓存，如果为 false则直接从缓存返回数据。</param>
			/// <returns>如果返回，则返回 Unavailable。
			/// 如果未设置，则返回 Unset</returns>
            Value __YYAPI GetValue(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies = PropertyIndicies::PI_Specified, _In_ bool _bUpdateCache = false);
			
            /// <summary>
            /// 修改 Local Value
            /// </summary>
            /// <param name="_Prop">元数据</param>
            /// <param name="_pValue">需要设置的新值</param>
            /// <returns></returns>
            HRESULT __YYAPI SetValue(_In_ const PropertyInfo& _Prop, _In_ const Value& _Value);

            HRESULT __YYAPI SetValueInternal(_In_ const PropertyInfo& _Prop, _In_ const Value& _Value, _In_ bool _bCanCancel = false);

			_Ret_maybenull_ Element* __YYAPI GetParent();
            
            int32_t __YYAPI GetLayoutPos();
            HRESULT __YYAPI SetLayoutPos(int32_t _iLayoutPos);

            float __YYAPI GetWidth();
            HRESULT __YYAPI SetWidth(float _iWidth);

            float __YYAPI GetHeight();
            HRESULT __YYAPI SetHeight(float _iHeight);

            float __YYAPI GetX();
            HRESULT __YYAPI SetX(float _iX);
            
            float __YYAPI GetY();
            HRESULT __YYAPI SetY(float _iY);

            Point __YYAPI GetLocation();

            Size __YYAPI GetExtent();

            ValueIs<ValueType::Layout> __YYAPI GetLayout();

            BorderStyle __YYAPI GetBorderStyle();

            HRESULT __YYAPI SetBorderStyle(BorderStyle _eBorderStyle);
            
            Color __YYAPI GetBorderColor();

            HRESULT __YYAPI SetBorderColor(Color _BorderColor);
            
            BorderStyle __YYAPI GetFocusBorderStyle();

            HRESULT __YYAPI SetFocusBorderStyle(BorderStyle _eBorderStyle);
            
            Color __YYAPI GetFocusBorderColor();

            HRESULT __YYAPI SetFocusBorderColor(Color _BorderColor);

            /// <summary>
            /// 获取内容流动方式，从左到右还是从右到左。
            /// </summary>
            /// <returns></returns>
            FlowDirection __YYAPI GetFlowDirection();
            
            HRESULT __YYAPI SetFlowDirection(FlowDirection _eFlowDirection);
            
            Rect __YYAPI ApplyFlowDirection(const Rect& _Src);

            bool __YYAPI IsMouseFocusWithin();
            
            /// <summary>
            /// 返回控件绑定的 Class，它用于 Sheet 表达式匹配。
            /// </summary>
            /// <returns>Class</returns>
            uString __YYAPI GetClass();

            HRESULT __YYAPI SetClass(uString _szClass);

            /// <summary>
            /// 控件是否被禁用。
            /// </summary>
            /// <returns></returns>
            bool __YYAPI IsEnabled();
            
            HRESULT __YYAPI SetEnabled(bool _bEnabled);

            /// <summary>
            /// 返回控件是否需要主动处理鼠标或者键盘的焦点状态。
            /// </summary>
            /// <returns>ActiveMarks的位组合</returns>
            uint32_t __YYAPI GetActive();

            /// <summary>
            /// 设置控件需要主动处理是焦点状态。比如设置鼠标后可以主动处理鼠标焦点。
            /// </summary>
            /// <param name="_fActive">ActiveMarks的位组合</param>
            /// <returns>HRESULT</returns>
            HRESULT __YYAPI SetActive(uint32_t _fActive);

            bool __YYAPI IsMouseFocused();

            int32_t __YYAPI GetDpi();

            uString __YYAPI GetFontFamily();

            HRESULT __YYAPI SetFontFamily(uString _szFontFamily);

            float __YYAPI GetFontSize();

            HRESULT __YYAPI SetFontSize(float _iFontSize);

            uint32_t __YYAPI GetFontWeight();

            HRESULT __YYAPI SetFontWeight(uint32_t _iFontWeight);

            uint32_t __YYAPI GetFontStyle();

            HRESULT __YYAPI SetFontStyle(uint32_t _fFontStyle);

            /// <summary>
            /// 当属性正在更改时调用，可以终止属性更改。
            /// </summary>
            /// <returns>如果返回 true，那么允许更改。如果返回false，更改将被撤销。</returns>
            virtual bool __YYAPI OnPropertyChanging(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

			virtual void __YYAPI OnPropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);

            /// <summary>
            /// 用于通知 PropertyGroup 的状态
            /// </summary>
            /// <param name="_fGroups">PropertyGroup的组合</param>
            void __YYAPI OnGroupChanged(uint32_t _fGroups);

            /// <summary>
            /// 获取顶层 Element，便于 StartDefer，如果未设置 pTopLevel，则自身为顶层 Element
            /// </summary>
            /// <returns></returns>
            _Ret_notnull_ Element* __YYAPI GetTopLevel();
			_Ret_maybenull_ DeferCycle* __YYAPI GetDeferObject(_In_ bool _bAllowCreate = true);
            void __YYAPI StartDefer(_Out_ intptr_t* _pCooike);
            void __YYAPI EndDefer(_In_ intptr_t _Cookie);
            
            HRESULT __YYAPI SetVisible(bool bVisible);

            bool __YYAPI IsVisible();

            ElementList __YYAPI GetChildren();

            virtual HRESULT __YYAPI Insert(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint_t _cChildren, _In_ uint_t _uInsert);

            __inline HRESULT __YYAPI Add(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint_t _cChildren)
            {
                return Insert(_ppChildren, _cChildren, vecLocChildren.GetSize());
            }

            __inline HRESULT __YYAPI Add(_In_ Element* _ppChildren)
            {
                return Insert(&_ppChildren, 1, vecLocChildren.GetSize());
            }

            virtual HRESULT __YYAPI Remove(_In_reads_(_cChildren) Element* const* _ppChildren, _In_ uint_t _cChildren);
            
            __inline HRESULT __YYAPI Remove(_In_ Element* _pChild)
            {
                return Remove(&_pChild, 1);
            }

            __inline HRESULT __YYAPI RemoveAll()
            {
                return Remove(vecLocChildren.GetData(), vecLocChildren.GetSize());
            }

            /// <summary>
            /// 当销毁控制时被调用。
            /// </summary>
            /// <returns></returns>
            virtual void __YYAPI OnDestroy();

            /// <summary>
            /// 释放控件相关资源，包含子控件。
            /// </summary>
            /// <param name="_fDelayed">是否延迟删除。</param>
            /// <returns></returns>
            HRESULT __YYAPI Destroy(_In_ bool _fDelayed = true);

            /// <summary>
            /// 释放子控件的相关资源（不会释放控件自己）。
            /// </summary>
            /// <param name="_fDelayed">是否延迟删除。</param>
            /// <returns></returns>
            HRESULT __YYAPI DestroyAllChildren(_In_ bool _fDelayed = true);

            virtual void __YYAPI Paint(_In_ Render* _pRenderTarget, _In_ const Rect& _Bounds);

            void __YYAPI PaintBorder(_In_ Render* _pRenderTarget, _In_ BorderStyle _eBorderStyle, _In_ const Rect& _BorderThickness, Color _BorderColor, _Inout_ Rect& _Bounds);

            void __YYAPI PaintBackground(_In_ Render* _pRenderTarget, const Value& _Background, _In_ const Rect& _Bounds);
            
            void __YYAPI PaintContent(
                _In_ Render* _pRenderTarget,
                _In_ const Value& _Content,
                _In_ const Font& _FontInfo,
                _In_ Color _ForegroundColor,
                _In_ const Rect& _Bounds,
                _In_ int32_t _fContentAlign
                );

            virtual Size __YYAPI GetContentSize(Size _ConstraintSize);
            virtual Size __YYAPI SelfLayoutUpdateDesiredSize(Size _ConstraintSize);
            virtual void __YYAPI SelfLayoutDoLayout(Size _ConstraintSize);

            void __YYAPI Detach(DeferCycle* _pDeferCycle);

            void __YYAPI Invalidate();


            template<typename _Type>
            _Type* __YYAPI TryCast()
            {
                auto _pControlInfo = GetControlInfo();
                if (!_pControlInfo)
                    return nullptr;

                if (!_pControlInfo->IsSubclassOf(_Type::GetStaticControlInfo()))
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
            static int32_t __YYAPI SpecCacheIsEqual(
                _In_ Element* _pElement1,
                _In_ Element* _pElement2,
                _In_ const PropertyInfo& _Prop
                );

            /// <summary>
            /// 从 _pFrom，获取父节点 = this 的子节点
            /// </summary>
            /// <param name="_pFrom"></param>
            /// <returns>如果没有这样的节点则返回 nullptr。</returns>
            _Ret_maybenull_ Element* __YYAPI GetImmediateChild(_In_opt_ Element* _pFrom);

            /// <summary>
            /// 设置键盘焦点。
            /// 注意：控件需要Visible、未禁用，且拥有 Active::Keyboard，否则将失败。
            /// 注意：因为键盘焦点属于物理焦点，设置键盘焦点时同时也将改变逻辑焦点。
            /// </summary>
            /// <returns>如果设置成功，则返回 true。</returns>
            bool __YYAPI SetKeyboardFocus();

            /// <summary>
            /// 设置逻辑焦点。
            /// 注意：控件需要Visible、未禁用，且拥有 Active::Keyboard 或者 Active::Mouse，否则将失败。
            /// 注意：如果逻辑焦点与键盘焦点不同，那么将清除逻辑焦点。
            /// </summary>
            /// <returns>如果设置成功，则返回 true。</returns>
            bool __YYAPI SetFocus();

		protected:
			// Value Update
            HRESULT __YYAPI PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue);
			HRESULT __YYAPI PostSourceChange();
            HRESULT __YYAPI GetDependencies(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _NewValue, DeferCycle* _pDeferCycle);

            static HRESULT __YYAPI AddDependency(Element* _pElement, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, DeferCycle* _pDeferCycle);
            
            HRESULT __YYAPI GetBuriedSheetDependencies(const PropertyInfo* _pProp, Element* _pElement, DepRecs* _pDR, DeferCycle* _pDeferCycle);

            static void __YYAPI VoidPCNotifyTree(int, DeferCycle*);

			/// <summary>
			/// 通用处理程序，可以处理属性的默认缓存行为。
			/// </summary>
			/// <param name="_eType"></param>
			/// <param name="_pHandleData"></param>
			/// <returns>如果被处理，则返回 true。</returns>
            bool __YYAPI GeneralHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            template<typename _HandleType>
            bool __YYAPI GeneralHandle(_Inout_ _HandleType* _pHandleData)
            {
                return GeneralHandle(_HandleType::HandleType, _pHandleData);
            }
            
            Value __YYAPI GetGeneralCacheValue(
                ValueType _eType,
                uint16_t _uOffsetToCache,
                uint8_t _uCacheBit,
                uint16_t _uOffsetToHasCache,
                uint8_t _uHasCacheBit
                );

            bool __YYAPI GetGeneralLocalValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI GetGeneralSpecifiedValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI SetGeneralCacheValue(
                ValueType _eType,
                uint16_t _uOffsetToCache,
                uint8_t _uCacheBit,
                uint16_t _uOffsetToHasCache,
                uint8_t _uHasCacheBit,
                Value _NewValue);

            bool __YYAPI SetGeneralLocalValue(_Inout_ SetValueHandleData* _pHandleData);

            bool __YYAPI SetGeneralSpecifiedValue(_Inout_ SetValueHandleData* _pHandleData);

            bool __YYAPI GetGeneralFastSpecValueCompare(_Inout_ FastSpecValueCompareHandleData* _pHandleData);

            bool __YYAPI ParentPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

			virtual bool __YYAPI OnParentPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData);
            
            virtual bool __YYAPI GetParentPropDependencies(GetDependenciesHandleData* _pHandleData);

            bool __YYAPI VisiblePropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            virtual bool __YYAPI OnVisiblePropChanged(_In_ OnPropertyChangedHandleData* _pHandle);
            
            virtual bool __YYAPI GetVisiblePropDependencies(_In_ GetDependenciesHandleData* _pHandleData);

            bool __YYAPI GetVisiblePropValue(_In_ GetValueHandleData* _pHandleData);
            
            bool __YYAPI SetVisiblePropValue(_In_ SetValueHandleData* _pHandleData);

			bool __YYAPI EnabledPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            virtual bool __YYAPI OnEnabledPropChanged(_In_ OnPropertyChangedHandleData* _pHandle);

			bool __YYAPI ActivePropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            virtual bool __YYAPI OnActivePropChanged(_In_ OnPropertyChangedHandleData* _pHandle);
			
            bool __YYAPI DpiPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            virtual bool __YYAPI OnDpiPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData);

            void __YYAPI FlushDesiredSize(DeferCycle* _pDeferCycle);

            void __YYAPI FlushLayout(DeferCycle* _pDeferCycle);

            static bool __YYAPI SetGroupChanges(Element* pElement, uint32_t _fGroups, DeferCycle* pDeferCycle);

            static void __YYAPI TransferGroupFlags(Element* pElement, uint32_t _fGroups);
            
            static bool __YYAPI MarkElementForDesiredSize(Element* _pElement);

            static bool __YYAPI MarkElementForLayout(Element* _pElement, uint32_t _fNeedsLayoutNew);

            bool __YYAPI SetNeedsLayout(uint32_t _fNeedsLayoutNew);

            Size __YYAPI UpdateDesiredSize(Size _ConstraintSize);

            void __YYAPI UpdateLayoutPosition(Point _LayoutPosition);
            
            void __YYAPI UpdateLayoutSize(Size _LayoutSize);

            bool __YYAPI ExtentPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            bool __YYAPI GetExtentPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI LocationPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            bool __YYAPI GetLocationPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI MouseFocusedPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            virtual bool __YYAPI OnMouseFocusedPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData);

            bool __YYAPI GetMouseFocusedPropValue(_Inout_ GetValueHandleData* _pHandleData);
            
            bool __YYAPI MouseFocusWithinPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            bool __YYAPI GetMouseFocusWithinPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI KeyboardFocusedPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            virtual bool __YYAPI OnKeyboardFocusedPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData);

            bool __YYAPI GetKeyboardFocusedPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI KeyboardFocusWithinPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            bool __YYAPI GetKeyboardFocusWithinPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI FocusedPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            virtual bool __YYAPI OnFocusedPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData);

            bool __YYAPI GetFocusedPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI FocusWithinPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);
            
            bool __YYAPI GetFocusWithinPropValue(_Inout_ GetValueHandleData* _pHandleData);

            bool __YYAPI SheetPropHandle(_In_ CustomPropertyHandleType _eType, _Inout_ CustomPropertyBaseHandleData* _pHandleData);

            virtual bool __YYAPI GetSheetPropDependencies(_Inout_ GetDependenciesHandleData* _pHandleData);
            
            virtual HRESULT __YYAPI OnHosted(Window* _pNewWindow);

            virtual HRESULT __YYAPI OnUnHosted(Window* _pOldWindow);

            virtual Element* __YYAPI GetAdjacent(Element* _pFrom, NavigatingType _eNavDir, NavReference const* _pnr, bool _bKeyableOnly);

            virtual bool __YYAPI OnKeyDown(const KeyboardEvent& _KeyEvent);

            virtual bool __YYAPI OnChar(const KeyboardEvent& _KeyEvent);

            /// <summary>
            /// 处理键盘导航事件。
            /// </summary>
            /// <param name="_Event">键盘导航事件。</param>
            /// <returns>如果已经处理，则返回 true</returns>
            virtual bool __YYAPI OnKeyboardNavigate(const KeyboardNavigateEvent& _Event);

		};
	}
}

#pragma pack(pop)
