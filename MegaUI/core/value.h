#pragma once

#include <stdint.h>

#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "..\base\DynamicArray.h"

namespace YY
{
	namespace MegaUI
	{
		class Element;
		typedef DynamicArray<Element*> ElementList;

#define _MEGA_UI_VALUE_TPYE_MAP(_APPLY)                      \
        _APPLY(int32,       int32_t,            _int32Value) \
        _APPLY(boolean,     bool,               _boolValue)  \
        _APPLY(raw_ustring, raw_const_ustring,  _pszValue )  \
        _APPLY(POINT,       POINT,              _ptVal    )  \
        _APPLY(SIZE,        SIZE,               _sizeVal  )  \
        _APPLY(RECT,        RECT,               _rectVal  )  \
        _APPLY(Element,     Element*,           _peleValue)  \
        _APPLY(ElementList, ElementList*,       _peListVal)  \
        _APPLY(ATOM,        ATOM,               _atomVal  )  \
        _APPLY(HCURSOR,     HCURSOR,            _cursorVal)

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
			uint_t  _eType : 6;
			// 不要释放内部缓冲区
			uint_t _bSkipFree : 1;
#ifdef _WIN64
			uint_t   _cRef : 57;
#else
			uint_t   _cRef : 25;
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
				uint_t  _eType : 6;
				uint_t _bSkipFree : 1;
#ifdef _WIN64
				uint_t   _cRef : 57;
#else
				uint_t   _cRef : 25;
#endif
				_Type _Value;
			};

			template<>
			struct ConstValue<void>
			{
				uint_t  _eType : 6;
				uint_t _bSkipFree : 1;
#ifdef _WIN64
				uint_t   _cRef : 57;
#else
				uint_t   _cRef : 25;
#endif
			};

#define _APPLY(_eTYPE, _TYPE, _VAR) template<> struct ValueTypeToType<ValueType::_eTYPE> { using _Type = _TYPE; using ConstValue = ConstValue<_Type>; };
			template<ValueType eType>
			struct ValueTypeToType
			{
				using _Type = void;
				using ConstValue = ConstValue<_Type>;
			};
			_MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY

#define _DEFINE_CONST_VALUE(_VAR, _eTYPE, ...) static constexpr const ValueTypeToType<_eTYPE>::ConstValue _VAR = { (uint_t)_eTYPE, 1, uint_t(-1), __VA_ARGS__ }

#define _RETUNR_CONST_VALUE(_eTYPE, ...)               \
        _DEFINE_CONST_VALUE(Ret, _eTYPE, __VA_ARGS__); \
        return (Value*)&Ret;

		public:
			template<int32_t iValue>
			static Value* __fastcall GetInt32ConstValue()
			{
				_RETUNR_CONST_VALUE(ValueType::int32, iValue);
			}

			static Value* __fastcall GetAtomZero();
			static Value* __fastcall GetBoolFalse();
			static Value* __fastcall GetBoolTrue();
			static Value* __fastcall GetColorTrans();
			static Value* __fastcall GetCursorNull();
			static Value* __fastcall GetElListNull();
			static Value* __fastcall GetElementNull();
			static Value* __fastcall GetInt32Zero();
			static Value* __fastcall GetNull();
			static Value* __fastcall GetPointZero();
			static Value* __fastcall GetRectZero();
			static Value* __fastcall GetSizeZero();
			static Value* __fastcall GetStringNull();
			static Value* __fastcall GetUnavailable();
			static Value* __fastcall GetUnset();

			void __fastcall AddRef();
			void __fastcall Release();
			bool __fastcall IsEqual(Value* pv);
			ValueType __fastcall GetType();

			// Value creation methods
			static Value* __fastcall CreateInt32(int32_t iValue);
			static Value* __fastcall CreateBool(bool bValue);
			static Value* __fastcall CreateElementRef(Element* peValue);
			static Value* __fastcall CreateElementList(ElementList* peListValue);
			static Value* __fastcall CreateString(raw_const_ustring pszValue, HINSTANCE hResLoad = NULL);
			static Value* __fastcall CreatePoint(int32_t x, int32_t y);
			static Value* __fastcall CreateSize(int32_t cx, int32_t cy);
			static Value* __fastcall CreateRect(int32_t left, int32_t top, int32_t right, int32_t bottom);
			static Value* __fastcall CreateDFCFill(uint32_t uType, uint32_t uState);
			static Value* __fastcall CreateAtom(raw_const_ustring pszValue);
			static Value* __fastcall CreateCursor(raw_const_ustring pszValue);
			static Value* __fastcall CreateCursor(HCURSOR hValue);


			int32_t __fastcall GetInt32();
			bool __fastcall GetBool();
		};
	}
}