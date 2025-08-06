#pragma once
#include <YY/Base/YY.h>
#include <Media/Rect.h>
#include <Media/Size.h>
#include <Media/Font.h>
#include <MegaUI/Core/TextScaleManger.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        enum class UnitType : uint8_t
        {
            None = 0,
            // 设备相关像素（缩写px），等价于 None
            Pixel = None,
            // 设备无关像素（缩写 dp或dip）, px = dp * dpi / USER_DEFAULT_SCREEN_DPI
            DevicePixel,
            // 缩放无关像素（缩写 sp），px = (sp * TextScale) * dpi / USER_DEFAULT_SCREEN_DPI
            ScalePixel,
            // 字体的点数，也称呼为磅（缩写 pt），px = pt * dpi / 72
            FontPoint,
            // 缩放无关的字体的点数（缩写 spt），px = (spt * TextScale) * dpi / 72
            ScaleFontPoint,
        };
        
        inline bool __YYAPI IsRelativeUnit(UnitType _eType) noexcept
        {
            return _eType != UnitType::Pixel;
        }

        struct UnitMetrics
        {
            uint32_t uDpi = USER_DEFAULT_SCREEN_DPI;
            float nTextScale = 1.0f;

            static UnitMetrics __YYAPI GetSystemMetrics() noexcept
            {
                return UnitMetrics {GetDpiForSystem(), GetSystemTextScale()};
            }

            constexpr float __YYAPI ApplyDimension(UnitType _eType, float _nValue) const noexcept
            {
                switch (_eType)
                {
                case UnitType::Pixel:
                    return _nValue;
                case UnitType::DevicePixel:
                    return _nValue * uDpi / USER_DEFAULT_SCREEN_DPI;
                case UnitType::ScalePixel:
                    return _nValue * nTextScale * uDpi / USER_DEFAULT_SCREEN_DPI;
                case UnitType::FontPoint:
                    return _nValue * uDpi / 72;
                case UnitType::ScaleFontPoint:
                    return _nValue * nTextScale * uDpi / 72;
                default:
                    return 0.0f;
                }
            }
        };

        struct Unit
        {
            float Value = 0.0f;
            UnitType eType = UnitType::Pixel;
            
            constexpr float __YYAPI ApplyDimension(const UnitMetrics& _oMetrics) const noexcept
            {
                return _oMetrics.ApplyDimension(eType, Value);
            }

            bool __YYAPI IsRelativeUnit() const noexcept
            {
                return YY::MegaUI::IsRelativeUnit(eType);
            }

            bool __YYAPI operator==(const Unit& _oOther) const noexcept
            {
                return Value == _oOther.Value && eType == _oOther.eType;
            }

#if !defined(_HAS_CXX20) || _HAS_CXX20 == 0
            bool __YYAPI operator!=(const Unit& _oOther) const noexcept
            {
                return Value != _oOther.Value || eType != _oOther.eType;
            }
#endif
        };

        struct UnitSize
        {
            Size Value;
            UnitType eWidthType = UnitType::None;
            UnitType eHeightType = UnitType::None;
            
            constexpr UnitSize() noexcept = default;

            constexpr UnitSize(const Size& _Value) noexcept
                : Value(_Value)
            {
            }

            constexpr UnitSize(const Unit& _Width, const Unit& _Height) noexcept
                : Value(_Width.Value, _Height.Value)
                , eWidthType(_Width.eType)
                , eHeightType(_Height.eType)
            {
            }

            Unit __YYAPI GetWidth() const noexcept
            {
                return Unit {Value.Width, eWidthType};
            }

            Unit __YYAPI GetHeight() const noexcept
            {
                return Unit {Value.Height, eHeightType};
            }

            constexpr Size __YYAPI ApplyDimension(const UnitMetrics& _oMetrics) const noexcept
            {
                return Size(_oMetrics.ApplyDimension(eWidthType, Value.Width), _oMetrics.ApplyDimension(eHeightType, Value.Height));
            }

            bool __YYAPI IsRelativeUnit() const noexcept
            {
                return YY::MegaUI::IsRelativeUnit(eWidthType) || YY::MegaUI::IsRelativeUnit(eHeightType);
            }

            bool __YYAPI operator==(const UnitSize& _oOther) const noexcept
            {
                return Value == _oOther.Value && eWidthType == _oOther.eWidthType && eHeightType == _oOther.eHeightType;
            }

#if !defined(_HAS_CXX20) || _HAS_CXX20 == 0
            bool __YYAPI operator!=(const UnitSize& _oOther) const noexcept
            {
                return Value != _oOther.Value || eWidthType != _oOther.eWidthType || eHeightType != _oOther.eHeightType;
            }
#endif
        };

        struct UnitRect
        {
            Rect Value;
            UnitType eLeftType = UnitType::None;
            UnitType eTopType = UnitType::None;
            UnitType eRightType = UnitType::None;
            UnitType eBottomType = UnitType::None;

            constexpr UnitRect() noexcept = default;

            constexpr UnitRect(const Rect& _Value) noexcept
                : Value(_Value)
            {
            }

            constexpr UnitRect(const Unit& _Left, const Unit& _Top, const Unit& _Right, const Unit& _Bottom) noexcept
                : Value(_Left.Value, _Top.Value, _Right.Value, _Bottom.Value)
                , eLeftType(_Left.eType)
                , eTopType(_Top.eType)
                , eRightType(_Right.eType)
                , eBottomType(_Bottom.eType)
            {
            }

            Unit __YYAPI GetLeft() const noexcept
            {
                return Unit {Value.Left, eLeftType};
            }

            Unit __YYAPI GetTop() const noexcept
            {
                return Unit {Value.Top, eTopType};
            }

            Unit __YYAPI GetRight() const noexcept
            {
                return Unit {Value.Right, eRightType};
            }

            Unit __YYAPI GetBottom() const noexcept
            {
                return Unit {Value.Bottom, eBottomType};
            }

            constexpr Rect __YYAPI ApplyDimension(const UnitMetrics& _oMetrics) const noexcept
            {
                return Rect(
                    _oMetrics.ApplyDimension(eLeftType, Value.Left),
                    _oMetrics.ApplyDimension(eTopType, Value.Top),
                    _oMetrics.ApplyDimension(eRightType, Value.Right),
                    _oMetrics.ApplyDimension(eBottomType, Value.Bottom));
            }

            bool __YYAPI IsRelativeUnit() const noexcept
            {
                return YY::MegaUI::IsRelativeUnit(eLeftType) || YY::MegaUI::IsRelativeUnit(eTopType) || YY::MegaUI::IsRelativeUnit(eRightType) || YY::MegaUI::IsRelativeUnit(eBottomType);
            }

            bool __YYAPI operator==(const UnitRect& _oOther) const noexcept
            {
                return Value == _oOther.Value && eLeftType == _oOther.eLeftType && eTopType == _oOther.eTopType && eRightType == _oOther.eRightType && eBottomType == _oOther.eBottomType;
            }

#if !defined(_HAS_CXX20) || _HAS_CXX20 == 0
            bool __YYAPI operator!=(const UnitRect& _oOther) const noexcept
            {
                return Value != _oOther.Value || eLeftType != _oOther.eLeftType || eTopType != _oOther.eTopType || eRightType != _oOther.eRightType || eBottomType != _oOther.eBottomType;
            }
#endif
        };
    } // namespace MegaUI
} // namespace YY
