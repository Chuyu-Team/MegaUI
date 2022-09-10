#include "pch.h"
#include "value.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        void __MEGA_UI_API Value::SharedData::AddRef()
        {
            if (uRawType >= ~uint_t(0x7Fu))
                return;

            // 因为开头 7 位是eType 与 bSkipFree，所以每次对 cRef +1，等效于 uRawData + 0x80
            _InterlockedExchangeAdd(&uRawType, uint_t(0x80u));
        }

        void __MEGA_UI_API Value::SharedData::Release()
        {
            if (uRawType >= ~uint_t(0x7Fu))
                return;

            // 等效于 cRef >= 2
            // 0x100 = 0x80 * 2
            if (_InterlockedExchangeAdd(&uRawType, uint_t(-int_t(0x80))) >= uint_t(0x100u))
                return;

            // 引用计数归零
            if (!bSkipFree)
            {
                switch (ValueType(eType))
                {
                case YY::MegaUI::ValueType::uString:
                    szValue.~StringBase();
                    break;
                case YY::MegaUI::ValueType::ElementList:
                    ListVal.~ElementList();
                    break;
                case YY::MegaUI::ValueType::ATOM:
                    if (uAtomVal)
                        DeleteAtom(uAtomVal);
                    break;
                default:
                    break;
                }
            }

            HFree(this);
        }


        Value __MEGA_UI_API Value::GetAtomZero()
        {
            _RETUNR_CONST_VALUE(ValueType::ATOM, 0);
        }

        Value __MEGA_UI_API Value::GetInt32Zero()
        {
            return GetInt32ConstValue<0>();
        }

        Value __MEGA_UI_API Value::GetBoolFalse()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, false);
        }

        Value __MEGA_UI_API Value::GetBoolTrue()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, true);
        }

        Value __MEGA_UI_API Value::GetCursorNull()
        {
            _RETUNR_CONST_VALUE(ValueType::HCURSOR, NULL);
        }

        Value __MEGA_UI_API Value::GetElListNull()
        {            
            static const ConstValueSharedData<const uchar_t*> g_ListNull =
            {
                (uint_t)ValueType::ElementList,
                1,
                uint_max,
                nullptr,
            };

            return Value((Value::SharedData*)&g_ListNull);
        }

        Value __MEGA_UI_API Value::GetElementNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Element, nullptr);
        }

        Value __MEGA_UI_API Value::GetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Null);
        }

        Value __MEGA_UI_API Value::GetPointZero()
        {
            _RETUNR_CONST_VALUE(ValueType::POINT, { 0, 0 });
        }

        Value __MEGA_UI_API Value::GetRectZero()
        {
            _RETUNR_CONST_VALUE(ValueType::Rect, {0, 0, 0, 0});
        }

        Value __MEGA_UI_API Value::GetSizeZero()
        {
            _RETUNR_CONST_VALUE(ValueType::SIZE, { 0, 0 });
        }

        Value __MEGA_UI_API Value::GetStringNull()
        {
            static const ConstValueSharedData<const uchar_t*> g_StringNull =
            {
                (uint_t)ValueType::uString,
                1,
                uint_max,
                uString::StringData::GetEmtpyStringData()->GetStringBuffer(),
            };

            return Value((Value::SharedData*)&g_StringNull);
        }

        Value __MEGA_UI_API Value::GetUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::Unavailable);
        }

        Value __MEGA_UI_API Value::GetUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::Unset);
        }

        Value __MEGA_UI_API Value::GetLayoutNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Layout, nullptr);
        }

        Value __MEGA_UI_API Value::GetColorTransparant()
        {
            _RETUNR_CONST_VALUE(ValueType::Color, {});
        }

        Value __MEGA_UI_API Value::GetSheetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::StyleSheet, nullptr);
        }
        
        Value& __MEGA_UI_API Value::operator=(const Value& _Other)
        {
            if (pSharedData != _Other.pSharedData)
            {
                if (pSharedData)
                    pSharedData->Release();

                pSharedData = _Other.pSharedData;

                if (pSharedData)
                    pSharedData->AddRef();
            }

            return *this;
        }

        Value& __MEGA_UI_API Value::operator=(Value&& _Other) noexcept
        {
            if (this != &_Other)
            {
                if (pSharedData)
                    pSharedData->Release();

                pSharedData = _Other.pSharedData;
                _Other.pSharedData = nullptr;
            }

            return *this;
        }

        Value& __MEGA_UI_API Value::operator=(std::nullptr_t)
        {
            if (pSharedData)
                pSharedData->Release();
            pSharedData = nullptr;
            return *this;
        }

        bool __MEGA_UI_API Value::operator==(const Value& _Other) const
        {
            return CmpValue(_Other, ValueCmpOperation::Equal);
        }

        bool __MEGA_UI_API Value::operator!=(const Value& _Other) const
        {
            return operator==(_Other) == false;
        }

        bool __MEGA_UI_API Value::operator==(std::nullptr_t) const
        {
            return pSharedData == nullptr;
        }

        bool __MEGA_UI_API Value::operator!=(std::nullptr_t) const
        {
            return pSharedData != nullptr;
        }

        ValueType __MEGA_UI_API Value::GetType() const
        {
            if (!pSharedData)
                return ValueType::Null;
            return ValueType(pSharedData->eType);
        }
        
        Value __MEGA_UI_API Value::CreateInt32(int32_t _iValue)
        {
            if (_iValue == 0)
                return GetInt32Zero();

            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::int32_t);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->int32Value = _iValue;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateBool(bool _bValue)
        {
            return _bValue ? GetBoolTrue() : GetBoolFalse();
        }
        
        Value __MEGA_UI_API Value::CreateElementRef(Element* _pValue)
        {
            if (_pValue == nullptr)
                return GetElementNull();

            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Element);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->pEleValue = _pValue;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateElementList(const ElementList& _pListValue)
        {
            if (_pListValue.GetSize() == 0)
                return GetElListNull();

            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ElementList);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->ListVal) ElementList(_pListValue);
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateString(const uString& _szValue)
        {
            if (_szValue.GetSize() == 0)
                return GetStringNull();

            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::uString);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->szValue) uString(_szValue);
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreatePoint(int32_t _iX, int32_t _iY)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::POINT);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ptVal.x = _iX;
                pValue->ptVal.y = _iY;

            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateSize(int32_t _iCX, int32_t _iCY)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::SIZE);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->sizeVal.cx = _iCX;
                pValue->sizeVal.cy = _iCY;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateRect(int32_t _iLeft, int32_t _iTop, int32_t _iRight, int32_t _iBottom)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Rect);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->rectVal.left = _iLeft;
                pValue->rectVal.top = _iTop;
                pValue->rectVal.right = _iRight;
                pValue->rectVal.bottom = _iBottom;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateAtom(raw_const_ustring_t _szValue)
        {
            return Value::CreateAtom(AddAtomW(_szValue));
        }
        
        Value __MEGA_UI_API Value::CreateAtom(ATOM _uAtomValue)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ATOM);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->uAtomVal = _uAtomValue;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateCursor(raw_const_ustring_t _szValue)
        {
            if (_szValue == nullptr || _szValue[0] == 0)
                return nullptr;

            auto _hCursor = LoadCursorFromFileW(_szValue);

            if (_hCursor == NULL)
                return nullptr;

            auto _pValue = Value::CreateCursor(_hCursor);

            return _pValue;
        }
        
        Value __MEGA_UI_API Value::CreateCursor(HCURSOR _hCursorValue)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::HCURSOR);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->hCursorVal = _hCursorValue;
            }
            return Value(pValue);
        }
        
        Value __MEGA_UI_API Value::CreateColor(Color _Color)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Color);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ColorValue = _Color;
            }
            return Value(pValue);
        }

        int32_t __MEGA_UI_API Value::GetInt32() const
        {
            if (GetType() != ValueType::int32_t)
                throw Exception();

            return pSharedData->int32Value;
        }
        
        bool __MEGA_UI_API Value::GetBool() const
        {
            if (GetType() != ValueType::boolean)
                throw Exception();

            return pSharedData->boolValue;
        }
        
        SIZE __MEGA_UI_API Value::GetSize() const
        {
            if (GetType() != ValueType::SIZE)
                throw Exception();

            return pSharedData->sizeVal;
        }
        
        POINT __MEGA_UI_API Value::GetPoint() const
        {
            if (GetType() != ValueType::POINT)
                throw Exception();

            return pSharedData->ptVal;
        }
        
        uint8_t* __MEGA_UI_API Value::GetRawBuffer()
        {
            return pSharedData ? pSharedData->RawBuffer : nullptr;
        }
        
        Color __MEGA_UI_API Value::GetColor() const
        {
            if (GetType() != ValueType::Color)
                throw Exception();

            return pSharedData->ColorValue;
        }

        Element* __MEGA_UI_API Value::GetElement() const
        {
            if (GetType() != ValueType::Element)
                throw Exception();

            return pSharedData->pEleValue;
        }

        bool __MEGA_UI_API Value::CmpValue(const Value& _Other, ValueCmpOperation _Operation) const
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
            case ValueType::uString:
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
            case ValueType::POINT:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->ptVal.x == _Other.pSharedData->ptVal.x
                        && pSharedData->ptVal.y == _Other.pSharedData->ptVal.y;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->ptVal.x != _Other.pSharedData->ptVal.x
                        || pSharedData->ptVal.y != _Other.pSharedData->ptVal.y;
                    break;
                }
                break;
            case ValueType::SIZE:
                switch (_Operation)
                {
                case ValueCmpOperation::Equal:
                    return pSharedData->sizeVal.cx == _Other.pSharedData->sizeVal.cx
                        && pSharedData->sizeVal.cy == _Other.pSharedData->sizeVal.cy;
                    break;
                case ValueCmpOperation::NotEqual:
                    return pSharedData->sizeVal.cx != _Other.pSharedData->sizeVal.cx
                        || pSharedData->sizeVal.cy != _Other.pSharedData->sizeVal.cy;
                    break;
                }
                break;
            case ValueType::Rect:
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
    } // namespace MegaUI
}


