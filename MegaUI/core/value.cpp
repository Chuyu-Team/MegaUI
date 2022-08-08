#include "pch.h"
#include "value.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        void __fastcall Value::SharedData::AddRef()
        {
            if (uRawType >= ~uint_t(0x7Fu))
                return;

            // 因为开头 7 位是eType 与 bSkipFree，所以每次对 cRef +1，等效于 uRawData + 0x80
            _InterlockedExchangeAdd(&uRawType, uint_t(0x80u));
        }

        void __fastcall Value::SharedData::Release()
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


        Value __fastcall Value::GetAtomZero()
        {
            _RETUNR_CONST_VALUE(ValueType::ATOM, 0);
        }

        Value __fastcall Value::GetInt32Zero()
        {
            return GetInt32ConstValue<0>();
        }

        Value __fastcall Value::GetBoolFalse()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, false);
        }

        Value __fastcall Value::GetBoolTrue()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, true);
        }

        Value __fastcall Value::GetCursorNull()
        {
            _RETUNR_CONST_VALUE(ValueType::HCURSOR, NULL);
        }

        Value __fastcall Value::GetElListNull()
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

        Value __fastcall Value::GetElementNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Element, nullptr);
        }

        Value __fastcall Value::GetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Null);
        }

        Value __fastcall Value::GetPointZero()
        {
            _RETUNR_CONST_VALUE(ValueType::POINT, { 0, 0 });
        }

        Value __fastcall Value::GetRectZero()
        {
            _RETUNR_CONST_VALUE(ValueType::Rect, {0, 0, 0, 0});
        }

        Value __fastcall Value::GetSizeZero()
        {
            _RETUNR_CONST_VALUE(ValueType::SIZE, { 0, 0 });
        }

        Value __fastcall Value::GetStringNull()
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

        Value __fastcall Value::GetUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::Unavailable);
        }

        Value __fastcall Value::GetUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::Unset);
        }

        Value __fastcall Value::GetLayoutNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Layout, nullptr);
        }

        Value __fastcall Value::GetColorTransparant()
        {
            _RETUNR_CONST_VALUE(ValueType::Color, {});
        }
        
        Value& __fastcall Value::operator=(const Value& _Other)
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

        Value& __fastcall Value::operator=(Value&& _Other) noexcept
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

        Value& __fastcall Value::operator=(std::nullptr_t)
        {
            if (pSharedData)
                pSharedData->Release();
            pSharedData = nullptr;
            return *this;
        }

        bool __fastcall Value::operator==(const Value& _Other) const
        {
            if (pSharedData == _Other.pSharedData)
                return true;

            if (!_Other.pSharedData)
                return false;

            if (pSharedData->eType != _Other.pSharedData->eType)
                return false;

            switch (ValueType(pSharedData->eType))
            {
            case YY::MegaUI::ValueType::int32_t:
                return pSharedData->int32Value == _Other.pSharedData->int32Value;
                break;
            case YY::MegaUI::ValueType::boolean:
                return pSharedData->boolValue == _Other.pSharedData->boolValue;
                break;
            case YY::MegaUI::ValueType::uString:
                return pSharedData->szValue == _Other.pSharedData->szValue;
                break;
            case YY::MegaUI::ValueType::POINT:
                return pSharedData->ptVal.x == _Other.pSharedData->ptVal.x && pSharedData->ptVal.y == _Other.pSharedData->ptVal.y;
                break;
            case YY::MegaUI::ValueType::SIZE:
                return pSharedData->sizeVal.cx == _Other.pSharedData->sizeVal.cx && pSharedData->sizeVal.cy == _Other.pSharedData->sizeVal.cy;
                break;
            case YY::MegaUI::ValueType::Rect:
                return EqualRect(&pSharedData->rectVal, &_Other.pSharedData->rectVal) != FALSE;
                break;
            case YY::MegaUI::ValueType::Element:
                return pSharedData->pEleValue == _Other.pSharedData->pEleValue;
                break;
            case YY::MegaUI::ValueType::ElementList:
                return pSharedData->ListVal == _Other.pSharedData->ListVal;
                break;
            case YY::MegaUI::ValueType::ATOM:
                return pSharedData->uAtomVal == _Other.pSharedData->uAtomVal;
                break;
            case YY::MegaUI::ValueType::HCURSOR:
                return pSharedData->hCursorVal == _Other.pSharedData->hCursorVal;
                break;
            case YY::MegaUI::ValueType::Color:
                return pSharedData->ColorValue.ColorRGBA == _Other.pSharedData->ColorValue.ColorRGBA;
                break;
            default:
                return true;
                break;
            }
        }

        bool __fastcall Value::operator!=(const Value& _Other) const
        {
            return operator==(_Other) == false;
        }

        bool __fastcall Value::operator==(std::nullptr_t) const
        {
            return pSharedData == nullptr;
        }

        bool __fastcall Value::operator!=(std::nullptr_t) const
        {
            return pSharedData != nullptr;
        }

        ValueType __fastcall Value::GetType()
        {
            if (!pSharedData)
                return ValueType::Null;
            return ValueType(pSharedData->eType);
        }
        
        Value __fastcall Value::CreateInt32(int32_t _iValue)
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
        
        Value __fastcall Value::CreateBool(bool _bValue)
        {
            return _bValue ? GetBoolTrue() : GetBoolFalse();
        }
        
        Value __fastcall Value::CreateElementRef(Element* _pValue)
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
        
        Value __fastcall Value::CreateElementList(const ElementList& _pListValue)
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
        
        Value __fastcall Value::CreateString(const uString& _szValue)
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
        
        Value __fastcall Value::CreatePoint(int32_t _iX, int32_t _iY)
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
        
        Value __fastcall Value::CreateSize(int32_t _iCX, int32_t _iCY)
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
        
        Value __fastcall Value::CreateRect(int32_t _iLeft, int32_t _iTop, int32_t _iRight, int32_t _iBottom)
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
        
        Value __fastcall Value::CreateAtom(raw_const_ustring_t _szValue)
        {
            return Value::CreateAtom(AddAtomW(_szValue));
        }
        
        Value __fastcall Value::CreateAtom(ATOM _uAtomValue)
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
        
        Value __fastcall Value::CreateCursor(raw_const_ustring_t _szValue)
        {
            if (_szValue == nullptr || _szValue[0] == 0)
                return nullptr;

            auto _hCursor = LoadCursorFromFileW(_szValue);

            if (_hCursor == NULL)
                return nullptr;

            auto _pValue = Value::CreateCursor(_hCursor);

            return _pValue;
        }
        
        Value __fastcall Value::CreateCursor(HCURSOR _hCursorValue)
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
        
        Value __fastcall Value::CreateColorRGBA(COLORREF ColorRGBA)
        {
            auto pValue = (Value::SharedData*)HAlloc(sizeof(Value::SharedData));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Color);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ColorValue.ColorRGBA = ColorRGBA;
            }
            return Value(pValue);
        }

        int32_t __fastcall Value::GetInt32()
        {
            if (GetType() != ValueType::int32_t)
                throw Exception();

            return pSharedData->int32Value;
        }
        
        bool __fastcall Value::GetBool()
        {
            if (GetType() != ValueType::boolean)
                throw Exception();

            return pSharedData->boolValue;
        }
        
        SIZE __fastcall Value::GetSize()
        {
            if (GetType() != ValueType::SIZE)
                throw Exception();

            return pSharedData->sizeVal;
        }
        
        POINT __fastcall Value::GetPoint()
        {
            if (GetType() != ValueType::POINT)
                throw Exception();

            return pSharedData->ptVal;
        }
        
        uint8_t* __fastcall Value::GetRawBuffer()
        {
            return pSharedData ? pSharedData->RawBuffer : nullptr;
        }
        
        Color __fastcall Value::GetColor()
        {
            if (GetType() != ValueType::Color)
                throw Exception();

            return pSharedData->ColorValue;
        }
    }
}


