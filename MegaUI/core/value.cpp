#include "pch.h"
#include "value.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        Value* __fastcall Value::GetAtomZero()
        {
            _RETUNR_CONST_VALUE(ValueType::ATOM, 0);
        }

        Value* __fastcall Value::GetInt32Zero()
        {
            return GetInt32ConstValue<0>();
        }

        Value* __fastcall Value::GetBoolFalse()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, false);
        }

        Value* __fastcall Value::GetBoolTrue()
        {
            _RETUNR_CONST_VALUE(ValueType::boolean, true);
        }

        Value* __fastcall Value::GetCursorNull()
        {
            _RETUNR_CONST_VALUE(ValueType::HCURSOR, NULL);
        }

        Value* __fastcall Value::GetElListNull()
        {            
            static const ConstValue<const uchar_t*> g_ListNull =
            {
                (uint_t)ValueType::ElementList,
                1,
                uint_max,
                nullptr,
            };

            return (Value*)&g_ListNull;
        }

        Value* __fastcall Value::GetElementNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Element, nullptr);
        }

        Value* __fastcall Value::GetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Null);
        }

        Value* __fastcall Value::GetPointZero()
        {
            _RETUNR_CONST_VALUE(ValueType::POINT, { 0, 0 });
        }

        Value* __fastcall Value::GetRectZero()
        {
            _RETUNR_CONST_VALUE(ValueType::RECT, { 0, 0, 0, 0 });
        }

        Value* __fastcall Value::GetSizeZero()
        {
            _RETUNR_CONST_VALUE(ValueType::SIZE, { 0, 0 });
        }

        Value* __fastcall Value::GetStringNull()
        {
            static const ConstValue<const uchar_t*> g_StringNull =
            {
                (uint_t)ValueType::uString,
                1,
                uint_max,
                uString::StringData::GetEmtpyStringData()->GetStringBuffer(),
            };

            return (Value*)&g_StringNull;
        }

        Value* __fastcall Value::GetUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::Unavailable);
        }

        Value* __fastcall Value::GetUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::Unset);
        }

        Value* __fastcall Value::GetLayoutNull()
        {
            _RETUNR_CONST_VALUE(ValueType::Layout, nullptr);
        }
        
        void __fastcall Value::AddRef()
        {
            if (uRawType >= ~uint_t(0x7Fu))
                return;

            // 因为开头 7 位是eType 与 bSkipFree，所以每次对 cRef +1，等效于 uRawData + 0x80
            _InterlockedExchangeAdd(&uRawType, uint_t(0x80u));
        }
        
        void __fastcall Value::Release()
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
        
        bool __fastcall Value::IsEqual(Value* pValue)
        {
            if (this == pValue)
                return true;

            if (!pValue)
                return false;

            if (eType != pValue->eType)
                return false;

            switch (ValueType(eType))
            {
            case YY::MegaUI::ValueType::int32_t:
                return int32Value == pValue->int32Value;
                break;
            case YY::MegaUI::ValueType::boolean:
                return boolValue == pValue->boolValue;
                break;
            case YY::MegaUI::ValueType::uString:
                return szValue == pValue->szValue;
                break;
            case YY::MegaUI::ValueType::POINT:
                return ptVal.x == pValue->ptVal.x && ptVal.y == pValue->ptVal.y;
                break;
            case YY::MegaUI::ValueType::SIZE:
                return sizeVal.cx == pValue->sizeVal.cx && sizeVal.cy == pValue->sizeVal.cy;
                break;
            case YY::MegaUI::ValueType::RECT:
                return EqualRect(&rectVal, &pValue->rectVal) != FALSE;
                break;
            case YY::MegaUI::ValueType::Element:
                return pEleValue == pValue->pEleValue;
                break;
            case YY::MegaUI::ValueType::ElementList:
                return ListVal == pValue->ListVal;
                break;
            case YY::MegaUI::ValueType::ATOM:
                return uAtomVal == pValue->uAtomVal;
                break;
            case YY::MegaUI::ValueType::HCURSOR:
                return hCursorVal == pValue->hCursorVal;
                break;
            default:
                return true;
                break;
            }
        }
        
        ValueType __fastcall Value::GetType()
        {
            return ValueType(eType);
        }
        
        Value* __fastcall Value::CreateInt32(int32_t _iValue)
        {
            if (_iValue == 0)
                return GetInt32Zero();

            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::int32_t);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->int32Value = _iValue;
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateBool(bool _bValue)
        {
            return _bValue ? GetBoolTrue() : GetBoolFalse();
        }
        
        Value* __fastcall Value::CreateElementRef(Element* _pValue)
        {
            if (_pValue == nullptr)
                return GetElementNull();

            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::Element);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->pEleValue = _pValue;
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateElementList(const ElementList& _pListValue)
        {
            if (_pListValue.GetSize() == 0)
                return GetElListNull();

            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ElementList);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->ListVal) ElementList(_pListValue);
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateString(const uString& _szValue)
        {
            if (_szValue.GetSize() == 0)
                return GetStringNull();

            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::uString);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                new (&pValue->szValue) uString(_szValue);
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreatePoint(int32_t _iX, int32_t _iY)
        {
            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::POINT);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->ptVal.x = _iX;
                pValue->ptVal.y = _iY;

            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateSize(int32_t _iCX, int32_t _iCY)
        {
            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::SIZE);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->sizeVal.cx = _iCX;
                pValue->sizeVal.cy = _iCY;
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateRect(int32_t _iLeft, int32_t _iTop, int32_t _iRight, int32_t _iBottom)
        {
            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::RECT);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->rectVal.left = _iLeft;
                pValue->rectVal.top = _iTop;
                pValue->rectVal.right = _iRight;
                pValue->rectVal.bottom = _iBottom;
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateAtom(raw_const_ustring_t _szValue)
        {
            return Value::CreateAtom(AddAtomW(_szValue));
        }
        
        Value* __fastcall Value::CreateAtom(ATOM _uAtomValue)
        {
            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::ATOM);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->uAtomVal = _uAtomValue;
            }
            return pValue;
        }
        
        Value* __fastcall Value::CreateCursor(raw_const_ustring_t _szValue)
        {
            if (_szValue == nullptr || _szValue[0] == 0)
                return nullptr;

            auto _hCursor = LoadCursorFromFileW(_szValue);

            if (_hCursor == NULL)
                return nullptr;

            auto _pValue = Value::CreateCursor(_hCursor);

            return _pValue;
        }
        
        Value* __fastcall Value::CreateCursor(HCURSOR _hCursorValue)
        {
            auto pValue = (Value*)HAlloc(sizeof(Value));
            if (pValue)
            {
                pValue->eType = uint_t(ValueType::HCURSOR);
                pValue->bSkipFree = 0;
                pValue->cRef = 1;
                pValue->hCursorVal = _hCursorValue;
            }
            return pValue;
        }
        
        int32_t __fastcall Value::GetInt32()
        {
            if (GetType() != ValueType::int32_t)
                throw Exception();

            return int32Value;
        }
        
        bool __fastcall Value::GetBool()
        {
            if (GetType() != ValueType::boolean)
                throw Exception();

            return boolValue;
        }
        
        SIZE __fastcall Value::GetSize()
        {
            if (GetType() != ValueType::SIZE)
                throw Exception();

            return sizeVal;
        }
    }
}


