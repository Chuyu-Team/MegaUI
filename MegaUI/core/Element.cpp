#include "pch.h"

#include "Element.h"

#include "Property.h"
#include "value.h"
#include "ControlInfoImp.h"
#include "Layout.h"
#include <MegaUI/Window/Window.h>
#include "StyleSheet.h"
#include <MegaUI/base/ComPtr.h>
#include <Multimedia/Font.h>

#pragma warning(disable : 28251)
#pragma warning(disable : 26812)

namespace YY
{
	namespace MegaUI
	{
        static const EnumMap LayoutPosEnumMap[] =
        {
            { "None", LP_None },
            { "Absolute", LP_Absolute },
            { "Auto", LP_Auto },
            { }
        };

        static const EnumMap BorderStyleEnumMap[] =
        {
            { "Solid",   int32_t(BorderStyle::Solid) },
            { "Raised",  int32_t(BorderStyle::Raised) },
            { "Sunken",  int32_t(BorderStyle::Sunken) },
            { "Rounded", int32_t(BorderStyle::Rounded) },
            { }
        };

        static const EnumMap FlowDirectionEnumMap[] =
        {
            { "LeftToRight", (int32_t)FlowDirection::LeftToRight },
            { "RightToLeft", (int32_t)FlowDirection::RightToLeft },
            { }
        };
        
        static const EnumMap ActiveEnumMap[] =
        {
            { "Inactive", Active::Inactive },
            { "Mouse", Active::Mouse },
            { "Keyboard", Active::Keyboard },
            { }
        };

        static const EnumMap ContentAlignEnumMap[] =
        {
            { "Left", ContentAlign::Left },
            { "Center", ContentAlign::Center },
            { "Right", ContentAlign::Right },
            { "Top", ContentAlign::Top },
            { "Middle", ContentAlign::Middle },
            { "Bottom", ContentAlign::Bottom },
            { "Wrap", ContentAlign::Wrap },
            { "EndEllipsis", ContentAlign::EndEllipsis },
            { }
        };

        static constexpr const EnumMap FontWeightEnumMaps[] =
        {
            {"Thin", FontWeight::Thin},
            {"ExtraLight", FontWeight::ExtraLight},
            {"UltraLight", FontWeight::UltraLight},
            {"Light", FontWeight::Light},
            {"SemiLight", FontWeight::SemiLight},
            {"Normal", FontWeight::Normal},
            {"Regular", FontWeight::Regular},
            {"Medium", FontWeight::Medium},
            {"DemiBold", FontWeight::DemiBold},
            {"SemiBold", FontWeight::SemiBold},
            {"Bold", FontWeight::Bold},
            {"ExtraBold", FontWeight::ExtraBold},
            {"UltraBold", FontWeight::UltraBold},
            {"Black", FontWeight::Black},
            {"Heavy", FontWeight::Heavy},
            {"ExtraBlack", FontWeight::ExtraBlack},
            {"UltraBlack", FontWeight::UltraBlack},
            { },
        };

        static constexpr const EnumMap FontStyleEnumMap[] =
        {
            {"None", FontStyle::None},
            {"Italic", FontStyle::Italic},
            {"Underline", FontStyle::Underline},
            {"StrikeOut", FontStyle::StrikeOut},
            {},
        };

		_APPLY_MEGA_UI_STATIC_CONTROL_INFO(Element, _MEGA_UI_ELEMENT_PROPERTY_TABLE);

        Element::Element()
            : RenderNode{}
            , _iGCSlot(-1)
            , _iGCLPSlot(-1)
            , _iPCTail(-1)
            , pDeferObject(nullptr)
            , pTopLevel(nullptr)
            , pLocParent(nullptr)
            , LocPosInLayout {}
            , LocSizeInLayout {}
            , LocDesiredSize {}
            , LocLastDesiredSizeConstraint {}
            , iSpecLayoutPos(g_ControlInfoData.LayoutPosProp.pFunDefaultValue().GetInt32())
            , pSheet(nullptr)
            , SpecID(0)
            , bSelfLayout(false)
            , fNeedsLayout(0)
            , bLocMouseFocusWithin(false)
            , bDestroy(false)
            , bSpecVisible(false)
            , bCmpVisible(false)
            , bSpecEnabled(true)
            , bHasLocMouseFocused(false)
            , bLocMouseFocused(false)
            , bSpecMouseFocused(false)
            , bSpecFocusVisible(false)
            , bNeedsDSUpdate(true)
            , bLocKeyboardFocusWithin(false)
            , bHasLocKeyboardFocused(false)
            , bLocKeyboardFocused(false)
            , bSpecKeyboardFocused(false)
            , bLocFocusWithin(false)
            , bHasLocFocused(false)
            , bLocFocused(false)
            , bSpecFocused(false)
        {
            SpecFont.szFace = Element::g_ControlInfoData.FontFamilyProp.pFunDefaultValue().GetString();
            SpecFont.iSize = Element::g_ControlInfoData.FontSizeProp.pFunDefaultValue().GetFloat();
            SpecFont.uWeight = Element::g_ControlInfoData.FontWeightProp.pFunDefaultValue().GetInt32();
            SpecFont.fStyle = Element::g_ControlInfoData.FontStyleProp.pFunDefaultValue().GetInt32();
        }

        Element::~Element()
        {
        }

        HRESULT Element::Initialize(int32_t _iDPI, uint32_t _fCreate, Element* _pTopLevel, intptr_t* _pCooike)
        {
            pTopLevel = _pTopLevel;
            iLocDpi = _iDPI;

            if (_pCooike)
                StartDefer(_pCooike);

            return S_OK;
        }

		Value Element::GetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, bool _bUpdateCache)
        {
            if (_eIndicies >= PropertyIndicies::PI_MAX)
                return Value::CreateUnavailable();

            auto _uIndicies = uint32_t(_eIndicies);

            for (; _uIndicies && (_Prop.fFlags & (1u << (_uIndicies - 1))) == 0; --_uIndicies)
            {
                _eIndicies = PropertyIndicies(_uIndicies);
            }

            const auto _iIndex = GetControlInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return Value::CreateUnavailable();

            Value _pValue = Value::CreateUnset();

            do
            {
                GetValueHandleData _HandleData;
                _HandleData.pProp = &_Prop;
                _HandleData.eIndicies = _eIndicies;
                _HandleData.bUsingCache = _bUpdateCache == false;
                if (GeneralHandle(&_HandleData))
                {
                    if (_HandleData.Output.RetValue != nullptr)
                    {
                        _pValue = std::move(_HandleData.Output.RetValue);

                        if (_pValue.GetType() != ValueType::Unset)
                            break;
                    }
                }

                PropertyCustomCacheResult& _CacheResult = _HandleData.Output.CacheResult;

                // 走 _LocalPropValue，这是一种通用逻辑
                if ((_CacheResult & SkipLocalPropValue) == 0)
                {
                    auto _ppValue = LocalPropValue.GetItemPtr(_iIndex);
                    if (_ppValue && *_ppValue != nullptr)
                    {
                        _pValue = *_ppValue;
                        break;
                    }
                }

                // 如果值是本地的，那么最多就取一下 _LocalPropValue，我们就需要停止
                if (_eIndicies == PropertyIndicies::PI_Local)
                    break;

                // 尝试获取来自属性表的值
                if ((_Prop.fFlags & PF_Cascade) && (_CacheResult & SkipCascade) == 0)
                {
                    if (pSheet)
                    {
                        _pValue = pSheet->GetSheetValue(this, &_Prop);
                        if (_pValue.GetType() != ValueType::Unset)
                            break;
                    }
                }

                // 尝试从父节点继承
                if ((_Prop.fFlags & PF_Inherit) && (_CacheResult & SkipInherit) == 0)
                {
                    if (auto _pParent = GetParent())
                    {
                        auto pValueByParent = _pParent->GetValue(_Prop, _eIndicies, false);
                        auto ValueByParentType = pValueByParent.GetType();
                        if (ValueByParentType != ValueType::Unset && ValueByParentType != ValueType::Unavailable)
                        {
                            _pValue = std::move(pValueByParent);
                            break;
                        }
                    }
                }

                // 最终还是没有，那么继承Default 值
                if (_Prop.pFunDefaultValue)
                    _pValue = _Prop.pFunDefaultValue();

            } while (false);

            if (_pValue.GetType() >= ValueType::Null && _bUpdateCache)
            {
                SetValueHandleData _Info;
                _Info.pProp = &_Prop;
                _Info.eIndicies = _eIndicies;
                _Info.InputNewValue = _pValue;

                GeneralHandle(&_Info);
            }

            if (_pValue == nullptr)
                _pValue = Value::CreateUnset();

            return _pValue;
        }

        HRESULT Element::SetValue(const PropertyInfo& _Prop, const Value& _Value)
        {
            if (_Value == nullptr)
                return E_INVALIDARG;

            if (_Prop.fFlags & PF_ReadOnly)
                return E_NOTIMPL;

            return SetValueInternal(_Prop, _Value, true);
        }

        HRESULT Element::SetValueInternal(const PropertyInfo& _Prop, const Value& _Value, bool _bCanCancel)
        {
            const auto _iIndex = GetControlInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return E_NOT_SET;

            const auto _uIndex = (size_t)_iIndex;

            auto _pvOld = GetValue(_Prop, PropertyIndicies::PI_Local, false);
            if (_pvOld == nullptr)
                return E_OUTOFMEMORY;

            if (_pvOld == _Value)
                return S_OK;

            if (_bCanCancel && OnPropertyChanging(_Prop, PropertyIndicies::PI_Local, _pvOld, _Value) == false)
                return __HRESULT_FROM_WIN32(ERROR_CANCELLED);

            PreSourceChange(_Prop, PropertyIndicies::PI_Local, _pvOld, _Value);

            auto _hr = S_OK;
            do
            {
                SetValueHandleData _HandleData;
                _HandleData.pProp = &_Prop;
                _HandleData.eIndicies = PropertyIndicies::PI_Local;
                _HandleData.InputNewValue = _Value;

                if (GeneralHandle(&_HandleData))
                    break;

                const auto _uLocalPropValueCount = LocalPropValue.GetSize();

                if (_Value.GetType() == ValueType::Unset)
                {
                    // 将数值取消
                    if (_uLocalPropValueCount > _uIndex)
                        _hr = LocalPropValue.SetItem(_uIndex, Value());
                }
                else
                {
                    if (_uLocalPropValueCount <= _uIndex)
                        _hr = LocalPropValue.Resize(_uIndex + 1);

                    if (SUCCEEDED(_hr))
                        _hr = LocalPropValue.SetItem(_uIndex, _Value);
                }

            } while (false);

            PostSourceChange();

            return _hr;
        }

        Element* Element::GetParent()
        {
            return pLocParent;
        }

        int32_t Element::GetLayoutPos()
        {
            return iSpecLayoutPos;
        }

        HRESULT Element::SetLayoutPos(int32_t _iLayoutPos)
        {
            auto _pValue = Value::CreateInt32(_iLayoutPos);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.LayoutPosProp, _pValue);
        }

        float Element::GetWidth()
        {
            auto _pValue = GetValue(Element::g_ControlInfoData.WidthProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }
            return _pValue.GetFloat();
        }

        HRESULT Element::SetWidth(float _iWidth)
        {
            auto _pValue = Value::CreateFloat(_iWidth);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.WidthProp, _pValue);
        }

        float Element::GetHeight()
        {
            auto _pValue = GetValue(Element::g_ControlInfoData.HeightProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }
            return _pValue.GetFloat();
        }

        HRESULT Element::SetHeight(float _iHeight)
        {
            auto _pValue = Value::CreateFloat(_iHeight);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.HeightProp, _pValue);
        }

        float Element::GetX()
        {
            auto _pValue = GetValue(Element::g_ControlInfoData.XProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }

            return _pValue.GetFloat();
        }

        HRESULT Element::SetX(float _iX)
        {
            auto _pValue = Value::CreateFloat(_iX);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.XProp, _pValue);
        }

        float Element::GetY()
        {
            auto _pValue = GetValue(Element::g_ControlInfoData.YProp, PropertyIndicies::PI_Specified, false);
            if (_pValue == nullptr)
            {
                throw Exception();
                return -1;
            }

            return _pValue.GetFloat();
        }

        HRESULT Element::SetY(float _iY)
        {
            auto _pValue = Value::CreateFloat(_iY);
            if (_pValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.YProp, _pValue);
        }

        Point Element::GetLocation()
        {
            Point _Location = {};
            auto _pValue = GetValue(Element::g_ControlInfoData.LocationProp, PropertyIndicies::PI_Local, false);

            if (_pValue != nullptr)
            {
                _Location = _pValue.GetPoint();
            }

            return _Location;
        }

        Size Element::GetExtent()
        {
            Size _Extent = {};

            auto _pValue = GetValue(Element::g_ControlInfoData.ExtentProp, PropertyIndicies::PI_Local, false);
            if (_pValue != nullptr)
            {
                _Extent = _pValue.GetSize();
            }
            return _Extent;
        }

        ValueIs<ValueType::Layout> Element::GetLayout()
        {
            return GetValue(Element::g_ControlInfoData.LayoutProp, PropertyIndicies::PI_Specified, false);
        }

        BorderStyle Element::GetBorderStyle()
        {
            int32_t _iValue = 0;

            auto _pValue = GetValue(Element::g_ControlInfoData.BorderStyleProp, PropertyIndicies::PI_Specified, false);
            if (_pValue != nullptr)
            {
                _iValue = _pValue.GetInt32();
            }
            return BorderStyle(_iValue);
        }

        HRESULT Element::SetBorderStyle(BorderStyle _eBorderStyle)
        {
            auto _NewValue = Value::CreateInt32((int32_t)_eBorderStyle);
            if (_NewValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.BorderStyleProp, _NewValue);
        }

        Color Element::GetBorderColor()
        {
            auto _BorderColorValue = GetValue(Element::g_ControlInfoData.BorderColorProp);
            return _BorderColorValue.HasValue() ? _BorderColorValue.GetColor() : Color();
        }

        HRESULT Element::SetBorderColor(Color _BorderColor)
        {
            auto _NewValue = Value::CreateColor(_BorderColor);
            if (_NewValue == nullptr)
                return E_OUTOFMEMORY;
            
            return SetValue(Element::g_ControlInfoData.BorderColorProp, _NewValue);
        }

        BorderStyle Element::GetFocusBorderStyle()
        {
            int32_t _iValue = 0;

            auto _pValue = GetValue(Element::g_ControlInfoData.FocusStyleProp, PropertyIndicies::PI_Specified, false);
            if (_pValue != nullptr)
            {
                _iValue = _pValue.GetInt32();
            }
            return BorderStyle(_iValue);
        }

        HRESULT Element::SetFocusBorderStyle(BorderStyle _eBorderStyle)
        {
            auto _NewValue = Value::CreateInt32(int32_t(_eBorderStyle));
            if (_NewValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FocusStyleProp, _NewValue);
        }

        Color Element::GetFocusBorderColor()
        {
            auto _FocusBorderColorValue = GetValue(Element::g_ControlInfoData.FocusColorProp);
            return _FocusBorderColorValue.HasValue() ? _FocusBorderColorValue.GetColor() : Color();
        }

        HRESULT Element::SetFocusBorderColor(Color _BorderColor)
        {
            auto _NewValue = Value::CreateColor(_BorderColor);
            if (_NewValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FocusColorProp, _NewValue);
        }

        FlowDirection Element::GetFlowDirection()
        {
            return iSpecFlowDirection;
        }

        HRESULT Element::SetFlowDirection(FlowDirection _eFlowDirection)
        {
            switch (_eFlowDirection)
            {
            case FlowDirection::LeftToRight:
            case FlowDirection::RightToLeft:
            {
                auto _FlowDirectionValue = Value::CreateInt32((int32_t)_eFlowDirection);
                if (_FlowDirectionValue == nullptr)
                    return E_OUTOFMEMORY;

                return SetValue(Element::g_ControlInfoData.FlowDirectionProp, _FlowDirectionValue);
                break;
            }
            default:
                return E_INVALIDARG;
                break;
            }
        }

        bool Element::IsMouseFocusWithin()
        {
            return bLocMouseFocusWithin;
        }

        uString Element::GetClass()
        {
            auto _Value = GetValue(Element::g_ControlInfoData.ClassProp, PropertyIndicies::PI_Specified, false);
            if (_Value.GetType() == ValueType::uString)
            {
                return _Value.GetString();
            }

            return uString();
        }

        HRESULT Element::SetClass(uString _szClass)
        {
            auto _NewValue = Value::CreateString(_szClass);
            if (_NewValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.ClassProp, _NewValue);
        }

        bool Element::GetEnabled()
        {
            return bSpecEnabled;
        }

        HRESULT Element::SetEnabled(bool _bEnabled)
        {
            return SetValue(Element::g_ControlInfoData.EnabledProp, Value::CreateBool(_bEnabled));
        }

        uint32_t Element::GetActive()
        {
            return uSpecActive;
        }

        HRESULT Element::SetActive(uint32_t _fActive)
        {
            return SetValue(Element::g_ControlInfoData.ActiveProp, Value::CreateInt32(_fActive));
        }

        bool Element::GetMouseFocused()
        {
            return bSpecMouseFocused;
        }

        int32_t Element::GetDpi()
        {
            return iLocDpi;
        }

        bool Element::OnPropertyChanging(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _OldValue, _In_ const Value& _NewValue)
        {
            return true;
        }

		void Element::OnPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, const Value& _OldValue, const Value& _NewValue)
		{
            if (_Prop.pfnCustomPropertyHandle)
            {
                OnPropertyChangedHandleData _HandleData;
                _HandleData.pProp = &_Prop;
                _HandleData.eIndicies = _eIndicies;
                _HandleData.OldValue = _OldValue;
                _HandleData.NewValue = _NewValue;

                (this->*_Prop.pfnCustomPropertyHandle)(CustomPropertyHandleType::OnPropertyChanged, &_HandleData);
            }
        }

        void Element::OnGroupChanged(uint32_t _fGroups)
        {
            if (_fGroups == 0)
                return;
            
            auto _pDeferObject = GetDeferObject();
            if (!_pDeferObject)
                return;

            if (_fGroups & PG_NormalPriMask)
            {
                // Affects Desired Size or Affects Layout
                if (_fGroups & (PG_AffectsDesiredSize | PG_AffectsLayout))
                {
                    // 找到通知树的根节点
                    auto _pRoot = this;
                    while (_pRoot->GetLayoutPos() != LP_Absolute)
                    {
                        auto _pParent = _pRoot->GetParent();
                        if (!_pParent)
                            break;

                        _pRoot = _pParent;
                    }

                    if (_fGroups & PG_AffectsDesiredSize)
                    {
                        _pDeferObject->UpdateDesiredSizeRootPendingSet.Insert(_pRoot);
                    }

                    if (_fGroups & PG_AffectsLayout)
                    {
                        _pDeferObject->LayoutRootPendingSet.Insert(_pRoot);
                    }
                }

                if (_fGroups & (PG_AffectsParentDesiredSize | PG_AffectsParentLayout))
                {
                    // Locate Layout/DS root and queue tree as needing a layout pass
                    // Root doesn't have parent or is absolute positioned
                    Element* _pRoot;
                    Element* _pParent = nullptr;

                    for (Element* _pTmp = this;;)
                    {
                        _pRoot = _pTmp;

                        _pTmp = _pRoot->GetParent();
                        if (_pTmp && !_pParent)
                            _pParent = _pTmp;

                        if (_pTmp == nullptr || _pRoot->GetLayoutPos() == LP_Absolute)
                            break;
                    }

                    if (_pParent)
                    {
                        if (_fGroups & PG_AffectsParentDesiredSize)
                        {
                            _pDeferObject->UpdateDesiredSizeRootPendingSet.Insert(_pRoot);
                        }

                        if (_fGroups & PG_AffectsParentLayout)
                        {
                            _pDeferObject->LayoutRootPendingSet.Insert(_pRoot);
                        }
                    }
                }
            }
            else if (_fGroups & PG_LowPriMask)
            {
                auto _uAddInvalidateMarks = 0u;
                if (_fGroups & PG_AffectsBounds)
                {
                    _uAddInvalidateMarks |= ElementRenderNode::InvalidatePosition | ElementRenderNode::InvalidateExtent;
                }

                if (_fGroups & PG_AffectsDisplay)
                {
                    _uAddInvalidateMarks |= ElementRenderNode::InvalidateContent;
                }

                // 如果
                if (_uAddInvalidateMarks)
                {
                    // TODO：暂时没有做局部更新，统一刷新窗口
                    if (_uAddInvalidateMarks & (ElementRenderNode::InvalidatePosition | ElementRenderNode::InvalidateExtent))
                    {
                        #if 1
                        if (pWindow)
                            pWindow->InvalidateRect(nullptr);
                        #else
                        Rect _InvalidateRectOld = RenderNode.Bounds;
                        Rect _InvalidateRectNew(POINT {}, GetExtent());
                        Window* _pWindow = nullptr;

                        for (auto _pItem = this;;)
                        {
                            _pWindow = _pItem->TryCast<Window>();
                            if (_pWindow)
                                break;

                            auto _pParent = _pItem->GetParent();
                            if (!_pParent)
                                break;

                            if (_pParent->RenderNode.uInvalidateMarks)
                                break;

                            auto _Location = _pItem->GetLocation();

                            _InvalidateRectNew.Left += _Location.x;
                            _InvalidateRectNew.Right += _Location.x;
                            _InvalidateRectNew.Top += _Location.y;
                            _InvalidateRectNew.Bottom += _Location.y;

                            Rect _ParentRect(POINT {}, _pParent->GetExtent());

                            if (IntersectRect(&_InvalidateRectNew, &_InvalidateRectNew, &_ParentRect) == FALSE
                                && IntersectRect(&_InvalidateRectOld, &_InvalidateRectOld, &_pParent->RenderNode.Bounds) == FALSE)
                            {
                                // 显式区域为空，无需刷新 UI
                                break;
                            }

                            _pItem = _pParent;
                        }

                        if (_pWindow)
                            _pWindow->InvalidateRect(_InvalidateRectOld | _InvalidateRectNew);
                        #endif
                    }
                    else if (_uAddInvalidateMarks & ElementRenderNode::InvalidateContent)
                    {
                        #if 1
                        if (pWindow)
                            pWindow->InvalidateRect(nullptr);
                        #else
                        Rect _InvalidateRect = RenderNode.Bounds;
                        Window* _pWindow = nullptr;
                        for (Element* _pItem = this;;)
                        {
                            _pWindow = _pItem->TryCast<Window>();
                            if (_pWindow)
                                break;

                            _pItem = _pItem->GetParent();
                            if (!_pItem)
                                break;

                            if (_pItem->RenderNode.uInvalidateMarks)
                                break;

                            if (!IntersectRect(&_InvalidateRect, &_InvalidateRect, &_pItem->RenderNode.Bounds))
                            {
                                // 显式区域为空，无需刷新 UI
                                break;
                            }
                        }

                        if (_pWindow)
                            _pWindow->InvalidateRect(_InvalidateRect);
                        #endif
                    }

                    RenderNode.uInvalidateMarks |= _uAddInvalidateMarks;
                }
            }
        }

        Element* Element::GetTopLevel()
        {
            auto _pTopLevel = this;

            for (; _pTopLevel->pTopLevel; _pTopLevel = _pTopLevel->pTopLevel);

            return _pTopLevel;
        }

        DeferCycle* Element::GetDeferObject(bool _bAllowCreate)
        {
            if (pDeferObject)
                return pDeferObject;

            auto _pTopLevel = GetTopLevel();
            if (_pTopLevel->pDeferObject)
                return _pTopLevel->pDeferObject;

            if (!_bAllowCreate)
                return nullptr;

            auto _pDeferObject = HNew<DeferCycle>();
            if (!_pDeferObject)
                return nullptr;

            _pDeferObject->AddRef();
            _pTopLevel->pDeferObject = _pDeferObject;
            return _pDeferObject;
        }

        void Element::StartDefer(intptr_t* _pCooike)
		{
            *_pCooike = 0;
            
			if (auto _pDeferCycle = GetDeferObject())
			{
                // 随便写一个值，看起来比较特殊就可以了
                *_pCooike = 0x12345;

                _pDeferCycle->AddRef();
                ++_pDeferCycle->uEnter;
			}
        }

        uString Element::GetFontFamily()
        {
            return SpecFont.szFace;
        }

        HRESULT Element::SetFontFamily(uString _szFontFamily)
        {
            auto _FontFamilyValue = Value::CreateString(_szFontFamily);
            if (_FontFamilyValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FontFamilyProp, _FontFamilyValue);
        }

        float Element::GetFontSize()
        {
            return SpecFont.iSize;
        }

        HRESULT Element::SetFontSize(float _iFontSize)
        {
            auto _FontSizeValue = Value::CreateFloat(_iFontSize);
            if (_FontSizeValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FontSizeProp, _FontSizeValue);
        }

        uint32_t Element::GetFontWeight()
        {
            return SpecFont.uWeight;
        }

        HRESULT Element::SetFontWeight(uint32_t _iFontWeight)
        {
            auto _FontWeightValue = Value::CreateInt32(_iFontWeight);
            if (_FontWeightValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FontWeightProp, _FontWeightValue);
        }

        uint32_t Element::GetFontStyle()
        {
            return SpecFont.fStyle;
        }

        HRESULT Element::SetFontStyle(uint32_t _fFontStyle)
        {
            auto _FontStyleValue = Value::CreateInt32(_fFontStyle);
            if (_FontStyleValue == nullptr)
                return E_OUTOFMEMORY;

            return SetValue(Element::g_ControlInfoData.FontStyleProp, _FontStyleValue);
        }

		void Element::EndDefer(intptr_t _Cookie)
		{
            auto _pDeferCycle = GetDeferObject(false);
            if (!_pDeferCycle)
                return;

            if (_Cookie != 0x12345)
			{
				throw std::exception("Cookie Error", 0);
				return;
			}

            if (_pDeferCycle->uEnter == 1)
            {
                // StartDefer计数归零，开始应用更改。
                _pDeferCycle->bFiring = true;

                auto& vecGroupChangeNormalPriority = _pDeferCycle->vecGroupChangeNormalPriority;
                auto& vecGroupChangeLowPriority = _pDeferCycle->vecGroupChangeLowPriority;
                auto& vecPropertyChanged = _pDeferCycle->vecPropertyChanged;
                auto& LayoutRootPendingSet = _pDeferCycle->LayoutRootPendingSet;
                auto& UpdateDesiredSizeRootPendingSet = _pDeferCycle->UpdateDesiredSizeRootPendingSet;

                auto& uGroupChangeNormalPriorityFireCount = _pDeferCycle->uGroupChangeNormalPriorityFireCount;
                auto& uGroupChangeLowPriorityFireCount = _pDeferCycle->uGroupChangeLowPriorityFireCount;

                for (;;)
                {
                    // 首先通知正常优先级的 Group 队列
                    if (uGroupChangeNormalPriorityFireCount < vecGroupChangeNormalPriority.GetSize())
                    {
                        auto pGroupChangeRecord = vecGroupChangeNormalPriority.GetItemPtr(uGroupChangeNormalPriorityFireCount);
                        ++uGroupChangeNormalPriorityFireCount;

                        if (pGroupChangeRecord && pGroupChangeRecord->pElement)
                        {
                            pGroupChangeRecord->pElement->_iGCSlot = -1;
                            _pDeferCycle->Release();

                            pGroupChangeRecord->pElement->OnGroupChanged(pGroupChangeRecord->fGroups & PropertyGroup::PG_NormalPriMask);
                        }
                        continue;
                    }

                    // 然后更新 DesiredSize
                    if(auto pFistPending = UpdateDesiredSizeRootPendingSet.Pop())
                    {
                        pFistPending->FlushDesiredSize(_pDeferCycle);
                        continue;
                    }

                    // 然后更新布局
                    if (auto pFistPending = LayoutRootPendingSet.Pop())
                    {
                        pFistPending->FlushLayout(_pDeferCycle);
                        continue;
                    }

                    // 最后更新低优先级的 Group
                    if (uGroupChangeLowPriorityFireCount < vecGroupChangeLowPriority.GetSize())
                    {
                        auto pGroupChangeRecord = vecGroupChangeLowPriority.GetItemPtr(uGroupChangeLowPriorityFireCount);
                        ++uGroupChangeLowPriorityFireCount;

                        if (pGroupChangeRecord && pGroupChangeRecord->pElement)
                        {
                            pGroupChangeRecord->pElement->_iGCLPSlot = -1;
                            _pDeferCycle->Release();

                            pGroupChangeRecord->pElement->OnGroupChanged(pGroupChangeRecord->fGroups & PropertyGroup::PG_LowPriMask);
                        }
                        continue;
                    }

                    // 处理完成
                    vecGroupChangeNormalPriority.Clear();
                    uGroupChangeNormalPriorityFireCount = 0;
                    vecGroupChangeLowPriority.Clear();
                    uGroupChangeLowPriorityFireCount = 0;

                    _pDeferCycle->bFiring = false;
                    break;
                }
            }

            --_pDeferCycle->uEnter;
        }

        HRESULT Element::SetVisible(bool bVisible)
        {
            return SetValue(Element::g_ControlInfoData.VisibleProp, Value::CreateBool(bVisible));
        }

        bool Element::GetVisible()
        {
            return bCmpVisible;
        }
        
        void Element::OnDestroy()
        {
            // TODO
        }

        HRESULT Element::Destroy(bool _fDelayed)
        {
            if (pLocParent)
            {
                auto _hr = pLocParent->Remove(this);
                if (FAILED(_hr))
                    return _hr;
            }
            
            // 如果没有绑定窗口，那么无视延迟释放
            if (_fDelayed == false || pWindow == nullptr)
            {
                DestroyAllChildren(false);
                HDelete(this);
                return S_OK;
            }
            else
            {
                // 延后释放时会自动递归释放子控件
                return pWindow->PostDelayedDestroyElement(this);
            }
        }

        HRESULT Element::DestroyAllChildren(bool _fDelayed)
        {
            auto _Children = GetChildren();
            if (_Children.GetSize() == 0)
                return S_OK;

            auto _hr = RemoveAll();
            if (FAILED(_hr))
                return _hr;

            for (auto pItem : _Children)
            {
                pItem->Destroy(_fDelayed);
            }

            return S_OK;
        }

        void Element::Paint(_In_ Render* _pRenderTarget, _In_ const Rect& _Bounds)
        {
            Rect _PaintBounds = _Bounds;
            if (SpecBorderThickness.Left != 0 || SpecBorderThickness.Top != 0 || SpecBorderThickness.Right != 0 || SpecBorderThickness.Bottom != 0)
            {
                PaintBorder(
                    _pRenderTarget,
                    GetBorderStyle(),
                    ApplyFlowDirection(SpecBorderThickness),
                    GetBorderColor(),
                    _PaintBounds);
            }

            PaintBackground(
                _pRenderTarget,
                GetValue(Element::g_ControlInfoData.BackgroundProp, PropertyIndicies::PI_Specified, false),
                _PaintBounds);

            // 应用内边距
            _PaintBounds.DeflateRect(ApplyFlowDirection(SpecPadding));

            // 绘制焦点框
            if (bSpecFocusVisible 
                && (SpecFocusThickness.Left != 0 || SpecFocusThickness.Top != 0 || SpecFocusThickness.Right != 0 || SpecFocusThickness.Bottom != 0))
            {
                PaintBorder(
                    _pRenderTarget,
                    GetFocusBorderStyle(),
                    ApplyFlowDirection(SpecFocusThickness),
                    GetFocusBorderColor(),
                    _PaintBounds);
            }

            PaintContent(
                _pRenderTarget,
                GetValue(Element::g_ControlInfoData.ContentProp),
                SpecFont,
                GetValue(Element::g_ControlInfoData.ForegroundProp).GetColor(),
                _PaintBounds,
                fSpecContentAlign);
        }

        void Element::PaintBorder(Render* _pRenderTarget, BorderStyle _eBorderStyle, const Rect& _BorderThickness, Color _BorderColor, Rect& _Bounds)
        {
            if (_BorderThickness.Left == 0 && _BorderThickness.Top == 0 && _BorderThickness.Right == 0 && _BorderThickness.Bottom == 0)
                return;

            Rect _NewBounds = _Bounds;
            _NewBounds.DeflateRect(_BorderThickness);

            if (_BorderColor.Alpha != 0)
            {
                switch (_eBorderStyle)
                {
                case BorderStyle::Solid:
                {
                    ComPtr<ID2D1SolidColorBrush> _BorderBrush;
                    auto hr = _pRenderTarget->CreateSolidColorBrush(
                        _BorderColor,
                        &_BorderBrush);

                    if (SUCCEEDED(hr))
                    {
                        // 左边
                        _pRenderTarget->FillRectangle({_Bounds.Left, _NewBounds.Top, _NewBounds.Left, _NewBounds.Bottom}, _BorderBrush);
                        // 上面
                        _pRenderTarget->FillRectangle({_Bounds.Left, _Bounds.Top, _Bounds.Right, _NewBounds.Top}, _BorderBrush);
                        // 右边
                        _pRenderTarget->FillRectangle({_NewBounds.Right, _Bounds.Top, _Bounds.Right, _Bounds.Bottom}, _BorderBrush);
                        // 下面
                        _pRenderTarget->FillRectangle({_Bounds.Left, _NewBounds.Bottom, _Bounds.Right, _Bounds.Bottom}, _BorderBrush);
                    }
                    break;
                }
                case BorderStyle::Raised:
                case BorderStyle::Sunken:
                {
                    Rect _BoundsOutter = _Bounds;
                    _BoundsOutter.DeflateRect({_BorderThickness.Left / 2, _BorderThickness.Top / 2, _BorderThickness.Right / 2, _BorderThickness.Bottom / 2});

                    ComPtr<ID2D1SolidColorBrush> hbOLT;       // Brush for outter Left and Top
                    ComPtr<ID2D1SolidColorBrush> hbORB;       // Brush for outter Right and Bottom
                    ComPtr<ID2D1SolidColorBrush> hbILT;       // Brush for inner Left Top
                    ComPtr<ID2D1SolidColorBrush> hbIRB;       // Brush for inner Right and Bottom

                    if (_eBorderStyle == BorderStyle::Raised)
                    {
                        _pRenderTarget->CreateSolidColorBrush(
                            _BorderColor,
                            &hbOLT);
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, VERYDARK),
                            &hbORB);
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, VERYLIGHT),
                            &hbILT);
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, DARK),
                            &hbIRB);
                    }
                    else
                    {
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, VERYDARK),
                            &hbOLT);
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, VERYLIGHT),
                            &hbORB);
                        _pRenderTarget->CreateSolidColorBrush(
                            AdjustBrightness(_BorderColor, DARK),
                            &hbILT);
                        _pRenderTarget->CreateSolidColorBrush(
                            _BorderColor,
                            &hbIRB);
                    }

                    // Paint etches
                    _pRenderTarget->FillRectangle({_Bounds.Left, _Bounds.Top, _BoundsOutter.Left, _BoundsOutter.Bottom}, hbOLT);            // Outter left
                    _pRenderTarget->FillRectangle({_BoundsOutter.Left, _Bounds.Top, _BoundsOutter.Right, _BoundsOutter.Top}, hbOLT);        // Outter Top
                    _pRenderTarget->FillRectangle({_BoundsOutter.Right, _Bounds.Top, _Bounds.Right, _Bounds.Bottom}, hbORB);                // Outter Right
                    _pRenderTarget->FillRectangle({_Bounds.Left, _BoundsOutter.Bottom, _BoundsOutter.Right, _Bounds.Bottom}, hbORB);        // Outter Bottom
                    _pRenderTarget->FillRectangle({_BoundsOutter.Left, _BoundsOutter.Top, _NewBounds.Left, _NewBounds.Bottom}, hbILT);      // Inner Left
                    _pRenderTarget->FillRectangle({_NewBounds.Left, _BoundsOutter.Top, _NewBounds.Right, _NewBounds.Top}, hbILT);           // Inner Top
                    _pRenderTarget->FillRectangle({_NewBounds.Right, _BoundsOutter.Top, _BoundsOutter.Right, _BoundsOutter.Bottom}, hbIRB); // Inner Right
                    _pRenderTarget->FillRectangle({_BoundsOutter.Left, _NewBounds.Bottom, _NewBounds.Right, _BoundsOutter.Bottom}, hbIRB);  // Inner Bottom
                    break;
                }
                default:
                    break;
                }
            }

            _Bounds = _NewBounds;
        }

        void Element::PaintBackground(_In_ Render* _pRenderTarget, const Value& _Background, _In_ const Rect& _Bounds)
        {
            if (_Background.GetType() == ValueType::Color)
            {
                auto _BackgroundColor = _Background.GetColor();
                if (_BackgroundColor.Alpha == 0)
                    return;

                ComPtr<ID2D1SolidColorBrush> _BackgroundBrush;
                auto hr = _pRenderTarget->CreateSolidColorBrush(
                    _Background.GetColor(),
                    &_BackgroundBrush);

                if (FAILED(hr))
                    return;

                _pRenderTarget->FillRectangle(_Bounds, _BackgroundBrush);
            }
        }

        void Element::PaintContent(
            Render* _pRenderTarget,
            const Value& _Content,
            const Font& _FontInfo,
            Color _ForegroundColor,
            const Rect& _Bounds,
            int32_t _fContentAlign
            )
        {
            if (_Content.GetType() == ValueType::uString)
            {
                _pRenderTarget->DrawString(_Content.GetString(), _FontInfo, _ForegroundColor, _Bounds, _fContentAlign);
            }
        }

        Size Element::GetContentSize(Size _ConstraintSize)
        {
            // todo
            Size _ContentSize;

            auto _ContentValue = GetValue(Element::g_ControlInfoData.ContentProp);
            switch (_ContentValue.GetType())
            {
            case ValueType::uString:
            {
                auto _pRender = pWindow->GetRender();
                _pRender->MeasureString(_ContentValue.GetString(), SpecFont, _ConstraintSize, fSpecContentAlign, &_ContentSize);
                break;
            }
            default:
                return _ConstraintSize;
                __debugbreak();
                break;
            }
            return _ContentSize;
        }

        Size Element::SelfLayoutUpdateDesiredSize(Size _ConstraintSize)
        {
            // 仅给子类留一个口，什么也不用做
            return SIZE{};
        }

        void Element::SelfLayoutDoLayout(Size _ConstraintSize)
        {
            // 仅给子类留一个口，什么也不用做
        }

        void Element::Detach(DeferCycle* _pDeferCycle)
        {
            _pDeferCycle->UpdateDesiredSizeRootPendingSet.Remove(this);
            _pDeferCycle->LayoutRootPendingSet.Remove(this);

            if (_iGCSlot != -1)
            {
                _pDeferCycle->vecGroupChangeNormalPriority[_iGCSlot].pElement = nullptr;
                _pDeferCycle->Release();
                _iGCSlot = -1;
            }

            if (_iGCLPSlot != -1)
            {
                _pDeferCycle->vecGroupChangeLowPriority[_iGCLPSlot].pElement = nullptr;
                _pDeferCycle->Release();
                _iGCLPSlot = -1;
            }

            if (_iPCTail != -1)
            {
                for (size_t index = _iPCTail; index >= _pDeferCycle->uPropertyChangedFireCount;)
                {
                    auto& _ChangedItem = _pDeferCycle->vecPropertyChanged[index];

                    if (_ChangedItem.iRefCount)
                    {
                        _ChangedItem.pElement = nullptr;
                        _ChangedItem.iRefCount = 0;
                        _ChangedItem.pNewValue = nullptr;
                        _ChangedItem.pOldValue = nullptr;

                        _pDeferCycle->Release();
                    }

                    index = _ChangedItem.iPrevElRec;
                }

                _iPCTail = -1;
            }
        }

        ElementList Element::GetChildren()
        {
            return vecLocChildren;
        }

        HRESULT Element::Insert(Element* const* _ppChildren, uint_t _cChildren, uint_t _uInsert)
        {
            if (_cChildren == 0)
                return S_OK;

            if (_ppChildren == nullptr)
                return E_INVALIDARG;

            auto _OldChildrenList = GetChildren();
            const auto _cOldChildrenList = _OldChildrenList.GetSize();

            if (_cOldChildrenList < _uInsert)
                return E_INVALIDARG;

            const uint64_t _uNewSize = (uint64_t)_cOldChildrenList + (uint64_t)_cChildren;
            // Element 使用 int32 存储下标，所以 不能超过 int32_max
            if (_uNewSize > int32_max)
                return E_NOTIMPL;

            ElementList _NewChildrenList;
            auto _hr = _NewChildrenList.Reserve((uint_t)_uNewSize);
            if (FAILED(_hr))
                return _hr;

            auto _NewParentValue = Value::CreateElementRef(this);
            if (_NewParentValue == nullptr)
                return E_OUTOFMEMORY;

            HREFTYPE hr = S_OK;

            do
            {
                for (uint_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    if (_pTmp->pLocParent)
                    {
                        _pTmp->pLocParent->Remove(_pTmp);
                    }
                    else if (GetDeferObject(false) != _pTmp->pDeferObject && _pTmp->pDeferObject)
                    {
                        _pTmp->Detach(_pTmp->pDeferObject);
                        _pTmp->pDeferObject->Release();
                        _pTmp->pDeferObject = nullptr;
                    }
                }

                _NewChildrenList.Add(_OldChildrenList.GetData(), _uInsert);

                auto _uLastIndex = _uInsert;

                for (uint_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    _pTmp->_iIndex = (int32_t)_uLastIndex;
                    _NewChildrenList.Add(_pTmp);
                    ++_uLastIndex;
                }

                for (uint_t _uIndex = _uInsert; _uIndex != _cOldChildrenList; ++_uIndex)
                {
                    auto _pTmp = _OldChildrenList[_uIndex];
                    _pTmp->_iIndex = (int32_t)_uLastIndex;
                    _NewChildrenList.Add(_pTmp);
                    ++_uLastIndex;
                }

                if (_NewChildrenList.GetSize() == _OldChildrenList.GetSize())
                    break;

                auto _NewChildrenValue = Value::CreateElementList(_NewChildrenList);
                if (_NewChildrenValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _OldChildrenValue = Value::CreateElementList(_OldChildrenList);
                if (_OldChildrenValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                intptr_t Cooike = 0;

                StartDefer(&Cooike);
                
                // 更新 ChildrenProp
                PreSourceChange(Element::g_ControlInfoData.ChildrenProp, PropertyIndicies::PI_Local, _OldChildrenValue, _NewChildrenValue);
                vecLocChildren.SetArray(std::move(_NewChildrenList));
                PostSourceChange();

                auto _pDeferObject = GetDeferObject();

                for (uint_t _uIndex = 0; _uIndex != _cChildren; ++_uIndex)
                {
                    auto _pTmp = _ppChildren[_uIndex];
                    if (_pTmp == nullptr || _pTmp == this)
                        continue;

                    if (auto p = _pTmp->pDeferObject)
                    {
                        p->Release();
                    }

                    _pTmp->pDeferObject = _pDeferObject;
                    _pDeferObject->AddRef();
                    
                    _pTmp->PreSourceChange(Element::g_ControlInfoData.ParentProp, PropertyIndicies::PI_Local, Value::CreateElementNull(), _NewParentValue);

                    _pTmp->pLocParent = this;
                    _pTmp->pTopLevel = this;

                    _pTmp->PostSourceChange();

                    _pTmp->pDeferObject->Release();
                    _pTmp->pDeferObject = nullptr;
                }
                
                EndDefer(Cooike);

            } while (false);

            if (FAILED(hr))
            {
                for (uint_t _uIndex = _uInsert; _uIndex != _cOldChildrenList; ++_uIndex)
                {
                    _OldChildrenList[_uIndex]->_iIndex = (int32_t)_uIndex;
                }
            }

            return hr;
        }
        
        HRESULT Element::Remove(Element* const* _ppChildren, uint_t _cChildren)
        {
            if (_cChildren == 0)
                return S_OK;

            if (_ppChildren == nullptr)
                return E_INVALIDARG;
            
            auto _ChildrenOld = GetChildren();
            const auto _cChildrenOldSize = _ChildrenOld.GetSize();
            if (_cChildrenOldSize == 0)
                return S_FALSE;
            
            // 入口明明加了int32_max限制，这里为什么任然出现大于 int32_max？？？
            if (_cChildrenOldSize > int32_max)
                return E_UNEXPECTED;

            uint_t _uRemoveCount = 0u;

            for (uint_t _Index = 0u; _Index != _cChildren; ++_Index)
            {
                auto _pItem = _ppChildren[_Index];
                if (_pItem == nullptr)
                    continue;

                if (_pItem->GetParent() != this)
                    continue;

                _pItem->_iIndex = -1;
                ++_uRemoveCount;
            }

            if (_uRemoveCount == 0)
                return S_FALSE;

            HRESULT hr = S_OK;

            do
            {
                ElementList _ChildrenNew;
                auto _uSizeNew = _cChildrenOldSize - _uRemoveCount;
                if (_uSizeNew > int32_max)
                {
                    // 入口明明加了int32_max限制，这里为什么任然出现大于 int32_max？？？
                    hr = E_UNEXPECTED;
                    break;
                }

                if (_uSizeNew)
                {
                    auto _pBuffer = _ChildrenNew.LockBufferAndSetSize(_uSizeNew);
                    if (!_pBuffer)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    for (auto _pItem : _ChildrenOld)
                    {
                        if (_pItem->_iIndex == -1)
                            continue;

                         _pItem->_iIndex = int32_t(_pBuffer - _ChildrenNew.GetData());
                        *_pBuffer = _pItem;
                        ++_pBuffer;
                    }

                    _ChildrenNew.UnlockBuffer();
                }

                auto _ChildrenValueOld = Value::CreateElementList(_ChildrenOld);
                if (_ChildrenValueOld == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _ChildrenValueNew = Value::CreateElementList(_ChildrenNew);
                if (_ChildrenValueNew == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                auto _OldParentValue = Value::CreateElementRef(this);
                if (_OldParentValue == nullptr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                intptr_t _CookiePtr;
                StartDefer(&_CookiePtr);

                PreSourceChange(Element::g_ControlInfoData.ChildrenProp, PropertyIndicies::PI_Local, _ChildrenValueOld, _ChildrenValueNew);
                vecLocChildren.SetArray(std::move(_ChildrenNew));
                PostSourceChange();


                auto _pDeferObject = GetDeferObject();

                for (uint_t _Index = 0u; _Index != _cChildren; ++_Index)
                {
                    auto _pItem = _ppChildren[_Index];
                    if (_pItem == nullptr)
                        continue;

                    if (_pItem->GetParent() != this)
                        continue;

                    if (_pItem->pDeferObject == nullptr)
                    {
                        _pItem->pDeferObject = _pDeferObject;
                        _pDeferObject->AddRef();
                    }
                    
                    _pItem->PreSourceChange(Element::g_ControlInfoData.ParentProp, PropertyIndicies::PI_Local, _OldParentValue, Value::CreateElementNull());
                    _pItem->pLocParent = nullptr;
                    _pItem->pTopLevel = nullptr;
                    PostSourceChange();
                }

                EndDefer(_CookiePtr);
            } while (false);

            if (FAILED(hr))
            {
                // 回滚操作
                for (uint_t _uIndex = 0; _uIndex != _cChildrenOldSize; ++_uIndex)
                {
                    // _cChildrenOldSize 保证 <= int32max
                    _ChildrenOld[_uIndex]->_iIndex = (int32_t)_uIndex;
                }
            }

            return hr;
        }

        int32_t Element::SpecCacheIsEqual(Element* _pElement1, Element* _pElement2, const PropertyInfo& _Prop)
        {
            FastSpecValueCompareHandleData _HandleData;
            _HandleData.pProp = &_Prop;
            _HandleData.eIndicies = PropertyIndicies::PI_Specified;
            _HandleData.pOther = _pElement2;

            if (!_pElement1->GeneralHandle(&_HandleData))
                return -1;

            return _HandleData.Output.iResult;
        }

        bool Element::SetKeyboardFocus()
        {
            if (!pWindow)
                return false;

            if (GetVisible() == false || GetEnabled() == false || (GetActive() & Active::Keyboard) == 0)
                return false;

            auto _hr = SetValueInternal(Element::g_ControlInfoData.KeyboardFocusedProp, Value::CreateBoolTrue(), false);
            return SUCCEEDED(_hr);
        }
        
        bool Element::SetFocus()
        {
            if (GetVisible() == false || GetEnabled() == false || (GetActive() & Active::ActiveMarks) == 0)
                return false;

            auto _hr = SetValueInternal(Element::g_ControlInfoData.FocusedProp, Value::CreateBoolTrue(), false);
            return SUCCEEDED(_hr);
        }

        Element* Element::GetImmediateChild(Element* _pFrom)
        {
            if (!_pFrom)
                return nullptr;

            for (Element* _pParent = _pFrom->GetParent(); _pParent != this; _pParent = _pParent->GetParent())
            {
                if (!_pParent)
                    return nullptr;

                _pFrom = _pParent;
            }

            return _pFrom;
        }

        HRESULT Element::PreSourceChange(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _pNewValue)
        {
            if (_pOldValue == nullptr || _pNewValue == nullptr)
                return E_INVALIDARG;

            auto _pDeferObject = GetDeferObject();

            if (!_pDeferObject)
                return 0x800403EA;

            ++_pDeferObject->uPropertyChangeEnter;

            PCRecord* _pPCRecord = _pDeferObject->vecPropertyChanged.EmplacePtr();
            if (!_pPCRecord)
                return E_OUTOFMEMORY;

            auto TempInt = _pDeferObject->uPropertyChangedPostSourceCount;

            _pDeferObject->AddRef();
            _pPCRecord->iRefCount = 1;
            _pPCRecord->vC = -1;
            _pPCRecord->pElement = this;
            _pPCRecord->pProp = &_Prop;
            _pPCRecord->iIndex = _eIndicies;
            _pPCRecord->iPrevElRec = _pPCRecord->pElement->_iPCTail;
            _pPCRecord->pElement->_iPCTail = TempInt;

            _pPCRecord->pOldValue = _pOldValue;

            if (_eIndicies == PropertyIndicies::PI_Local)
            {
                _pPCRecord->pNewValue = _pNewValue;
            }
            else
            {
                _pPCRecord->pNewValue = nullptr;
            }

            auto bFaild = false;

            DepRecs DepRecs;

            for (auto i = _pDeferObject->uPropertyChangedPostSourceCount; i < _pDeferObject->vecPropertyChanged.GetSize(); ++i)
            {
                auto pc = _pDeferObject->vecPropertyChanged.GetItemPtr(i);

                if (FAILED(pc->pElement->GetDependencies(*pc->pProp, pc->iIndex, &DepRecs, TempInt, _pNewValue, _pDeferObject)))
                    bFaild = true;

                pc = &_pDeferObject->vecPropertyChanged[i];

                pc->dr = DepRecs;
            }

            for (auto i = _pDeferObject->uPropertyChangedPostSourceCount + 1; i < _pDeferObject->vecPropertyChanged.GetSize(); ++i)
            {
                if (i > int32_max)
                {
                    throw Exception(_S("PreSourceChange期间，uPropertyChangedPostSourceCount数值超过 int32_max。"));
                }
                auto pc = &_pDeferObject->vecPropertyChanged[i];

                int32_t _iScanIndex = i;
                if (pc->iRefCount && pc->vC == -1 && pc->pOldValue == nullptr)
                {
                    auto p2a = -1;

                    auto pValue = pc->pElement->GetValue(*pc->pProp, pc->iIndex, false);

                    auto j = pc->pElement->_iPCTail;

                    PCRecord* pctmp = nullptr;

                    do
                    {
                        auto pcj = &_pDeferObject->vecPropertyChanged[j];

                        if (pcj->iIndex == pc->iIndex && pcj->pProp == pc->pProp)
                        {
                            if (p2a == -1)
                            {
                                pctmp = pcj;
                                p2a = j;
                                pcj->pOldValue = pValue;
                            }
                            else
                            {
                                pcj->vC = p2a;
                                ++pctmp->iRefCount;

                                for (auto i = 0; i < pcj->dr.cDepCnt; ++i)
                                {
                                    VoidPCNotifyTree(i + pcj->dr.iDepPos, _pDeferObject);
                                }

                                pcj->dr.cDepCnt = 0;
                            }
                        }

                        j = pcj->iPrevElRec;

                    } while (j >= _iScanIndex);
                }
            }

            return bFaild != 0 ? 0x800403EB : S_OK;
        }
        
        HRESULT Element::PostSourceChange()
        {
            auto pDeferObject = GetDeferObject();
            if (!pDeferObject)
                return 0x800403EA;

            bool bSuccess = false;
            intptr_t Cookie;

            StartDefer(&Cookie);

            for (; pDeferObject->uPropertyChangedPostSourceCount < pDeferObject->vecPropertyChanged.GetSize(); ++pDeferObject->uPropertyChangedPostSourceCount)
            {
                auto pc = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedPostSourceCount);

                if (pc->iRefCount)
                {
                    if ((pc->vC & 0x80000000) == 0)
                    {
                        pc->iRefCount = 0;

                        pDeferObject->Release();
                    }
                    else
                    {
                        if (pc->pNewValue == nullptr)
                            pc->pNewValue = pc->pElement->GetValue(*pc->pProp, pc->iIndex, true);

                        if (pc->pOldValue == pc->pNewValue)
                        {
                            pc->iRefCount = 1;

                            VoidPCNotifyTree(pDeferObject->uPropertyChangedPostSourceCount, pDeferObject);
                        }
                        /*else if (!pc->pProp->v18->_iGlobalIndex)
                        {
                            if (auto pElement = pc->pNewValue->GetElement())
                            {
                                auto pTop = pElement->GetTopLevel();

                                auto _pControlInfo = pTop->GetClassInfoW();

                                if (_pControlInfo->IsSubclassOf(HWNDElement::GetClassInfoPtr()))
                                    pc->pe->_InheritProperties();
                            }
                        }*/
                    }
                }
            }

            if (pDeferObject->uPropertyChangeEnter == 1)
            {
                for (; pDeferObject->uPropertyChangedFireCount < pDeferObject->vecPropertyChanged.GetSize(); ++pDeferObject->uPropertyChangedFireCount)
                {
                    // 上面已经边界检查了
                    #pragma warning(suppress : 6011)
                    auto pPCRecord = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedFireCount);

                    if (pPCRecord->iRefCount)
                    {
                        if ((pPCRecord->pProp->fFlags & PropertyFlag::PF_TypeBits) == PropertyIndiciesMapToPropertyFlag(pPCRecord->iIndex))
                        {
                            bSuccess = SetGroupChanges(pPCRecord->pElement, pPCRecord->pProp->fGroups, pDeferObject) == 0;
                        }

                        //pPCRecord->pElement->HandleUiaPropertyListener(pPCRecord->pProp, pPCRecord->iIndex, pPCRecord->pOldValue, pPCRecord->pNewValue);

                        pPCRecord->pElement->OnPropertyChanged(*pPCRecord->pProp, pPCRecord->iIndex, pPCRecord->pOldValue, pPCRecord->pNewValue);
                        
                        pPCRecord = pDeferObject->vecPropertyChanged.GetItemPtr(pDeferObject->uPropertyChangedFireCount);

                        #pragma warning(suppress : 6011)
                        pPCRecord->pOldValue = nullptr;

                        pPCRecord->pNewValue = nullptr;

                        pPCRecord->iRefCount = 0;

                        pDeferObject->Release();
                    }

                    if (pPCRecord->pElement && pDeferObject->uPropertyChangedFireCount == pPCRecord->pElement->_iPCTail)
                    {
                        pPCRecord->pElement->_iPCTail = -1;
                    }
                }

                pDeferObject->uPropertyChangedFireCount = 0;
                pDeferObject->uPropertyChangedPostSourceCount = 0;

                pDeferObject->vecPropertyChanged.Clear();
            }

            //LABEL_15

            --pDeferObject->uPropertyChangeEnter;
            EndDefer(Cookie);

            return bSuccess != 0 ? 0x800403EB : 0;
        }

        HRESULT Element::GetDependencies(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, int iPCSrcRoot, const Value& _pNewValue, DeferCycle* _pDeferCycle)
        {
            pdr->iDepPos = -1;
            pdr->cDepCnt = 0;

            HRESULT _hrLast = S_OK;

            if ((_Prop.fFlags & PF_TypeBits) == PropertyIndiciesMapToPropertyFlag(_eIndicies))
            {
                if (auto p = pSheet)
                {
                    auto _hr = p->GetSheetDependencies(this, &_Prop, pdr, _pDeferCycle);
                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
            }

            if (_Prop.pfnCustomPropertyHandle)
            {
                GetDependenciesHandleData _HandleData;
                _HandleData.pProp = &_Prop;
                _HandleData.eIndicies = _eIndicies;
                _HandleData.pdr = pdr;
                _HandleData.iPCSrcRoot = iPCSrcRoot;
                _HandleData.NewValue = _pNewValue;
                _HandleData.pDeferCycle = _pDeferCycle;

                if ((this->*_Prop.pfnCustomPropertyHandle)(_HandleData.HandleType, &_HandleData))
                {
                    if (FAILED(_HandleData.Output.hr))
                        _hrLast = _HandleData.Output.hr;
                }
            }
            
            if (_Prop.ppDependencies)
            {
                for (auto _ppDependencies = _Prop.ppDependencies; *_ppDependencies; ++_ppDependencies)
                {
                    auto& _DependencyProp = **_ppDependencies;
                    auto _uDependencyIndicies = (uint32_t)_eIndicies;

                    for (; _uDependencyIndicies; --_uDependencyIndicies)
                    {
                        if (_DependencyProp.fFlags & (1u << _uDependencyIndicies))
                        {
                            auto _hr = AddDependency(this, _DependencyProp, (PropertyIndicies)_uDependencyIndicies, pdr, _pDeferCycle);
                            if (FAILED(_hr))
                                _hrLast = _hr;
                            break;
                        }
                    }                    
                }
            }

            switch (_eIndicies)
            {
            case PropertyIndicies::PI_Local:
                if (_Prop.fFlags & PropertyFlag::PF_HasSpecified)
                {
                    auto _hr = AddDependency(this, _Prop, PropertyIndicies::PI_Specified, pdr, _pDeferCycle);
                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
                break;
            case PropertyIndicies::PI_Specified:
                if (_Prop.fFlags & PropertyFlag::PF_Inherit)
                {
                    for (auto _pChild : vecLocChildren)
                    {
                        auto _iPropIndex = _pChild->GetControlInfo()->GetPropertyInfoIndex(_Prop);
                        if (_iPropIndex < 0)
                            continue;
                        
                        auto ppValue = _pChild->LocalPropValue.GetItemPtr(_iPropIndex);
                        if (ppValue && *ppValue != nullptr)
                            continue;

                        auto _hr = AddDependency(_pChild, _Prop, PropertyIndicies::PI_Specified, pdr, _pDeferCycle);
                        if (FAILED(_hr))
                            _hrLast = _hr;
                    }
                }
                
                if (_Prop.fFlags & PropertyFlag::PF_HasComputed)
                {
                    auto _hr = AddDependency(this, _Prop, PropertyIndicies::PI_Computed, pdr, _pDeferCycle);
                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
                break;
            case PropertyIndicies::PI_Computed:
                break;
            }
            return _hrLast;
        }

        HRESULT Element::AddDependency(Element* _pElement, const PropertyInfo& _Prop, PropertyIndicies _eIndicies, DepRecs* pdr, DeferCycle* _pDeferCycle)
        {
            PCRecord* pItem = _pDeferCycle->vecPropertyChanged.EmplacePtr();
            if (!pItem)
            {
                return E_OUTOFMEMORY;
            }
            auto _uIndex = _pDeferCycle->vecPropertyChanged.GetItemIndex(pItem);
            if (_uIndex > int32_max)
                throw Exception();

            auto _iIndex = (int32_t)_uIndex;

            _pDeferCycle->AddRef();

            pItem->iRefCount = 1;
            pItem->vC = -1;
            pItem->pElement = _pElement;
            pItem->pProp = &_Prop;
            pItem->iIndex = _eIndicies;
            pItem->pOldValue = nullptr;
            pItem->pNewValue = nullptr;
            pItem->iPrevElRec = _pElement->_iPCTail;
            _pElement->_iPCTail = _iIndex;

            if (!pdr->cDepCnt)
            {
                pdr->iDepPos = _iIndex;
            }

            ++(pdr->cDepCnt);

            return S_OK;
        }

        HRESULT Element::GetBuriedSheetDependencies(const PropertyInfo* _pProp, Element* _pElement, DepRecs* _pDR, DeferCycle* _pDeferCycle)
        {
            HRESULT _hrLast = S_OK;

            if (auto p = pSheet)
            {
                auto _hr = p->GetSheetDependencies(this, _pProp, _pDR, _pDeferCycle);
                if (FAILED(_hr))
                    _hrLast = _hr;
            }

            auto _Children = GetChildren();
            for (auto pItem : _Children)
            {
                if (Element::SpecCacheIsEqual(pItem, _pElement, *_pProp) == 1)
                    continue;

                {
                    auto _LocalValue = pItem->GetValue(*_pProp, PropertyIndicies::PI_Local, false);
                    if (_LocalValue == nullptr)
                        continue;
                }

                auto _hr = pItem->GetBuriedSheetDependencies(_pProp, _pElement, _pDR, _pDeferCycle);
                if (FAILED(_hr))
                    _hrLast = _hr;
            }

            return _hrLast;
        }

		void Element::VoidPCNotifyTree(int p1, DeferCycle* p2)
        {
            auto pr = p2->vecPropertyChanged.GetItemPtr(p1);
            if (!pr)
                return;
            if (pr->iRefCount)
            {
                if (pr->vC >= 0)
                {
                    VoidPCNotifyTree(pr->vC, p2);
                    pr->iRefCount = 0;
                }
                else
                {
                    if (--pr->iRefCount)
                        return;
                    
                    pr->pOldValue = nullptr;
                    pr->pNewValue = nullptr;

                    for (int i = 0; i != pr->dr.cDepCnt; ++i)
                        VoidPCNotifyTree(pr->dr.iDepPos + i, p2);
                }

                p2->Release();
            }
        }

        bool Element::MarkElementForLayout(Element* _pElement, uint32_t _fNeedsLayoutNew)
        {
            if (_pElement && _pElement->SetNeedsLayout(_fNeedsLayoutNew))
            {
                for (;;)
                {
                    if (_pElement->pLocParent == nullptr || _pElement->iSpecLayoutPos == LP_Absolute)
                    {
                        return true;
                    }

                    _pElement = _pElement->pLocParent;

                    if (!_pElement->SetNeedsLayout(LC_Unknow1))
                    {
                        break;
                    }
                }
            }

            return false;
        }

        bool Element::MarkElementForDesiredSize(Element* _pElement)
        {
            for (; _pElement; _pElement = _pElement->pLocParent)
            {
                if (_pElement->bNeedsDSUpdate)
                    break;

                _pElement->bNeedsDSUpdate = true;

                if (_pElement->pLocParent == nullptr || _pElement->iSpecLayoutPos == LP_Absolute)
                    return 1;
            }

            return 0;
        }

        bool Element::SetGroupChanges(Element* pElement, uint32_t _fGroups, DeferCycle* pDeferCycle)
        {
            if (pElement->fNeedsLayout == LC_Optimize)
            {
                _fGroups &= ~PG_AffectsLayout;
            }

            bool bResult = true;

            if (_fGroups & PG_NormalPriMask)
            {
                TransferGroupFlags(pElement, _fGroups);

                GCRecord* pItem = nullptr;

                if (pElement->_iGCSlot == -1)
                {
                    pItem = pDeferCycle->vecGroupChangeNormalPriority.EmplacePtr();
                    if (!pItem)
                    {
                        bResult = false;
                    }
                    else
                    {
                        uint_t uAddIndex = pDeferCycle->vecGroupChangeNormalPriority.GetItemIndex(pItem);
                        if (uAddIndex > int32_max)
                            throw Exception();

                        pDeferCycle->AddRef();
                        pItem->pElement = pElement;
                        pItem->fGroups = 0;

                        pElement->_iGCSlot = (int32_t)uAddIndex;
                    }
                }
                else
                {
                    pItem = pDeferCycle->vecGroupChangeNormalPriority.GetItemPtr(pElement->_iGCSlot);
                }

                if (pItem)
                    pItem->fGroups |= _fGroups;
            }

            if (_fGroups & PG_LowPriMask)
            {
                GCRecord* pItem = nullptr;

                if (pElement->_iGCLPSlot == -1)
                {
                    pItem = pDeferCycle->vecGroupChangeLowPriority.EmplacePtr();

                    if (!pItem)
                    {
                        bResult = false;
                    }
                    else
                    {
                        auto uAddIndex = pDeferCycle->vecGroupChangeLowPriority.GetItemIndex(pItem);
                        if (uAddIndex > int32_max)
                            throw Exception();

                        pDeferCycle->AddRef();
                        pItem->pElement = pElement;
                        pItem->fGroups = 0;

                        pElement->_iGCLPSlot = (int32_t)uAddIndex;
                    }
                }
                else
                {
                    pItem = pDeferCycle->vecGroupChangeLowPriority.GetItemPtr(pElement->_iGCLPSlot);
                }

                if (pItem)
                    pItem->fGroups |= _fGroups;
            }

            return bResult;
        }
        
        bool Element::SetNeedsLayout(uint32_t _fNeedsLayoutNew)
        {
            if (_fNeedsLayoutNew > fNeedsLayout)
            {
                fNeedsLayout = _fNeedsLayoutNew;
                return true;
            }
            return false;
        }

        void Element::TransferGroupFlags(Element* pElement, uint32_t _fGroups)
        {
            if (_fGroups & PG_AffectsLayout)
                MarkElementForLayout(pElement, LC_Normal);

            if (_fGroups & PG_AffectsDesiredSize)
                MarkElementForDesiredSize(pElement);

            if (_fGroups & PG_AffectsParentLayout)
                MarkElementForLayout(pElement->iSpecLayoutPos != LP_Absolute ? pElement->pLocParent : pElement, LC_Normal);

            if (_fGroups & PG_AffectsParentDesiredSize)
                MarkElementForDesiredSize(pElement->iSpecLayoutPos != LP_Absolute ? pElement->pLocParent : pElement);
        }

        bool Element::GeneralHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
		{
            auto _pProp = _pHandleData->pProp;
            if (_pProp->pfnCustomPropertyHandle)
            {
                if ((this->*_pProp->pfnCustomPropertyHandle)(_eType, _pHandleData))
                    return true;
            }


            uint16_t _uOffsetToCache = 0;
            uint16_t _uOffsetToHasCache = 0;
            uint8_t _uCacheBit;
            uint8_t _uHasCacheBit;

			switch (_pHandleData->eIndicies)
			{
			case PropertyIndicies::PI_Computed:
			case PropertyIndicies::PI_Specified:
                if (_pProp->BindCacheInfo.OffsetToSpecifiedValue)
				{
                    _uOffsetToCache = _pProp->BindCacheInfo.OffsetToSpecifiedValue;
                    _uCacheBit = _pProp->BindCacheInfo.SpecifiedValueBit;
                    _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasSpecifiedValueCache;
                    _uHasCacheBit = _pProp->BindCacheInfo.HasSpecifiedValueCacheBit;
					break;
				}
			case PropertyIndicies::PI_Local:
                _uOffsetToCache = _pProp->BindCacheInfo.OffsetToLocalValue;
                _uCacheBit = _pProp->BindCacheInfo.LocalValueBit;
                _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasLocalCache;
                _uHasCacheBit = _pProp->BindCacheInfo.HasLocalValueCacheBit;
				break;
			}

			if (_eType == CustomPropertyHandleType::GetValue)
			{
                if (_uOffsetToCache == 0)
                    return false;

                auto _pInfo = (GetValueHandleData*)_pHandleData;
                Value _pRetValue;

				do
				{
					// 如果属性是 PF_ReadOnly，那么它必然不会实际走到 _LocalPropValue 里面去，必须走 缓存
					// 如果 _UsingCache == true，那么我们可以走缓存
                    if ((_pProp->fFlags & PF_ReadOnly) || _pInfo->bUsingCache)
					{
						// 检测实际是否存在缓存，如果检测到没有缓存，那么直接返回
                        if (_uOffsetToHasCache)
						{
                            const auto _uHasValue = *((uint8_t*)this + _uOffsetToHasCache);
                            if ((_uHasValue & (1 << _uHasCacheBit)) == 0)
							{
                                _pRetValue = Value::CreateUnset();
								break;
							}
						}

						auto _pCache = (uint8_t*)this + _uOffsetToCache;

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            _pRetValue = Value::CreateInt32(*(int32_t*)_pCache);
							break;
                        case ValueType::float_t:
                            _pRetValue = Value::CreateFloat(*(float*)_pCache);
                            break;
						case ValueType::boolean:
                            _pRetValue = Value::CreateBool((*(uint8_t*)_pCache) & (1 << _uCacheBit));
							break;
                        case ValueType::Color:
                            _pRetValue = Value::CreateColor(*(Color*)_pCache);
                            break;
                        case ValueType::Point:
                            _pRetValue = Value::CreatePoint(*(Point*)_pCache);
                            break;
                        case ValueType::Size:
                            _pRetValue = Value::CreateSize(*(Size*)_pCache);
                            break;
                        case ValueType::Rect:
                            _pRetValue = Value::CreateRect(*(Rect*)_pCache);
                            break;
                        case ValueType::Element:
                            _pRetValue = Value::CreateElementRef(*(Element**)_pCache);
                            break;
                        case ValueType::ElementList:
                            _pRetValue = Value::CreateElementList(*(ElementList*)_pCache);
                            break;
                        case ValueType::StyleSheet:
                            _pRetValue = Value::CreateStyleSheet(*(StyleSheet**)_pCache);
                            break;
                        case ValueType::uString:
                            _pRetValue = Value::CreateString(*(uString*)_pCache);
                            break;
						default:
                            __debugbreak();
                            _pRetValue = Value::CreateNull();
							break;
						}

                        if (_pRetValue == nullptr)
                            _pRetValue = Value::CreateUnset();
					}
				} while (false);
				
				_pInfo->Output.RetValue = std::move(_pRetValue);

				if (_pProp->fFlags & PF_ReadOnly)
				{
                    _pInfo->Output.CacheResult = PropertyCustomCacheResult::SkipLocalPropValue;
				}
                else
                {
                    _pInfo->Output.CacheResult = PropertyCustomCacheResult::SkipNone;
                }
                return true;
			}
            else if (_eType == CustomPropertyHandleType::SetValue)
			{
                if (_uOffsetToCache == 0)
                    return false;

                auto _pInfo = (SetValueHandleData*)_pHandleData;

				do
				{
					auto& _pNewValue = _pInfo->InputNewValue;

					if (_pNewValue == nullptr)
						break;

                    auto _pCache = (uint8_t*)this + _uOffsetToCache;

					if (_pNewValue.GetType() == ValueType::Unset)
					{
                        if (_uOffsetToHasCache == 0)
                        {
                            // 不应该设置 Unset
                            __debugbreak();
                            break;
                        }

						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);

						_uHasCache &= ~(1 << _uHasCacheBit);

                        // 某些资源我们需要进行释放
                        switch ((ValueType)_pProp->BindCacheInfo.eType)
                        {
                        case ValueType::Element:
                            *(Element**)_pCache = nullptr;
                            break;
                        case ValueType::ElementList:
                            ((ElementList*)_pCache)->Clear();
                            break;
                        case ValueType::StyleSheet:
                        {
                            auto& _pOldStyleSheet = *(StyleSheet**)_pCache;
                            if (_pOldStyleSheet)
                            {
                                _pOldStyleSheet->Release();
                                _pOldStyleSheet = nullptr;
                            }
                            break;
                        }
                        case ValueType::uString:
                        {
                            auto& _szOldString = *(uString*)_pCache;
                            _szOldString.Clear();
                            break;
                        }
                        }
					}
                    else if (_pNewValue.GetType() == (ValueType)_pProp->BindCacheInfo.eType)
					{
						// 标记缓存已经被设置
                        if (_uOffsetToHasCache)
                        {
                            auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);
                            _uHasCache |= (1 << _uHasCacheBit);
                        }

                        auto _pDataBuffer = _pNewValue.GetRawBuffer();

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            *(int32_t*)_pCache = *(int32_t*)_pDataBuffer;
							break;
                        case ValueType::float_t:
                            *(float*)_pCache = *(float*)_pDataBuffer;
                            break;
						case ValueType::boolean:
                            if (*(bool*)_pDataBuffer)
							{
                                *_pCache |= (1 << _uCacheBit);
							}
							else
							{
                                *_pCache &= ~(1 << _uCacheBit);
							}
							break;
                        case ValueType::Color:
                            *(Color*)_pCache = *(Color*)_pDataBuffer;
                            break;
                        case ValueType::Point:
                            *(Point*)_pCache = *(Point*)_pDataBuffer;
                            break;
                        case ValueType::Size:
                            *(Size*)_pCache = *(Size*)_pDataBuffer;
                            break;
                        case ValueType::Rect:
                            *(Rect*)_pCache = *(Rect*)_pDataBuffer;
                            break;
                        case ValueType::Element:
                            *(Element**)_pCache = *(Element**)_pDataBuffer;
                            break;
                        case ValueType::ElementList:
                            ((ElementList*)_pCache)->SetArray(*(ElementList*)_pDataBuffer);
                            break;
                        case ValueType::StyleSheet:
                        {
                            auto& _pOldStyleSheet = *(StyleSheet**)_pCache;
                            auto _pNewStyleSheet = *(StyleSheet**)_pDataBuffer;

                            if (_pOldStyleSheet != _pNewStyleSheet)
                            {
                                if (_pOldStyleSheet)
                                    _pOldStyleSheet->Release();
                                if (_pNewStyleSheet)
                                    _pNewStyleSheet->AddRef();
                                _pOldStyleSheet = _pNewStyleSheet;
                            }
                            break;
                        }
                        case ValueType::uString:
                        {
                            auto& _szOldString = *(uString*)_pCache;
                            auto& _szNewString = *(uString*)_pDataBuffer;
                            if (_szOldString != _szNewString)
                            {
                                _szOldString = _szNewString;
                            }
                            break;
                        }
						default:
                            __debugbreak();
							break;
						}
					}

				} while (false);

                return true;
			}
            else if (_eType == CustomPropertyHandleType::FastSpecValueCompare)
            {
                if (!_pProp->BindCacheInfo.OffsetToSpecifiedValue)
                    return false;
                
                auto _pInfo = (FastSpecValueCompareHandleData*)_pHandleData;

                if (_uOffsetToHasCache)
                {
                    auto _uHasCacheBit = _pProp->BindCacheInfo.HasSpecifiedValueCacheBit;

                    const auto _bHasValue1 = ((*((uint8_t*)this + _uOffsetToHasCache)) & (1 << _uHasCacheBit)) != 0;
                    if (!_bHasValue1)
                        return false;

                    const auto _bHasValue2 = ((*((uint8_t*)_pInfo->pOther + _uOffsetToHasCache)) & (1 << _uHasCacheBit)) != 0;
                    if (!_bHasValue2)
                        return false;
                }

                auto _pRawBuffer1 = (uint8_t*)this + _uOffsetToCache;
                auto _pRawBuffer2 = (uint8_t*)_pInfo->pOther + _uOffsetToCache;

                switch (ValueType(_pProp->BindCacheInfo.eType))
                {
                case ValueType::int32_t:
                    _pInfo->Output.iResult = *(int32_t*)_pRawBuffer1 == *(int32_t*)_pRawBuffer2;
                    break;
                case ValueType::float_t:
                    _pInfo->Output.iResult = *(float*)_pRawBuffer1 == *(float*)_pRawBuffer2;
                    break;
                case ValueType::boolean:
                {
                    auto _bValue1 = ((*(uint8_t*)_pRawBuffer1) & (1 << _pProp->BindCacheInfo.SpecifiedValueBit)) != 0;
                    auto _bValue2 = ((*(uint8_t*)_pRawBuffer2) & (1 << _pProp->BindCacheInfo.SpecifiedValueBit)) != 0;

                    _pInfo->Output.iResult = _bValue1 == _bValue2;
                    break;
                }
                case ValueType::Color:
                    _pInfo->Output.iResult = (*(Color*)_pRawBuffer1) == (*(Color*)_pRawBuffer2);
                    break;
                case ValueType::Point:
                    _pInfo->Output.iResult = (*(Point*)_pRawBuffer1) == (*(Point*)_pRawBuffer2);
                    break;
                case ValueType::Size:
                    _pInfo->Output.iResult = (*(Size*)_pRawBuffer1) == (*(Size*)_pRawBuffer2);
                    break;
                case ValueType::Rect:
                    _pInfo->Output.iResult = *(Rect*)_pRawBuffer1 == *(Rect*)_pRawBuffer2;
                    break;
                default:
                    return false;
                    break;
                }
                return true;
            }

			return false;
		}
		
		bool Element::OnParentPropChanged(OnPropertyChangedHandleData* _pHandleData)
		{
            if (_pHandleData->eIndicies != PropertyIndicies::PI_Local)
                return false;

            auto _pOldParent = _pHandleData->OldValue.GetElement();
            auto _pNewParent = _pHandleData->NewValue.GetElement();

            if (_pOldParent && _pOldParent->pSheet)
            {
            
            }

            auto _pOldWindow = _pOldParent ? _pOldParent->pWindow : nullptr;
            auto _pNewWindow = _pNewParent ? _pNewParent->pWindow : nullptr;

            if (_pOldWindow == _pNewWindow)
                return true;

            if (_pOldWindow)
                OnUnHosted(_pOldWindow);

            if (_pNewWindow)
                OnHosted(_pNewWindow);

            return true;
        }

        bool Element::VisiblePropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                return OnVisiblePropChanged((OnPropertyChangedHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetDependencies:
                return GetVisibleDependencies((GetDependenciesHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetValue:
                return GetVisiblePropertyValue((GetValueHandleData*)_pHandleData);
            case CustomPropertyHandleType::SetValue:
                return SetVisiblePropertyValue((SetValueHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::OnVisiblePropChanged(OnPropertyChangedHandleData* _pHandleData)
        {
            if (_pHandleData->eIndicies == PropertyIndicies::PI_Computed)
            {
                // 实际计算值改变了才刷新显示
                Invalidate();
                return true;
            }

            return false;
        }

        bool Element::GetVisiblePropertyValue(GetValueHandleData* _pHandleData)
        {
            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                // 使用通用逻辑
                return false;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                if (_pHandleData->bUsingCache)
                {
                    _pHandleData->Output.RetValue = Value::CreateBool(bSpecVisible);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                }
                else
                {
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipNone;
                }
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Computed)
            {
                if (!_pHandleData->bUsingCache)
                {
                    auto _bCmpVisibleTmp = true;

                    for (auto _pElement = this; _pElement; _pElement = _pElement->GetParent())
                    {
                        if (!_pElement->bSpecVisible)
                        {
                            _bCmpVisibleTmp = false;
                            break;
                        }
                    }

                    _pHandleData->Output.RetValue = Value::CreateBool(_bCmpVisibleTmp);
                }
                else
                {
                    _pHandleData->Output.RetValue = Value::CreateBool(bCmpVisible);
                }

                _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                return true;
            }

            return false;
        }

        bool Element::SetVisiblePropertyValue(SetValueHandleData* _pHandleData)
        {
            if (_pHandleData->InputNewValue.GetType() == ValueType::boolean)
            {
                if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
                {
                    bSpecVisible = _pHandleData->InputNewValue.GetBool();
                    return true;
                }
                else if (_pHandleData->eIndicies == PropertyIndicies::PI_Computed)
                {
                    bCmpVisible = _pHandleData->InputNewValue.GetBool();
                    return true;
                }
            }

            return false;
        }

        bool Element::ParentPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                return OnParentPropChanged((OnPropertyChangedHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetDependencies:
                return GetParentDependencies((GetDependenciesHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetValue:
                break;
            case CustomPropertyHandleType::SetValue:
                break;
            default:
                break;
            }
            return false;
        }

        bool Element::EnabledPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                return OnEnabledPropChanged((OnPropertyChangedHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetDependencies:
            case CustomPropertyHandleType::GetValue:
            case CustomPropertyHandleType::SetValue:
                break;
            default:
                break;
            }
            return false;
        }

        bool Element::OnEnabledPropChanged(OnPropertyChangedHandleData* _pHandle)
        {
            // todo 如果被禁用，那么鼠标是不行了。
            if (_pHandle->NewValue.GetBool() == false)
            {
            }

            return false;
        }

        bool Element::ActivePropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                return OnActivePropChanged((OnPropertyChangedHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetDependencies:
            case CustomPropertyHandleType::GetValue:
            case CustomPropertyHandleType::SetValue:
                break;
            default:
                break;
            }
            return false;
        }

        bool Element::OnActivePropChanged(OnPropertyChangedHandleData* _pHandleData)
        {
            // todo，
            return false;
        }
        
        bool Element::DpiPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                return OnDpiPropChanged((OnPropertyChangedHandleData*)_pHandleData);
            case CustomPropertyHandleType::GetDependencies:
            case CustomPropertyHandleType::GetValue:
            case CustomPropertyHandleType::SetValue:
                break;
            default:
                break;
            }
            return false;
        }

        bool Element::OnDpiPropChanged(OnPropertyChangedHandleData* _pHandleData)
        {
            auto _pControlInfo = GetControlInfo();

            const PropertyInfo* _pProp;
            for (uint32_t _uIndex = 0u; _pProp = _pControlInfo->EnumPropertyInfo(_uIndex); ++_uIndex)
            {
                if ((_pProp->fFlags & PF_UpdateDpi) == 0)
                    continue;

                if (pWindow && GetParent() == nullptr)
                {
                    if (_pProp == &Element::g_ControlInfoData.WidthProp || _pProp == &Element::g_ControlInfoData.HeightProp)
                        continue;
                }

                auto _Value = GetValue(*_pProp, PropertyIndicies::PI_Local);
                if (!_Value.HasValue())
                    continue;

                auto _NewValue = _Value.UpdateDpi(GetDpi());
                if (_NewValue == nullptr || _NewValue.IsSame(_Value))
                    continue;

                SetValue(*_pProp, _NewValue);
            }

            return true;
        }

        void Element::FlushDesiredSize(DeferCycle* _pDeferCycle)
        {
            if ((pLocParent == nullptr || iSpecLayoutPos == LP_Absolute))
            {
                Size _ConstraintSize = { GetWidth(), GetHeight() };

                if (_ConstraintSize.Width == -1)
                    _ConstraintSize.Width = int16_max;

                if (_ConstraintSize.Height == -1)
                    _ConstraintSize.Height = int16_max;

                UpdateDesiredSize(_ConstraintSize);

                /*auto hDC = GetDC(nullptr);

                {
                    DCSurface DCS(hDC);

                    if (y == -1)
                        y = 0x7FFF;

                    if (x == -1)
                        x = 0x7FFF;

                    UpdateDesiredSize(x, y, &DCS);
                }

                ReleaseDC(nullptr, hDC);*/
            }
        }

        void Element::FlushLayout(DeferCycle* _pDeferCycle)
        {
            auto _fNeedsLayout = fNeedsLayout;

            if (fNeedsLayout == LC_Pass)
                return;

            fNeedsLayout = LC_Pass;

            if (_fNeedsLayout == LC_Normal)
            {
                auto _Layout = GetLayout();
                if ((_Layout.HasValue() && _Layout.GetValue()) || bSelfLayout)
                {
                    auto Extent = GetExtent();

                    Extent.Width -= SpecBorderThickness.Left + SpecBorderThickness.Right;
                    Extent.Height -= SpecBorderThickness.Top + SpecBorderThickness.Bottom;

                    Extent.Width -= SpecPadding.Left + SpecPadding.Right;
                    Extent.Height -= SpecPadding.Top + SpecPadding.Bottom;

                    if (Extent.Width < 0)
                        Extent.Width = 0;

                    if (Extent.Height < 0)
                        Extent.Height = 0;

                    if (bSelfLayout)
                        SelfLayoutDoLayout(Extent);
                    else
                        _Layout.GetValue()->DoLayout(this, Extent);
                }

                for (auto pChildren : GetChildren())
                {
                    auto _iLayoutPos = pChildren->GetLayoutPos();
                    if (_iLayoutPos == LP_None)
                    {
                        pChildren->UpdateLayoutPosition(Point{ 0, 0 });
                        pChildren->UpdateLayoutSize(Size {0, 0});
                    }
                    else if (_iLayoutPos != LP_Absolute)
                    {
                        pChildren->FlushLayout(_pDeferCycle);
                    }
                }
            }
        }
        
        Size Element::UpdateDesiredSize(Size _ConstraintSize)
        {
            Size sizeDesired = {};
            if (_ConstraintSize.Width < 0)
                _ConstraintSize.Width = 0;

            if (_ConstraintSize.Height < 0)
                _ConstraintSize.Height = 0;

            const auto _bChangedConst = LocLastDesiredSizeConstraint != _ConstraintSize;

            if (pWindow && bNeedsDSUpdate || _bChangedConst)
            {
                bNeedsDSUpdate = false;

                if (_bChangedConst)
                {
                    auto pSizeOld = Value::CreateSize(LocLastDesiredSizeConstraint);
                    auto pSizeNew = Value::CreateSize(_ConstraintSize);

                    if (pSizeNew != nullptr)
                    {
                        PreSourceChange(Element::g_ControlInfoData.LastDesiredSizeConstraintProp, PropertyIndicies::PI_Local, pSizeOld, pSizeNew);

                        LocLastDesiredSizeConstraint = _ConstraintSize;

                        PostSourceChange();
                    }
                }

                auto nWidth = GetWidth();

                if (nWidth > _ConstraintSize.Width)
                    nWidth = _ConstraintSize.Width;

                auto nHeight = GetHeight();
                if (nHeight > _ConstraintSize.Height)
                    nHeight = _ConstraintSize.Height;

                sizeDesired.Width = nWidth == -1 ? _ConstraintSize.Width : nWidth;
                sizeDesired.Height = nHeight == -1 ? _ConstraintSize.Height : nHeight;

                auto BorderX = SpecBorderThickness.Left + SpecBorderThickness.Right;
                auto BorderY = SpecBorderThickness.Top + SpecBorderThickness.Bottom;

                BorderX += SpecPadding.Left + SpecPadding.Right;
                BorderY += SpecPadding.Top + SpecPadding.Bottom;

                if (bSpecFocusVisible)
                {
                    BorderX += SpecFocusThickness.Left + SpecFocusThickness.Right;
                    BorderY += SpecFocusThickness.Top + SpecFocusThickness.Bottom;
                }

                Size _ConstraintContentSize;
                _ConstraintContentSize.Width = sizeDesired.Width - BorderX;

                if (_ConstraintContentSize.Width < 0)
                {
                    BorderX += _ConstraintContentSize.Width;
                    _ConstraintContentSize.Width = 0;
                }

                _ConstraintContentSize.Height = sizeDesired.Height - BorderY;
                if (_ConstraintContentSize.Height < 0)
                {
                    BorderY += _ConstraintContentSize.Height;
                    _ConstraintContentSize.Height = 0;
                }

                Size _ContentSize;

                if (bSelfLayout)
                {
                    _ContentSize = SelfLayoutUpdateDesiredSize(_ConstraintContentSize);
                }
                else
                {
                    #if 0
                    if (pLayout)
                    {
                        TmpSize = pLayout->UpdateDesiredSize(this, x, y);
                    }
                    else
                    #endif
                    {
                        _ContentSize = GetContentSize(_ConstraintContentSize);
                    }
                }

                if (_ContentSize.Width < 0)
                {
                    _ContentSize.Width = 0;
                }
                else if (_ContentSize.Width > _ConstraintContentSize.Width)
                {
                    _ContentSize.Width = _ConstraintContentSize.Width;
                }
                if (_ContentSize.Height < 0)
                {
                    _ContentSize.Height = 0;
                }
                else if (_ContentSize.Height > _ConstraintContentSize.Height)
                {
                    _ContentSize.Height = _ConstraintContentSize.Height;
                }

                if (nWidth == -1)
                {
                    if (_ContentSize.Width + BorderX < sizeDesired.Width)
                        sizeDesired.Width = _ContentSize.Width + BorderX;
                }

                if (nHeight == -1)
                {
                    if (_ContentSize.Height + BorderY < sizeDesired.Height)
                        sizeDesired.Height = _ContentSize.Height + BorderY;
                }

                if (sizeDesired.Height < SpecMinSize.Width)
                {
                    sizeDesired.Width = min(_ConstraintSize.Width, SpecMinSize.Width);
                }

                if (sizeDesired.Height < SpecMinSize.Height)
                {
                    sizeDesired.Height = min(_ConstraintSize.Height, SpecMinSize.Height);
                }

                auto pSizeOld = Value::CreateSize(LocDesiredSize);
                auto pSizeNew = Value::CreateSize(sizeDesired);

                PreSourceChange(g_ControlInfoData.DesiredSizeProp, PropertyIndicies::PI_Local, pSizeOld, pSizeNew);

                LocDesiredSize = sizeDesired;

                PostSourceChange();
            }
            else
            {
                sizeDesired = LocDesiredSize;
            }
            return sizeDesired;
        }

        Rect Element::ApplyFlowDirection(const Rect& _Src)
        {
            if (GetFlowDirection() == FlowDirection::RightToLeft)
                return Rect(_Src.Right, _Src.Top, _Src.Left, _Src.Bottom);
            else
                return _Src;
        }

        void Element::Invalidate()
        {
            if (pWindow)
                pWindow->InvalidateRect(nullptr);
        }
        
        bool Element::ExtentPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::GetValue:
                GetExtentPropertyValue((GetValueHandleData*)_pHandleData);
                return true;
            default:
                break;
            }
            return false;
        }

        bool Element::GetExtentPropertyValue(GetValueHandleData* _pHandleData)
        {
            auto& pExtentValue = _pHandleData->Output.RetValue;

            if (pLocParent && iSpecLayoutPos != LP_Absolute)
            {
                pExtentValue = Value::CreateSize(LocSizeInLayout);
            }
            else
            {
                pExtentValue = Value::CreateSize(LocDesiredSize);
            }
            _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
            return true;
        }

        bool Element::LocationPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::GetValue:
                return GetLocationPropertyValue((GetValueHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::GetLocationPropertyValue(GetValueHandleData* _pHandleData)
        {
            auto& _Location = _pHandleData->Output.RetValue;

            if (pLocParent && iSpecLayoutPos != LP_Absolute)
            {
                _Location = Value::CreatePoint(LocPosInLayout);
            }
            else
            {
                _Location = Value::CreatePoint(GetX(), GetY());
            }

            _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
            return true;
        }
        
        bool Element::MouseFocusedPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                break;
            case CustomPropertyHandleType::GetDependencies:
                break;
            case CustomPropertyHandleType::GetValue:
                return GetMouseFocusedPropertyValue((GetValueHandleData*)_pHandleData);
            case CustomPropertyHandleType::SetValue:
                return SetMouseFocusedPropertyValue((SetValueHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::GetMouseFocusedPropertyValue(GetValueHandleData* _pHandleData)
        {
            auto& _RetValue = _pHandleData->Output.RetValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (bHasLocMouseFocused)
                {
                    _RetValue = Value::CreateBool(bLocMouseFocused);
                }
                else
                {
                    _RetValue = Value::CreateUnset();
                }
                _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                if (_pHandleData->bUsingCache)
                {
                    _pHandleData->Output.RetValue = Value::CreateBool(bSpecMouseFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                if (bHasLocMouseFocused)
                {
                    _RetValue = Value::CreateBool(bLocMouseFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                _RetValue = Value::CreateUnset();
                // 如果自己需要处理鼠标焦点，那么阻止 父节点继承
                _pHandleData->Output.CacheResult = (GetActive() & Active::Mouse) ? PropertyCustomCacheResult(SkipInherit | SkipLocalPropValue) : SkipLocalPropValue;
                return true;
            }

            return false;
        }

        bool Element::SetMouseFocusedPropertyValue(SetValueHandleData* _pHandleData)
        {
            auto& _InputNewValue = _pHandleData->InputNewValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (_InputNewValue.GetType() == ValueType::Unset)
                {
                    bHasLocMouseFocused = false;
                }
                else
                {
                    bHasLocMouseFocused = true;
                    bLocMouseFocused = _InputNewValue.GetBool();
                }
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                bSpecMouseFocused = _InputNewValue.GetBool();
                return true;
            }

            return false;
        }
        
        bool Element::KeyboardFocusedPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::OnPropertyChanged:
                break;
            case CustomPropertyHandleType::GetDependencies:
                break;
            case CustomPropertyHandleType::GetValue:
                return GetKeyboardFocusedPropertyValue((GetValueHandleData*)_pHandleData);
            case CustomPropertyHandleType::SetValue:
                return SetKeyboardFocusedPropertyValue((SetValueHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::GetKeyboardFocusedPropertyValue(GetValueHandleData* _pHandleData)
        {
            auto& _RetValue = _pHandleData->Output.RetValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (bHasLocKeyboardFocused)
                {
                    _RetValue = Value::CreateBool(bLocKeyboardFocused);
                }
                else
                {
                    _RetValue = Value::CreateUnset();
                }
                _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                if (_pHandleData->bUsingCache)
                {
                    _pHandleData->Output.RetValue = Value::CreateBool(bSpecKeyboardFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                if (bHasLocKeyboardFocused)
                {
                    _RetValue = Value::CreateBool(bLocKeyboardFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                _RetValue = Value::CreateUnset();
                // 如果自己需要处理键盘焦点，那么阻止 父节点继承
                _pHandleData->Output.CacheResult = (GetActive() & Active::Keyboard) ? PropertyCustomCacheResult(SkipInherit | SkipLocalPropValue) : SkipLocalPropValue;
                return true;
            }

            return false;
        }

        bool Element::SetKeyboardFocusedPropertyValue(SetValueHandleData* _pHandleData)
        {
            auto& _InputNewValue = _pHandleData->InputNewValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (_InputNewValue.GetType() == ValueType::Unset)
                {
                    bHasLocKeyboardFocused = false;
                }
                else
                {
                    bHasLocKeyboardFocused = true;
                    bLocKeyboardFocused = _InputNewValue.GetBool();
                }
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                bSpecKeyboardFocused = _InputNewValue.GetBool();
                return true;
            }

            return false;
        }

        bool Element::FocusedPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::GetValue:
                return GetFocusedPropertyValue((GetValueHandleData*)_pHandleData);
            case CustomPropertyHandleType::SetValue:
                return SetFocusedPropertyValue((SetValueHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::GetFocusedPropertyValue(GetValueHandleData* _pHandleData)
        {
            auto& _RetValue = _pHandleData->Output.RetValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (bHasLocFocused)
                {
                    _RetValue = Value::CreateBool(bLocFocused);
                }
                else
                {
                    _RetValue = Value::CreateUnset();
                }
                _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                if (_pHandleData->bUsingCache)
                {
                    _pHandleData->Output.RetValue = Value::CreateBool(bSpecFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                if (bHasLocFocused)
                {
                    _RetValue = Value::CreateBool(bLocFocused);
                    _pHandleData->Output.CacheResult = PropertyCustomCacheResult::SkipAll;
                    return true;
                }

                _RetValue = Value::CreateUnset();
                // 如果自己需要处理键盘焦点，那么阻止 父节点继承
                _pHandleData->Output.CacheResult = (GetActive() & Active::Keyboard) ? PropertyCustomCacheResult(SkipInherit | SkipLocalPropValue) : SkipLocalPropValue;
                return true;
            }

            return false;
        }

        bool Element::SetFocusedPropertyValue(SetValueHandleData* _pHandleData)
        {
            auto& _InputNewValue = _pHandleData->InputNewValue;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                if (_InputNewValue.GetType() == ValueType::Unset)
                {
                    bHasLocKeyboardFocused = false;
                }
                else
                {
                    bHasLocKeyboardFocused = true;
                    bLocKeyboardFocused = _InputNewValue.GetBool();
                }
                return true;
            }
            else if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                bSpecKeyboardFocused = _InputNewValue.GetBool();
                return true;
            }

            return false;
        }

        bool Element::GetParentDependencies(GetDependenciesHandleData* _pHandleData)
        {
            HRESULT& _hrLast = _pHandleData->Output.hr;

            _pHandleData->pdr->iDepPos = -1;
            _pHandleData->pdr->cDepCnt = 0;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Local)
            {
                auto pItem = _pHandleData->pDeferCycle->vecPropertyChanged.GetItemPtr(_pHandleData->iPCSrcRoot);
                if (!pItem)
                {
                    _hrLast = E_UNEXPECTED;
                    return true;
                }

                auto pElement = pItem->pNewValue.GetElement();
                if (!pElement)
                {
                    _hrLast = S_OK;
                    return true;
                }

                auto _pControlInfo = this->GetControlInfo();
                
                for (uint32_t _Index = 0;; ++_Index)
                {
                    auto _pProp = _pControlInfo->EnumPropertyInfo(_Index);
                    if (!_pProp)
                        break;

                    if ((_pProp->fFlags & PF_Inherit) == 0)
                        continue;

                    if (SpecCacheIsEqual(this, pElement, *_pProp) == 1)
                        continue;

                    {
                        auto _LocalValue = GetValue(*_pProp, PropertyIndicies::PI_Local, false);
                        if (_LocalValue == nullptr)
                            continue;
                    }

                    HRESULT _hr;
                    if (_pProp->fFlags & PF_40)
                    {
                        _hr = GetBuriedSheetDependencies(_pProp, pElement, _pHandleData->pdr, _pHandleData->pDeferCycle);
                    }
                    else
                    {
                        _hr = AddDependency(this, *_pProp, PropertyIndicies::PI_Specified, _pHandleData->pdr, _pHandleData->pDeferCycle);
                    }

                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
            }

            return true;
        }

        bool Element::SheetPropertyHandle(CustomPropertyHandleType _eType, CustomPropertyBaseHandleData* _pHandleData)
        {
            switch (_eType)
            {
            case CustomPropertyHandleType::GetDependencies:
                return GeSheetDependencies((GetDependenciesHandleData*)_pHandleData);
            default:
                break;
            }
            return false;
        }

        bool Element::GeSheetDependencies(GetDependenciesHandleData* _pHandleData)
        {
            HRESULT& _hrLast = _pHandleData->Output.hr;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Specified)
            {
                auto pItem = _pHandleData->pDeferCycle->vecPropertyChanged.GetItemPtr(_pHandleData->iPCSrcRoot);
                if (!pItem)
                {
                    _hrLast =  E_UNEXPECTED;
                    return true;
                }

                if (pItem->pProp == &Element::g_ControlInfoData.SheetProp)
                {
                    auto _pControlInfo = GetControlInfo();

                    for (uint32_t _uIndex = 0;; ++_uIndex)
                    {
                        auto _pProp = _pControlInfo->EnumPropertyInfo(_uIndex);
                        if (!_pProp)
                            break;

                        if (_pProp->fFlags & PropertyFlag::PF_Cascade)
                        {
                            auto _hr = AddDependency(this, *_pProp, PropertyIndicies::PI_Specified, _pHandleData->pdr, _pHandleData->pDeferCycle);
                            if (FAILED(_hr))
                                _hrLast = _hr;
                        }
                    }
                    
                    if (auto _pStyleSheet = _pHandleData->NewValue.GetStyleSheet())
                    {
                        auto _hr = _pStyleSheet->GetSheetScope(this, _pHandleData->pdr, _pHandleData->pDeferCycle);
                        if (FAILED(_hr))
                            _hrLast = _hr;
                    }
                }
                else if (pItem->pProp == &Element::g_ControlInfoData.ParentProp)
                {
                    auto _pNewParent = _pHandleData->NewValue.GetElement();
                    if (_pNewParent)
                    {
                        if(auto _pStyleSheet = _pNewParent->pSheet)
                        {
                            auto _hr = _pStyleSheet->GetSheetScope(this, _pHandleData->pdr, _pHandleData->pDeferCycle);
                            if (FAILED(_hr))
                                _hrLast = _hr;
                        }
                    }
                }

                if (auto _pStyleSheet = pSheet)
                {
                    auto _hr = _pStyleSheet->GetSheetScope(this, _pHandleData->pdr, _pHandleData->pDeferCycle);
                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
            }

            return true;
        }

        bool Element::GetVisibleDependencies(GetDependenciesHandleData* _pHandleData)
        {
            HRESULT& _hrLast = _pHandleData->Output.hr;

            if (_pHandleData->eIndicies == PropertyIndicies::PI_Computed)
            {
                for (auto _pChildern : GetChildren())
                {
                    auto _hr = AddDependency(_pChildern, *_pHandleData->pProp, PropertyIndicies::PI_Computed, _pHandleData->pdr, _pHandleData->pDeferCycle);
                    if (FAILED(_hr))
                        _hrLast = _hr;
                }
            }

            return true;
        }

        void Element::UpdateLayoutPosition(Point _LayoutPosition)
        {
            if (LocPosInLayout == _LayoutPosition)
                return;

            auto _pPointOld = Value::CreatePoint(LocPosInLayout);
            if (_pPointOld == nullptr)
                return;

            auto _pPointNew = Value::CreatePoint(_LayoutPosition);
            if (_pPointNew == nullptr)
                return;

            PreSourceChange(Element::g_ControlInfoData.PosInLayoutProp, PropertyIndicies::PI_Local, _pPointOld, _pPointNew);

            LocPosInLayout = _LayoutPosition;

            PostSourceChange();
        }

        void Element::UpdateLayoutSize(Size _LayoutSize)
        {
            if (LocSizeInLayout == _LayoutSize)
                return;
            
            auto _pSizeOld = Value::CreateSize(LocSizeInLayout);
            if (_pSizeOld == nullptr)
                return;

            auto _pSizeNew = Value::CreateSize(_LayoutSize);
            if (_pSizeNew == nullptr)
                return;

            PreSourceChange(Element::g_ControlInfoData.SizeInLayoutProp, PropertyIndicies::PI_Local, _pSizeOld, _pSizeNew);

            LocSizeInLayout = _LayoutSize;

            PostSourceChange();
        }
        
        HRESULT Element::OnHosted(Window* _pNewWindow)
        {
            if (pWindow || _pNewWindow == nullptr)
                return E_INVALIDARG;

            intptr_t _Cooike = 0;
            pWindow = _pNewWindow;
            if (_pNewWindow->IsInitialized() && iLocDpi != _pNewWindow->GetDpi())
            {
                StartDefer(&_Cooike);
                PreSourceChange(
                    Element::g_ControlInfoData.DpiProp,
                    PropertyIndicies::PI_Local,
                    Value::CreateInt32(iLocDpi),
                    Value::CreateInt32(_pNewWindow->GetDpi()));

                iLocDpi = _pNewWindow->GetDpi();

                PostSourceChange();
            }

            for (auto pElement : GetChildren())
            {
                pElement->OnHosted(_pNewWindow);
            }

            if (_Cooike)
                EndDefer(_Cooike);

            return S_OK;
        }
        
        HRESULT Element::OnUnHosted(Window* _pOldWindow)
        {
            if (!pWindow)
                return S_FALSE;

            if (pWindow != _pOldWindow)
                return E_INVALIDARG;

            for (auto pElement : GetChildren())
            {
                pElement->OnHosted(_pOldWindow);
            }

            pWindow = nullptr;
            return S_OK;
        }

        Element* Element::GetAdjacent(Element* _pFrom, NavigatingType _eNavDir, NavReference const* _pnr, bool _bKeyableOnly)
        {
            // todo
            return nullptr;
        }

        bool Element::OnKeyDown(const KeyboardEvent& _KeyEvent)
        {
            NavigatingType _NavigateType;

            switch (_KeyEvent.vKey)
            {
            case VK_DOWN:
                _NavigateType = NavigatingType::Down;
                break;
            case VK_UP:
                _NavigateType = NavigatingType::Up;
                break;
            case VK_LEFT:
                _NavigateType = GetFlowDirection() == FlowDirection::LeftToRight ? NavigatingType::Left : NavigatingType::Right;
                break;
            case VK_RIGHT:
                _NavigateType = GetFlowDirection() == FlowDirection::LeftToRight ? NavigatingType::Right : NavigatingType::Left;
                break;
            case VK_HOME:
                _NavigateType = NavigatingType::First;
                break;
            case VK_END:
                _NavigateType = NavigatingType::Last;
                break;
            default:
                return false; 
                break;
            }
            
            return OnKeyboardNavigate(KeyboardNavigateEvent(_KeyEvent.pTarget, _NavigateType));
        }

        bool Element::OnChar(const KeyboardEvent& _KeyEvent)
        {
            if (_KeyEvent.vKey == VK_TAB)
            {
                const auto _NavigateType = _KeyEvent.fModifiers & EventModifier::Shift ? NavigatingType::Previous : NavigatingType::Next;
                return OnKeyboardNavigate(KeyboardNavigateEvent(_KeyEvent.pTarget, _NavigateType));
            }
            return false;
        }
        
        bool Element::OnKeyboardNavigate(const KeyboardNavigateEvent& _KeyEvent)
        {
            auto _pForm = _KeyEvent.pTarget == this ? this : GetImmediateChild(_KeyEvent.pTarget);
            auto _pTo = GetAdjacent(_pForm, _KeyEvent.Navigate, nullptr, true);
            if (_pTo)
            {
                _pTo->SetKeyboardFocus();
                return true;
            }

            return false;
        }
	}
}
