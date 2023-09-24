#pragma once

#include <stdint.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Base/YY.h>
#include <Base/Containers/Array.h>
#include <Base/Strings/String.h>
#include <Media/Color.h>
#include <Media/Rect.h>
#include <Media/Font.h>
#include <Media/Size.h>
#include <Media/Point.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;
        typedef Array<Element*> ElementList;

        class Layout;
        class StyleSheet;
        class UIParser;

#ifndef _WIN32
        typedef uint16_t ATOM;
        typedef void* HCURSOR;
#endif

#define _MEGA_UI_VALUE_TPYE_MAP(_APPLY)                      \
        _APPLY(int32_t,     int32_t,             int32Value) \
        _APPLY(float_t,     float,               floatValue) \
        _APPLY(boolean,     bool,                boolValue)  \
        _APPLY(uString,     uString,             szValue  )  \
        _APPLY(Point,       Point,               ptVal    )  \
        _APPLY(Size,        Size,                sizeVal  )  \
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
        
        enum class ValueSuffixType : uint16_t
        {
            None,
            // 设备无关像素（缩写px），等价于 None
            Pixel = None,
            // 设备相关像素（缩写 dp）, px = dp * dpi / 96
            DevicePixel,
            // 字体的点数，也称呼为磅（缩写 pt），px = pt * dpi / 72
            FontPoint,
        };

        // 一共4组位置信息，这是为了适应 Rect 这样的拥有4个成员的情况
        union ValueSuffix
        {
            struct
            {
                ValueSuffixType Type1 : 4;
                ValueSuffixType Type2 : 4;
                ValueSuffixType Type3 : 4;
                ValueSuffixType Type4 : 4;
                // 当前数值的Dpi
                uint16_t Dpi;
            };
            
            uint32_t RawView;
        };
        
        static_assert(sizeof(ValueSuffix) == sizeof(uint32_t), "");

        enum class ValueCmpOperation
        {
            Invalid = 0,
            Equal,
            NotEqual,
            GreaterThan,
            GreaterThanOrEqual,
            LessThan,
            LessThanOrEqual,
        };

        template<typename _Type>
        struct ConstValueSharedData
        {
            int_t eType : 6;
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
            int_t eType : 6;
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

#define _DEFINE_CONST_VALUE(_VAR, _eTYPE, ...) static constexpr const ValueTypeToType<_eTYPE>::SharedData _VAR = {(int_t)_eTYPE, uint_t(1), uint_t(-1), __VA_ARGS__}

#define _RETUNR_CONST_VALUE(_eTYPE, ...)               \
        _DEFINE_CONST_VALUE(Ret, _eTYPE, __VA_ARGS__); \
        return Value((Value::SharedData*)(&Ret));

		class Value
        {
            friend UIParser;

        public:
            struct SharedData
            {
                union
                {
                    struct
                    {
                        int_t eType : 6;
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
                    uint8_t RawBuffer[1];
#define _APPLY(_eTYPE, _TYPE, _VAR) _TYPE _VAR;
                    _MEGA_UI_VALUE_TPYE_MAP(_APPLY)
#undef _APPLY
                };

                ValueSuffix SuffixType;

                SharedData() = delete;
                SharedData(const SharedData&) = delete;
                ~SharedData() = delete;

                SharedData& operator=(const SharedData&) = delete;


                void __YYAPI AddRef();
                void __YYAPI Release();

                bool __YYAPI IsReadOnly();

                /// <summary>
                /// 判断内容是否是相对大小，而需要计算
                /// </summary>
                /// <returns></returns>
                bool __YYAPI NeedCalculate();

                int32_t __YYAPI GetDpi();
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
            
            Value& __YYAPI operator=(const Value& _Other);
            
            Value& __YYAPI operator=(Value&& _Other) noexcept;
            
            Value& __YYAPI operator=(std::nullptr_t);

            bool __YYAPI operator==(_In_ const Value& _Other) const;

            bool __YYAPI operator!=(_In_ const Value& _Other) const;

            bool __YYAPI operator==(std::nullptr_t) const;
            
            bool __YYAPI operator!=(std::nullptr_t) const;

            template<int32_t iValue>
            static Value __YYAPI CreateInt32()
            {
                _RETUNR_CONST_VALUE(ValueType::int32_t, iValue);
            }
            
            template<int32_t iValue>
            static Value __YYAPI CreateFloat()
            {
                _RETUNR_CONST_VALUE(ValueType::float_t, (float)iValue);
            }

            static Value __YYAPI CreateAtomZero();
            static Value __YYAPI CreateBoolFalse();
            static Value __YYAPI CreateBoolTrue();
            //static _Ret_notnull_ Value* __YYAPI GetColorTrans();
            static Value __YYAPI CreateCursorNull();
            static Value __YYAPI CreateEmptyElementList();
            static Value __YYAPI CreateElementNull();
            static Value __YYAPI CreateInt32Zero();
            static Value __YYAPI CreateNull();
            static Value __YYAPI CreatePointZero();
            static Value __YYAPI CreateRectZero();
            static Value __YYAPI CreateSizeZero();
            static Value __YYAPI CreateEmptyString();
            static Value __YYAPI CreateUnavailable();
            static Value __YYAPI CreateUnset();
            static Value __YYAPI CreateLayoutNull();

            /// <summary>
            /// 获取默认字体信息。
            /// </summary>
            /// <returns></returns>
            static Value __YYAPI CreateDefaultFontFamily();

            /// <summary>
            /// 创建一个全透明的颜色。
            /// </summary>
            /// <returns></returns>
            static Value __YYAPI CreateColorTransparant();
            static Value __YYAPI CreateSheetNull();


            ValueType __YYAPI GetType() const;

            /// <summary>
            /// 判断是否包含有效的内容，除Unavailable、Unset以及Null外均认为有效。
            /// </summary>
            /// <returns></returns>
            bool __YYAPI HasValue() const;

            static Value __YYAPI CreateInt32(_In_ int32_t _iValue);
            
            static Value __YYAPI CreateFloat(_In_ float _iValue, _In_ ValueSuffix _Suffix = {});
         
            static Value __YYAPI CreateBool(_In_ bool _bValue);
            static Value __YYAPI CreateElementRef(_In_opt_ Element* _pValue);
            static Value __YYAPI CreateElementList(_In_ const ElementList& _pListValue);
            static Value __YYAPI CreateString(_In_ const uString& _szValue);
            //static _Ret_maybenull_ Value* __YYAPI CreateString(_In_ uint16_t _uId, _In_opt_ HINSTANCE _hResLoad = NULL);
            static Value __YYAPI CreatePoint(_In_ float _iX, _In_ float _iY);
            
            __inline static Value __YYAPI CreatePoint(_In_ Point _Point)
            {
                return CreatePoint(_Point.X, _Point.Y);
            }

            static Value __YYAPI CreateSize(_In_ float _iCX, _In_ float _iCY, _In_ ValueSuffix _Suffix = {});

            __inline static Value __YYAPI CreateSize(_In_ Size _Size, _In_ ValueSuffix _Suffix = {})
            {
                return CreateSize(_Size.Width, _Size.Height, _Suffix);
            }

            static Value __YYAPI CreateRect(
                _In_ float _iLeft,
                _In_ float _iTop,
                _In_ float _iRight,
                _In_ float _iBottom,
                _In_ ValueSuffix _Suffix = {});
            
            __inline static Value __YYAPI CreateRect(_In_ const Rect& _Rect, _In_ ValueSuffix _Suffix = {})
            {
                return CreateRect(_Rect.Left, _Rect.Top, _Rect.Right, _Rect.Bottom, _Suffix);
            }
            
            //static _Ret_maybenull_ Value* __YYAPI CreateDFCFill(_In_ uint32_t _uType, _In_ uint32_t _uState);
            static Value __YYAPI CreateAtom(_In_z_ raw_const_ustring_t _szValue);
            static Value __YYAPI CreateAtom(_In_ ATOM _uAtomValue);
            static Value __YYAPI CreateCursor(_In_z_ raw_const_ustring_t _szValue);
            static Value __YYAPI CreateCursor(_In_ HCURSOR _hCursorValue);
            
            static Value __YYAPI CreateColor(_In_ Color _Color);
            
            static Value __YYAPI CreateStyleSheet(_In_ StyleSheet* _pStyleSheet);

            int32_t __YYAPI GetInt32() const;

            float __YYAPI GetFloat() const;

            bool __YYAPI GetBool() const;
            Size __YYAPI GetSize() const;
            Point __YYAPI GetPoint() const;
            uint8_t* __YYAPI GetRawBuffer();
            Color __YYAPI GetColor() const;
            Element* __YYAPI GetElement() const;

            ElementList __YYAPI GetElementList() const;

            uString __YYAPI GetString() const;

            StyleSheet* __YYAPI GetStyleSheet() const;

            Rect& __YYAPI GetRect() const;

            bool __YYAPI CmpValue(const Value& _Other, ValueCmpOperation _Operation, bool _bIgnoreDpi = true) const;

            Value __YYAPI UpdateDpi(_In_ int32_t _iNewDpi) const;

            /// <summary>
            /// 判断二个Value是否是同一个变量。
            /// </summary>
            bool __YYAPI IsSame(const Value& _Other) const;
        };

        float __YYAPI UpdateDpi(_In_ float _iValue, _In_ int32_t _iOldDpi, _In_ int32_t _iNewDpi, _In_ ValueSuffixType _Type);
        
        Rect __YYAPI UpdateDpi(_In_ Rect _Rect, _In_ int32_t _iNewDpi, _In_ ValueSuffix _Suffix);

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

            bool __YYAPI HasValue()
            {
                return GetType() == eType;
            }

            _Type& __YYAPI GetValue()
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
