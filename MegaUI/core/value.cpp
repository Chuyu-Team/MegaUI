#include "pch.h"
#include "value.h"

namespace YY
{
    namespace MegaUI
    {
        Value* __fastcall Value::GetAtomZero()
        {
            _RETUNR_CONST_VALUE(ValueType::eAtom, 0);
        }

        Value* __fastcall Value::GetIntZero()
        {
            return GetIntConstValue<0>();
        }

        Value* __fastcall Value::GetBoolFalse()
        {
            _RETUNR_CONST_VALUE(ValueType::eBool, false);
        }

        Value* __fastcall Value::GetBoolTrue()
        {
            _RETUNR_CONST_VALUE(ValueType::eBool, true);
        }

        Value* __fastcall Value::GetCursorNull()
        {
            _RETUNR_CONST_VALUE(ValueType::eCursor, NULL);
        }

        Value* __fastcall Value::GetElListNull()
        {
            _RETUNR_CONST_VALUE(ValueType::eElementList, nullptr);
        }

        Value* __fastcall Value::GetElementNull()
        {
            _RETUNR_CONST_VALUE(ValueType::eElement, nullptr);
        }

        Value* __fastcall Value::GetNull()
        {
            _RETUNR_CONST_VALUE(ValueType::eNull);
        }

        Value* __fastcall Value::GetPointZero()
        {
            _RETUNR_CONST_VALUE(ValueType::ePoint, { 0, 0 });
        }

        Value* __fastcall Value::GetRectZero()
        {
            _RETUNR_CONST_VALUE(ValueType::eRect, { 0, 0, 0, 0 });
        }

        Value* __fastcall Value::GetSizeZero()
        {
            _RETUNR_CONST_VALUE(ValueType::eSize, { 0, 0 });
        }

        Value* __fastcall Value::GetStringNull()
        {
            _RETUNR_CONST_VALUE(ValueType::eString, nullptr);
        }

        Value* __fastcall Value::GetUnavailable()
        {
            _RETUNR_CONST_VALUE(ValueType::eUnavailable);
        }

        Value* __fastcall Value::GetUnset()
        {
            _RETUNR_CONST_VALUE(ValueType::eUnset);
        }
    }
}


