#include "pch.h"
#include <MegaUI/Core/Value.h>

#include <stdlib.h>

#include <MegaUI/Core/StyleSheet.h>
#include <MegaUI/Core/Element.h>
#include <Base/Memory/Alloc.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {
        void __YYAPI Value::SharedData::AddRef()
        {
            if (IsReadOnly())
                return;

            // 因为开头 7 位是eType 与 bSkipFree，所以每次对 cRef +1，等效于 uRawData + 0x80
            Sync::Add(&uRawType, uint_t(0x80u));
        }

        void __YYAPI Value::SharedData::Release()
        {
            if (IsReadOnly())
                return;

            // 等效于 --cRef >= 1
            if (Sync::Subtract(&uRawType, uint_t(0x80)) >= uint_t(0x80))
                return;

            // 引用计数归零
            if (!bSkipFree)
            {
                switch (ValueType(eType))
                {
                case ValueType::String:
                    szValue.~StringBase();
                    break;
                case ValueType::ElementList:
                    ListVal.~ElementList();
                    break;
#ifdef _WIN32
                case ValueType::ATOM:
                    if (uAtomVal)
                        DeleteAtom(uAtomVal);
                    break;
#endif
                case ValueType::StyleSheet:
                    if (pStyleSheet)
                        pStyleSheet->Release();
                    break;
                default:
                    break;
                }
            }

            Free(this);
        }

        bool __YYAPI Value::SharedData::IsReadOnly() const
        {
            return uRawType >= ~uint_t(0x7Fu);
        }

        bool __YYAPI Value::SharedData::IsRelativeUnit() const
        {
            switch (ValueType(eType))
            {
            case ValueType::Unit:
                return UnitValue.IsRelativeUnit();
            case ValueType::UnitSize:
                return sizeVal.IsRelativeUnit();
            case ValueType::UnitRect:
                return rectVal.IsRelativeUnit();
            default:
                return false;
            }
        }

        Value __YYAPI Value::CreateAtomZero()
        {
            _RETUNR_CONST_VALUE(ValueType::ATOM, 0);
        }

        Value __YYAPI Value::CreateInt32Zero()
        {
            return CreateInt32<0>();
        }

        Value __YYAPI Value::CreateBoolFalse()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, false);
        }

        Value __YYAPI Value::CreateBoolTrue()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, true);
        }

        Value __YYAPI Value::CreateCursorNull()
        {
            _RETUNR_CONST_VALUE(ValueType::HCURSOR, NULL);
        }

        Value __YYAPI Value::CreateEmptyElementList()
        {            
            static const ConstValueSharedData<const uchar_t*> g_ListNull =
            {
                (uint_t)ValueType::ElementList,
                1,
                (std::numeric_limits<uint_t>::max)(),
                (uchar_t*)ElementList::SharedData::GetEmptySharedData()->GetData(),
            };

            return Value((Value::SharedData*)&g_ListNull);
        }

        Value __YYAPI Value::CreateElementNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Element, nullptr);
        }

        Value __YYAPI Value::CreateNull()
        {
            return Value();
        }

        Value __YYAPI Value::CreatePointZero()
        {
            _RETUNR_CONST_VALUE(ValueType::Point, { 0, 0 });
        }

        Value __YYAPI Value::CreateRectZero()
        {
            _RETUNR_CONST_VALUE(ValueType::UnitRect, {});
        }

        Value __YYAPI Value::CreateSizeZero()
        {
            _RETUNR_CONST_VALUE(ValueType::UnitSize, {});
        }

        Value __YYAPI Value::CreateEmptyString()
        {
            static const ConstValueSharedData<const uchar_t*> g_StringNull =
            {
                (uint_t)ValueType::String,
                1,
                (std::numeric_limits<uint_t>::max)(),
                uString::StringData::GetEmtpyStringData()->GetStringBuffer(),
            };

            return Value((Value::SharedData*)&g_StringNull);
        }

        Value __YYAPI Value::CreateUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::Unavailable);
        }

        Value __YYAPI Value::CreateUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::Unset);
        }

        Value __YYAPI Value::CreateLayoutNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Layout, nullptr);
        }

        Value __YYAPI Value::CreateDefaultFontFamily()
        {
            return Value::CreateString(_S("Arial"));
        }

        Value __YYAPI Value::CreateColorTransparant()
        {
            _RETUNR_CONST_VALUE(ValueType::Color, {});
        }

        Value __YYAPI Value::CreateSheetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::StyleSheet, nullptr);
        }

        Value __YYAPI Value::CreateDefaultSystemDpi()
        {
            static const ConstValueSharedData<int32_t> g_DefaultSystemDpi =
            {
                (uint_t)ValueType::int32_t,
                1,
                (std::numeric_limits<uint_t>::max)(),
                GetDpiForSystem(),
            };

            return Value((Value::SharedData*)&g_DefaultSystemDpi);
        }

        Value& __YYAPI Value::operator=(std::nullptr_t)
        {
            pSharedData = nullptr;
            return *this;
        }

        bool __YYAPI Value::operator==(std::nullptr_t) const
        {
            return pSharedData == nullptr;
        }

        ValueType __YYAPI Value::GetType() const
        {
            if (!pSharedData)
                return ValueType::Null;
            return ValueType(pSharedData->eType);
        }

        bool __YYAPI Value::HasValue() const
        {
            return pSharedData && pSharedData->eType > int_t(ValueType::Null);
        }

        Value __YYAPI Value::CreateInt32(int32_t _iValue)
        {
            if (_iValue == 0)
                return CreateInt32Zero();

            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::int32_t);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->int32Value = _iValue;
            }
            return Value(pValue);
        }

        Value __YYAPI Value::CreateUnit(const Unit& _iValue)
        {
            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Unit);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->UnitValue = _iValue;
            }
            return Value(pValue);
        }

        Value __YYAPI Value::CreateBool(bool _bValue)
        {
            return _bValue ? CreateBoolTrue() : CreateBoolFalse();
        }
        
        Value __YYAPI Value::CreateElementRef(Element* _pValue)
        {
            if (_pValue == nullptr)
                return CreateElementNull();

            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Element);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->pEleValue = _pValue;
            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreateElementList(const ElementList& _pListValue)
        {
            if (_pListValue.GetSize() == 0)
                return CreateEmptyElementList();

            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ElementList);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->ListVal) ElementList(_pListValue);
            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreateString(const uString& _szValue)
        {
            if (_szValue.GetSize() == 0)
                return CreateEmptyString();

            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::String);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->szValue) uString(_szValue);
            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreatePoint(float _iX, float _iY)
        {
            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Point);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ptVal.X = _iX;
                pValue->ptVal.Y = _iY;

            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreateSize(const UnitSize& _Size)
        {
            auto pValue = (Value::SharedData*)AllocAndZero(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::UnitSize);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->sizeVal = _Size;
            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreateRect(const UnitRect& _Rect)
        {
            auto pValue = (Value::SharedData*)AllocAndZero(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::UnitRect);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->rectVal = _Rect;
            }
            return Value(pValue);
        }

#ifdef _WIN32
        Value __YYAPI Value::CreateAtom(raw_const_ustring_t _szValue)
        {
            return Value::CreateAtom(AddAtomW(_szValue));
        }
        
        Value __YYAPI Value::CreateAtom(ATOM _uAtomValue)
        {
            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ATOM);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->uAtomVal = _uAtomValue;
            }
            return Value(pValue);
        }
        
        Value __YYAPI Value::CreateCursor(raw_const_ustring_t _szValue)
        {
            if (_szValue == nullptr || _szValue[0] == 0)
                return nullptr;

            auto _hCursor = LoadCursorFromFileW(_szValue);

            if (_hCursor == NULL)
                return nullptr;

            auto _pValue = Value::CreateCursor(_hCursor);

            return _pValue;
        }
        
        Value __YYAPI Value::CreateCursor(HCURSOR _hCursorValue)
        {
            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::HCURSOR);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->hCursorVal = _hCursorValue;
            }
            return Value(pValue);
        }
#endif

        Value __YYAPI Value::CreateColor(Color _Color)
        {
            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Color);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ColorValue = _Color;
            }
            return Value(pValue);
        }

        Value __YYAPI Value::CreateStyleSheet(_In_ StyleSheet* _pStyleSheet)
        {
            if (!_pStyleSheet)
                return Value::CreateSheetNull();

            auto pValue = (Value::SharedData*)Alloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::StyleSheet);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->pStyleSheet = _pStyleSheet;
                if (_pStyleSheet)
                    _pStyleSheet->AddRef();

            }
            return Value(pValue);
        }

        int32_t __YYAPI Value::GetInt32() const
        {
            if (GetType() != ValueType::int32_t)
                throw Exception();

            return pSharedData->int32Value;
        }

        Unit& __YYAPI Value::GetUnit() const
        {
            if (GetType() != ValueType::Unit)
                throw Exception();

            return pSharedData->UnitValue;
        }
        
        bool __YYAPI Value::GetBool() const
        {
            if (GetType() != ValueType::boolean)
                throw Exception();

            return pSharedData->boolValue;
        }
        
        UnitSize __YYAPI Value::GetSize() const
        {
            if (GetType() != ValueType::UnitSize)
                throw Exception();

            return pSharedData->sizeVal;
        }
        
        Point __YYAPI Value::GetPoint() const
        {
            if (GetType() != ValueType::Point)
                throw Exception();

            return pSharedData->ptVal;
        }
        
        uint8_t* __YYAPI Value::GetRawBuffer()
        {
            return pSharedData ? pSharedData->RawBuffer : nullptr;
        }
        
        Color __YYAPI Value::GetColor() const
        {
            if (GetType() != ValueType::Color)
                throw Exception();

            return pSharedData->ColorValue;
        }

        Element* __YYAPI Value::GetElement() const
        {
            if (GetType() != ValueType::Element)
                throw Exception();

            return pSharedData->pEleValue;
        }

        ElementList Value::GetElementList() const
        {
            if (GetType() != ValueType::Element)
                throw Exception();

            return pSharedData->ListVal;
        }

        uString __YYAPI Value::GetString() const
        {
            if (GetType() != ValueType::String)
                throw Exception();
            return pSharedData->szValue;
        }

        StyleSheet* __YYAPI Value::GetStyleSheet() const
        {
            if (GetType() != ValueType::StyleSheet)
                throw Exception();
            return pSharedData->pStyleSheet;
        }

        UnitRect& __YYAPI Value::GetRect() const
        {
            if (GetType() != ValueType::UnitRect)
                throw Exception();
            return pSharedData->rectVal;
        }

        bool __YYAPI Value::CmpValue(const Value& _Other, ValueCmpOperation _Operation, bool _bIgnoreDpi) const
        {
            if (pSharedData == _Other.pSharedData)
            {
                switch (GetType())
                {
                case ValueType::int32_t:
                    return _Operation == ValueCmpOperation::Equal || _Operation == ValueCmpOperation::GreaterThanOrEqual || _Operation == ValueCmpOperation::LessThanOrEqual;
                    break;
                default:
                    return _Operation == ValueCmpOperation::Equal;
                    break;
                }
            }

            // 类型不同时，只能比较 NotEqual
            if (GetType() != _Other.GetType())
                return _Operation == ValueCmpOperation::NotEqual;

            switch (GetType())
            {
            case ValueType::int32_t:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return GetInt32() == _Other.GetInt32();
                    break;
                case ValueCmpOperation::NotEqual:
                    return GetInt32() != _Other.GetInt32();
                    break;
                case ValueCmpOperation::GreaterThan:
                    return GetInt32() > _Other.GetInt32();
                    break;
                case ValueCmpOperation::GreaterThanOrEqual:
                    return GetInt32() >= _Other.GetInt32();
                    break;
                case ValueCmpOperation::LessThan:
                    return GetInt32() < _Other.GetInt32();
                    break;
                case ValueCmpOperation::LessThanOrEqual:
                    return GetInt32() <= _Other.GetInt32();
                    break;
                }
                break;
            case ValueType::Unit:
            {
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->UnitValue == _Other.pSharedData->UnitValue;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->UnitValue != _Other.pSharedData->UnitValue;
                    break;
                }
                break;
            }
            case ValueType::boolean:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return GetBool() == _Other.GetBool();
                    break;
                case ValueCmpOperation::NotEqual:
                    return GetBool() != _Other.GetBool();
                    break;
                }
                break;
            case ValueType::String:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->szValue == _Other.pSharedData->szValue;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->szValue != _Other.pSharedData->szValue;
                    break;
                }
                break;
            case ValueType::Point:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->ptVal == _Other.pSharedData->ptVal;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->ptVal != _Other.pSharedData->ptVal;
                    break;
                }
                break;
            case ValueType::UnitSize:
            {
                auto& _iLeft = pSharedData->sizeVal;
                auto& _iRight = _Other.pSharedData->sizeVal;
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return _iLeft == _iRight;
                    break;
                case ValueCmpOperation::NotEqual:
                    return _iLeft != _iRight;
                    break;
                }
                break;
            }
            case ValueType::UnitRect:
            {
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->rectVal == _Other.pSharedData->rectVal;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->rectVal != _Other.pSharedData->rectVal;
                    break;
                }
                break;
            }
            case ValueType::Element:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->pEleValue == _Other.pSharedData->pEleValue;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->pEleValue != _Other.pSharedData->pEleValue;
                    break;
                }
                break;
            case ValueType::ElementList:
                if (_Operation == ValueCmpOperation::Equal || _Operation == ValueCmpOperation::NotEqual)
                {
                    auto _uSize = pSharedData->ListVal.GetSize();
                    auto _bSame = _uSize == _Other.pSharedData->ListVal.GetSize();
                    if (_bSame)
                    {
                        for (uint32_t _uIndex = 0; _uIndex != _uSize;++ _uIndex)
                        {
                            _bSame = pSharedData->ListVal[_uIndex] == _Other.pSharedData->ListVal[_uIndex];
                            if (!_bSame)
                                break;
                        }
                    }

                    return _Operation == ValueCmpOperation::Equal ? _bSame : _bSame == false;
                }
            case ValueType::ATOM:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->uAtomVal == _Other.pSharedData->uAtomVal;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->uAtomVal != _Other.pSharedData->uAtomVal;
                    break;
                }
                break;
            case ValueType::HCURSOR:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->hCursorVal == _Other.pSharedData->hCursorVal;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->hCursorVal != _Other.pSharedData->hCursorVal;
                    break;
                }
                break;
            case ValueType::Layout:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->pLayout == _Other.pSharedData->pLayout;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->pLayout != _Other.pSharedData->pLayout;
                    break;
                }
                break;
            case ValueType::Color:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->ColorValue.ColorRGBA == _Other.pSharedData->ColorValue.ColorRGBA;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->ColorValue.ColorRGBA != _Other.pSharedData->ColorValue.ColorRGBA;
                    break;
                }
                break;
            default:
                break;
            }

            return false;
        }

        bool __YYAPI Value::IsSame(const Value& _Other) const
        {
            return pSharedData == _Other.pSharedData;
        }
    } // namespace MegaUI
}


