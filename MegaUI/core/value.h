#pragma once

#include <stdint.h>

#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "..\base\DynamicArray.h"
#include "..\base\StringBase.h"
#include "..\base\Color.h"
#include "..\base\Rect.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;
        typedef DynamicArray<Element*, true, false> ElementList;

        class Layout;
        class StyleSheet;

#define _MEGA_UI_VALUE_TPYE_MAP(_APPLY)                      \
        _APPLY(int32_t,     int32_t,             int32Value) \
        _APPLY(boolean,     bool,                boolValue)  \
        _APPLY(uString,     uString,             szValue  )  \
        _APPLY(POINT,       POINT,               ptVal    )  \
        _APPLY(SIZE,        SIZE,                sizeVal  )  \
        _APPLY(Rect,        Rect,                rectVal  )  \
        _APPLY(Element,     Element*,            pEleValue)  \
        _APPLY(ElementList, ElementList,         ListVal  )  \
        _APPLY(ATOM,        ATOM,                uAtomVal )  \
        _APPLY(HCURSOR,     HCURSOR,             hCursorVal) \
        _APPLY(Layout,      Layout*,             pLayout  )  \
        _APPLY(Color,       Color,               ColorValue) \
        _APPLY(StyleSheet,  StyleSheet*,         pStyleSheet)

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
        
        enum class ValueCmpOperation
        {
            Equal = 0,
            NotEqual,
            GreaterThan,
            GreaterThanOrEqual,
            LessThan,
            LessThanOrEqual,
        };

        template<typename _Type>
        struct ConstValueSharedData
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
        struct ConstValueSharedData<void>
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
            using SharedData = ConstValueSharedData<_Type>; \
        };

        template<ValueType eType>
        struct ValueTypeToType
        {
            using _Type = void;
            using SharedData = ConstValueSharedData<_Type>;
        };
        _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY

#define _DEFINE_CONST_VALUE(_VAR, _eTYPE, ...) static constexpr const ValueTypeToType<_eTYPE>::SharedData _VAR = {(uint_t)_eTYPE, 1, uint_t(-1), __VA_ARGS__}

#define _RETUNR_CONST_VALUE(_eTYPE, ...)               \
        _DEFINE_CONST_VALUE(Ret, _eTYPE, __VA_ARGS__); \
        return Value((Value::SharedData*)(&Ret));

		class Value
        {
        public:
            struct SharedData
            {
                union
                {
                    struct
                    {
                        uint_t eType : 6;
                        // 不要释放内部缓冲区
                        uint_t bSkipFree : 1;
#ifdef _WIN64
                        uint_t cRef : 57;
#else
                        uint_t cRef : 25;
#endif
                    };

                    uint_t uRawType;
                };

                union
                {
                    byte_t RawBuffer[1];
#define _APPLY(_eTYPE, _TYPE, _VAR) _TYPE _VAR;
                    _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY
                };

                SharedData() = delete;
                SharedData(const SharedData&) = delete;
                ~SharedData() = delete;

                SharedData& operator=(const SharedData&) = delete;


                void __MEGA_UI_API AddRef();
                void __MEGA_UI_API Release();
            };

        private:
            SharedData* pSharedData;

        public:
            Value(SharedData* _pSharedData = nullptr)
                : pSharedData(_pSharedData)
            {
            }

            Value(const Value& _Other)
                : pSharedData(_Other.pSharedData)
            {
                if (pSharedData)
                    pSharedData->AddRef();
            }

            Value(Value&& _Other) noexcept
                : pSharedData(_Other.pSharedData)
            {
                _Other.pSharedData = nullptr;
            }

            ~Value()
            {
                if (pSharedData)
                    pSharedData->Release();
            }
            
            Value& __MEGA_UI_API operator=(const Value& _Other);
            
            Value& __MEGA_UI_API operator=(Value&& _Other) noexcept;
            
            Value& __MEGA_UI_API operator=(std::nullptr_t);

            bool __MEGA_UI_API operator==(_In_ const Value& _Other) const;

            bool __MEGA_UI_API operator!=(_In_ const Value& _Other) const;

            bool __MEGA_UI_API operator==(std::nullptr_t) const;
            
            bool __MEGA_UI_API operator!=(std::nullptr_t) const;

            template<int32_t iValue>
            static Value __MEGA_UI_API GetInt32ConstValue()
            {
                _RETUNR_CONST_VALUE(ValueType::int32_t, iValue);
            }

            static Value __MEGA_UI_API GetAtomZero();
            static Value __MEGA_UI_API GetBoolFalse();
            static Value __MEGA_UI_API GetBoolTrue();
            //static _Ret_notnull_ Value* __MEGA_UI_API GetColorTrans();
            static Value __MEGA_UI_API GetCursorNull();
            static Value __MEGA_UI_API GetElListNull();
            static Value __MEGA_UI_API GetElementNull();
            static Value __MEGA_UI_API GetInt32Zero();
            static Value __MEGA_UI_API GetNull();
            static Value __MEGA_UI_API GetPointZero();
            static Value __MEGA_UI_API GetRectZero();
            static Value __MEGA_UI_API GetSizeZero();
            static Value __MEGA_UI_API GetStringNull();
            static Value __MEGA_UI_API GetUnavailable();
            static Value __MEGA_UI_API GetUnset();
            static Value __MEGA_UI_API GetLayoutNull();
            static Value __MEGA_UI_API GetColorTransparant();
            static Value __MEGA_UI_API GetSheetNull();


            ValueType __MEGA_UI_API GetType() const;

            bool __MEGA_UI_API HasValue() const;

            // Value creation methods
            static Value __MEGA_UI_API CreateInt32(_In_ int32_t _iValue);
            static Value __MEGA_UI_API CreateBool(_In_ bool _bValue);
            static Value __MEGA_UI_API CreateElementRef(_In_opt_ Element* _pValue);
            static Value __MEGA_UI_API CreateElementList(_In_ const ElementList& _pListValue);
            static Value __MEGA_UI_API CreateString(_In_ const uString& _szValue);
            //static _Ret_maybenull_ Value* __MEGA_UI_API CreateString(_In_ uint16_t _uId, _In_opt_ HINSTANCE _hResLoad = NULL);
            static Value __MEGA_UI_API CreatePoint(_In_ int32_t _iX, _In_ int32_t _iY);
            
            __inline static Value __MEGA_UI_API CreatePoint(_In_ POINT _Point)
            {
                return CreatePoint(_Point.x, _Point.y);
            }

            static Value __MEGA_UI_API CreateSize(_In_ int32_t _iCX, _In_ int32_t _iCY);

            __inline static Value __MEGA_UI_API CreateSize(_In_ SIZE _Size)
            {
                return CreateSize(_Size.cx, _Size.cy);
            }
            static Value __MEGA_UI_API CreateRect(_In_ int32_t _iLeft, _In_ int32_t _iTop, _In_ int32_t _iRight, _In_ int32_t _iBottom);
            
            __inline static Value __MEGA_UI_API CreateRect(_In_ const Rect& _Rect)
            {
                return CreateRect(_Rect.left, _Rect.top, _Rect.right, _Rect.bottom);
            }


            //static _Ret_maybenull_ Value* __MEGA_UI_API CreateDFCFill(_In_ uint32_t _uType, _In_ uint32_t _uState);
            static Value __MEGA_UI_API CreateAtom(_In_z_ raw_const_ustring_t _szValue);
            static Value __MEGA_UI_API CreateAtom(_In_ ATOM _uAtomValue);
            static Value __MEGA_UI_API CreateCursor(_In_z_ raw_const_ustring_t _szValue);
            static Value __MEGA_UI_API CreateCursor(_In_ HCURSOR _hCursorValue);
            
            static Value __MEGA_UI_API CreateColor(_In_ Color _Color);

            int32_t __MEGA_UI_API GetInt32() const;
            bool __MEGA_UI_API GetBool() const;
            SIZE __MEGA_UI_API GetSize() const;
            POINT __MEGA_UI_API GetPoint() const;
            uint8_t* __MEGA_UI_API GetRawBuffer();
            Color __MEGA_UI_API GetColor() const;
            Element* __MEGA_UI_API GetElement() const;
            uString __MEGA_UI_API GetString() const;

            bool __MEGA_UI_API CmpValue(const Value& _Other, ValueCmpOperation _Operation) const;
        };

        template<ValueType _eType>
        class ValueIs : public Value
        {
        private:
            constexpr static ValueType eType = _eType;
        public:
            using _Type = typename ValueTypeToType<eType>::_Type;

            ValueIs(const Value& _Other)
                : Value(_Other)
            {
            }

            ValueIs(Value&& _Other)
                : Value(std::move(_Other))
            {
            }

            bool __MEGA_UI_API HasValue()
            {
                return GetType() == eType;
            }

            _Type& __MEGA_UI_API GetValue()
            {
                if (!HasValue())
                {
                    throw Exception();
                }

                return *(_Type*)GetRawBuffer();
            }
        };

    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
