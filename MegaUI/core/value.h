﻿#pragma once

#include <stdint.h>

#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "..\base\DynamicArray.h"
#include "..\base\StringBase.h"
#include "..\base\Color.h"
#include "..\base\Rect.h"
#include <MegaUI/base/Font.h>
#include <MegaUI/base/Size.h>
#include <MegaUI/base/Point.h>

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;
        typedef DynamicArray<Element*, true, false> ElementList;

        class Layout;
        class StyleSheet;
        class UIParser;

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
        _APPLY(StyleSheet,  StyleSheet*,         pStyleSheet) \
        _APPLY(Font,        Font,                FontValue)

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
        
        enum class ValueSuffixType
        {
            None,
            // 相对像素（缩写 rp）
            RelativePixel,
            // 字体的点数，也称呼为磅（缩写 pt）
            FontPoint,
        };

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

#define _DEFINE_CONST_VALUE(_VAR, _eTYPE, ...) static constexpr const ValueTypeToType<_eTYPE>::SharedData _VAR = {(uint_t)_eTYPE, 1, uint_t(-1), __VA_ARGS__}

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

                uint8_t SuffixType[4];

                SharedData() = delete;
                SharedData(const SharedData&) = delete;
                ~SharedData() = delete;

                SharedData& operator=(const SharedData&) = delete;


                void __MEGA_UI_API AddRef();
                void __MEGA_UI_API Release();

                bool __MEGA_UI_API IsReadOnly();

                /// <summary>
                /// 判断内容是否是相对大小，而需要计算
                /// </summary>
                /// <returns></returns>
                bool __MEGA_UI_API NeedCalculate();
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
            static Value __MEGA_UI_API CreateInt32()
            {
                _RETUNR_CONST_VALUE(ValueType::int32_t, iValue);
            }
            
            template<int32_t iValue>
            static Value __MEGA_UI_API CreateFloat()
            {
                _RETUNR_CONST_VALUE(ValueType::float_t, (float)iValue);
            }

            static Value __MEGA_UI_API CreateAtomZero();
            static Value __MEGA_UI_API CreateBoolFalse();
            static Value __MEGA_UI_API CreateBoolTrue();
            //static _Ret_notnull_ Value* __MEGA_UI_API GetColorTrans();
            static Value __MEGA_UI_API CreateCursorNull();
            static Value __MEGA_UI_API CreateEmptyElementList();
            static Value __MEGA_UI_API CreateElementNull();
            static Value __MEGA_UI_API CreateInt32Zero();
            static Value __MEGA_UI_API CreateNull();
            static Value __MEGA_UI_API CreatePointZero();
            static Value __MEGA_UI_API CreateRectZero();
            static Value __MEGA_UI_API CreateSizeZero();
            static Value __MEGA_UI_API CreateEmptyString();
            static Value __MEGA_UI_API CreateUnavailable();
            static Value __MEGA_UI_API CreateUnset();
            static Value __MEGA_UI_API CreateLayoutNull();

            /// <summary>
            /// 获取默认字体信息。
            /// </summary>
            /// <returns></returns>
            static Value __MEGA_UI_API CreateDefaultFont();

            static Value __MEGA_UI_API CreateFont(uString _szFace, float _uFontSize, uint32_t _uWeight, uint32_t _fStyle, Color _Color);

            /// <summary>
            /// 创建一个全透明的颜色。
            /// </summary>
            /// <returns></returns>
            static Value __MEGA_UI_API CreateColorTransparant();
            static Value __MEGA_UI_API CreateSheetNull();


            ValueType __MEGA_UI_API GetType() const;

            /// <summary>
            /// 判断是否包含有效的内容，除Unavailable、Unset以及Null外均认为有效。
            /// </summary>
            /// <returns></returns>
            bool __MEGA_UI_API HasValue() const;

            static Value __MEGA_UI_API CreateInt32(_In_ int32_t _iValue);
            
            static Value __MEGA_UI_API CreateFloat(_In_ float _iValue);
         
            static Value __MEGA_UI_API CreateBool(_In_ bool _bValue);
            static Value __MEGA_UI_API CreateElementRef(_In_opt_ Element* _pValue);
            static Value __MEGA_UI_API CreateElementList(_In_ const ElementList& _pListValue);
            static Value __MEGA_UI_API CreateString(_In_ const uString& _szValue);
            //static _Ret_maybenull_ Value* __MEGA_UI_API CreateString(_In_ uint16_t _uId, _In_opt_ HINSTANCE _hResLoad = NULL);
            static Value __MEGA_UI_API CreatePoint(_In_ float _iX, _In_ float _iY);
            
            __inline static Value __MEGA_UI_API CreatePoint(_In_ Point _Point)
            {
                return CreatePoint(_Point.X, _Point.Y);
            }

            static Value __MEGA_UI_API CreateSize(_In_ float _iCX, _In_ float _iCY);

            __inline static Value __MEGA_UI_API CreateSize(_In_ Size _Size)
            {
                return CreateSize(_Size.Width, _Size.Height);
            }

            static Value __MEGA_UI_API CreateRect(_In_ float _iLeft, _In_ float _iTop, _In_ float _iRight, _In_ float _iBottom);
            
            __inline static Value __MEGA_UI_API CreateRect(_In_ const Rect& _Rect)
            {
                return CreateRect(_Rect.Left, _Rect.Top, _Rect.Right, _Rect.Bottom);
            }
            
            //static _Ret_maybenull_ Value* __MEGA_UI_API CreateDFCFill(_In_ uint32_t _uType, _In_ uint32_t _uState);
            static Value __MEGA_UI_API CreateAtom(_In_z_ raw_const_ustring_t _szValue);
            static Value __MEGA_UI_API CreateAtom(_In_ ATOM _uAtomValue);
            static Value __MEGA_UI_API CreateCursor(_In_z_ raw_const_ustring_t _szValue);
            static Value __MEGA_UI_API CreateCursor(_In_ HCURSOR _hCursorValue);
            
            static Value __MEGA_UI_API CreateColor(_In_ Color _Color);
            
            static Value __MEGA_UI_API CreateStyleSheet(_In_ StyleSheet* _pStyleSheet);

            int32_t __MEGA_UI_API GetInt32() const;

            float __MEGA_UI_API GetFloat() const;

            bool __MEGA_UI_API GetBool() const;
            Size __MEGA_UI_API GetSize() const;
            Point __MEGA_UI_API GetPoint() const;
            uint8_t* __MEGA_UI_API GetRawBuffer();
            Color __MEGA_UI_API GetColor() const;
            Element* __MEGA_UI_API GetElement() const;
            uString __MEGA_UI_API GetString() const;

            StyleSheet* __MEGA_UI_API GetStyleSheet() const;

            Font& __MEGA_UI_API GetFont() const;

            Rect& __MEGA_UI_API GetRect() const;

            bool __MEGA_UI_API CmpValue(const Value& _Other, ValueCmpOperation _Operation) const;

            Value __MEGA_UI_API MakeSureValue(_In_ Element* _pElement) const;
            
        protected:
            static Value __MEGA_UI_API CreateFloat(_In_ float _iValue, _In_ uint8_t _SuffixType);

            static Value __MEGA_UI_API CreateSize(_In_ float _iCX, _In_ float _iCY, _In_reads_opt_(2) uint8_t* _pSuffixType);
            
            static Value __MEGA_UI_API CreateRect(_In_ float _iLeft, _In_ float _iTop, _In_ float _iRight, _In_ float _iBottom, _In_reads_opt_(4) uint8_t* _pSuffixType);

            static Value __MEGA_UI_API CreateFont(uString _szFace, float _uFontSize, uint8_t _FontSizeSuffixType, uint32_t _uWeight, uint32_t _fStyle, Color _Color);
        };

        float __MEGA_UI_API RelativePixelToPixel(float _iRelativePixel, int32_t _DPI);

        float __MEGA_UI_API PointToPixel(float _iFontPoint, int32_t _DPI);

        Rect __MEGA_UI_API RelativePixelToPixel(Rect _iRelativePixelRect, int32_t _DPI);

        Size __MEGA_UI_API RelativePixelToPixel(Size _iRelativePixelSize, int32_t _DPI);

        float __MEGA_UI_API UpdatePixel(float _iOldPixel, int32_t _OldDPI, int32_t _NewDPI);

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
