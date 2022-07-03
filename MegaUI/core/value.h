#pragma once

#include <stdint.h>

#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "..\base\DynamicArray.h"
#include "..\base\StringBase.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;
        typedef DynamicArray<Element*, true, false> ElementList;

#define _MEGA_UI_VALUE_TPYE_MAP(_APPLY)                      \
        _APPLY(int32_t,     int32_t,             int32Value) \
        _APPLY(boolean,     bool,                boolValue)  \
        _APPLY(uString,     uString,             szValue  )  \
        _APPLY(POINT,       POINT,               ptVal    )  \
        _APPLY(SIZE,        SIZE,                sizeVal  )  \
        _APPLY(RECT,        RECT,                rectVal  )  \
        _APPLY(Element,     Element*,            peleValue)  \
        _APPLY(ElementList, ElementList*,        peListVal)  \
        _APPLY(ATOM,        ATOM,                atomVal  )  \
        _APPLY(HCURSOR,     HCURSOR,             cursorVal)

		enum class ValueType
        {
            // 此值不可用
            Unavailable = -2,
            // 尚未设置
            Unset       = -1,
            // 什么也没有
            Null        = 0,

#define _APPLY(_eTYPE, _TYPE, _VAR) _eTYPE,
            _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY
        };

		class Value
        {
        private:
            uint_t eType : 6;
            // 不要释放内部缓冲区
            uint_t bSkipFree : 1;
#ifdef _WIN64
            uint_t cRef : 57;
#else
            uint_t cRef : 25;
#endif
            union
            {
#define _APPLY(_eTYPE, _TYPE, _VAR) _TYPE _VAR;
                _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY
            };

            template<typename _Type>
            struct ConstValue
            {
                uint_t eType : 6;
                uint_t bSkipFree : 1;
#ifdef _WIN64
                uint_t cRef : 57;
#else
                uint_t cRef : 25;
#endif
                _Type Value;
            };

            template<>
            struct ConstValue<void>
            {
                uint_t eType : 6;
                uint_t bSkipFree : 1;
#ifdef _WIN64
                uint_t cRef : 57;
#else
                uint_t cRef : 25;
#endif
            };

#define _APPLY(_eTYPE, _TYPE, _VAR)           \
    template<>                                \
    struct ValueTypeToType<ValueType::_eTYPE> \
    {                                         \
        using _Type = _TYPE;                  \
        using ConstValue = ConstValue<_Type>; \
    };
            template<ValueType eType>
            struct ValueTypeToType
            {
                using _Type = void;
                using ConstValue = ConstValue<_Type>;
            };
            _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY

#define _DEFINE_CONST_VALUE(_VAR, _eTYPE, ...) static constexpr const ValueTypeToType<_eTYPE>::ConstValue _VAR = {(uint_t)_eTYPE, 1, uint_t(-1), __VA_ARGS__}

#define _RETUNR_CONST_VALUE(_eTYPE, ...)           \
    _DEFINE_CONST_VALUE(Ret, _eTYPE, __VA_ARGS__); \
    return (Value*)&Ret;

        public:
            Value() = delete;
            ~Value() = delete;

            template<int32_t iValue>
            static _Ret_notnull_ Value* __fastcall GetInt32ConstValue()
            {
                _RETUNR_CONST_VALUE(ValueType::int32_t, iValue);
            }

            static _Ret_notnull_ Value* __fastcall GetAtomZero();
            static _Ret_notnull_ Value* __fastcall GetBoolFalse();
            static _Ret_notnull_ Value* __fastcall GetBoolTrue();
            static _Ret_notnull_ Value* __fastcall GetColorTrans();
            static _Ret_notnull_ Value* __fastcall GetCursorNull();
            static _Ret_notnull_ Value* __fastcall GetElListNull();
            static _Ret_notnull_ Value* __fastcall GetElementNull();
            static _Ret_notnull_ Value* __fastcall GetInt32Zero();
            static _Ret_notnull_ Value* __fastcall GetNull();
            static _Ret_notnull_ Value* __fastcall GetPointZero();
            static _Ret_notnull_ Value* __fastcall GetRectZero();
            static _Ret_notnull_ Value* __fastcall GetSizeZero();
            static _Ret_notnull_ Value* __fastcall GetStringNull();
            static _Ret_notnull_ Value* __fastcall GetUnavailable();
            static _Ret_notnull_ Value* __fastcall GetUnset();

            void __fastcall AddRef();
            void __fastcall Release();
            bool __fastcall IsEqual(_In_ Value* pValue);
            ValueType __fastcall GetType();

            // Value creation methods
            static _Ret_maybenull_ Value* __fastcall CreateInt32(_In_ int32_t _iValue);
            static _Ret_notnull_   Value* __fastcall CreateBool(_In_ bool _bValue);
            static _Ret_maybenull_ Value* __fastcall CreateElementRef(_In_opt_ Element* _pValue);
            static _Ret_maybenull_ Value* __fastcall CreateElementList(_In_opt_ ElementList* _pListValue);
            static _Ret_maybenull_ Value* __fastcall CreateString(_In_ uString _szValue);
            static _Ret_maybenull_ Value* __fastcall CreateString(_In_ uint16_t _uId, _In_opt_ HINSTANCE _hResLoad = NULL);
            static _Ret_maybenull_ Value* __fastcall CreatePoint(_In_ int32_t _iX, _In_ int32_t _iY);
            static _Ret_maybenull_ Value* __fastcall CreateSize(_In_ int32_t _iCX, _In_ int32_t _iCY);
            static _Ret_maybenull_ Value* __fastcall CreateRect(_In_ int32_t _iLeft, _In_ int32_t _iTop, _In_ int32_t _iRight, _In_ int32_t _iBottom);
            static _Ret_maybenull_ Value* __fastcall CreateDFCFill(_In_ uint32_t _uType, _In_ uint32_t _uState);
            static _Ret_maybenull_ Value* __fastcall CreateAtom(_In_z_ raw_const_ustring_t _szValue);
            static _Ret_maybenull_ Value* __fastcall CreateCursor(_In_z_ raw_const_ustring_t _szValue);
            static _Ret_maybenull_ Value* __fastcall CreateCursor(_In_ HCURSOR hValue);

            int32_t __fastcall GetInt32();
            bool __fastcall GetBool();
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
