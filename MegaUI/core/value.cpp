#include "pch.h"
#include "value.h"

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
            _RETUNR_CONST_VALUE(ValueType::ElementList, nullptr);
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
            _RETUNR_CONST_VALUE(ValueType::raw_ustring, nullptr);
        }

        Value* __fastcall Value::GetUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::Unavailable);
        }

        Value* __fastcall Value::GetUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::Unset);
        }
    }
}


