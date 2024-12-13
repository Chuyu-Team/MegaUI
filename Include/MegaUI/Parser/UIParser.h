#pragma once

#include <Base/YY.h>
#include <MegaUI/Window/WindowElement.h>
#include <Base/Containers/Array.h>
#include <Base/Containers/ArrayView.h>
#include <Base/Memory/RefPtr.h>

#include <MegaUI/Core/Property.h>
#include <MegaUI/Core/StyleSheet.h>

#pragma pack(push, __YY_PACKING)

namespace rapidxml
{
    template<class Ch>
    class xml_node;

    template<class Ch>
    class xml_attribute;
}

namespace YY
{
    namespace MegaUI
    {
        struct ExprNode;
        struct ParsedArg;

        struct UIParserPlayContext
        {
            int32_t iDPI = 96;
            // 可选，顶层控件，创建控件时使用，用于加速StartDefer
            Element* pTopElement = nullptr;
        };




        // 从 XML 或者 二进制XML反序列出 Element
        class UIParser
        {
        protected:
            struct UIParserRecorder
            {
                // 缩写为 resid
                u8String szResourceID;
                // 中间字节码，UIParser 会将 Xml中的内容转换为字节码，加速 Xml -> Element。
                // 使用者不需要关心字节码的实现。
                Array<uint8_t, AllocPolicy::SOO> ByteCode;
            };

            Array<Value> LocalValueCache;

            Array<UIParserRecorder, AllocPolicy::SOO> RecorderArray;
            
            Array<IControlInfo*> ControlInfoArray;
            Array<RefPtr<StyleSheet>> StyleSheets;

        public:
            UIParser() = default;

            ~UIParser() = default;

            void __YYAPI Clear();


            HRESULT __YYAPI ParserByXmlString(u8String&& _szXmlString);

            //HRESULT __YYAPI ParserByXmlString(u8StringView _szXmlString)
            //{
            //    u8String _szXmlStringBuffer;
            //    auto _hr = _szXmlStringBuffer.SetString(_szXmlString.GetConstString(), _szXmlString.GetSize());
            //    if (FAILED(_hr))
            //        return _hr;

            //    return ParserByXmlString(std::move(_szXmlStringBuffer));
            //}

            HRESULT __YYAPI Play(
                _In_ u8StringView _szResID,
                _In_opt_ UIParserPlayContext* _pContext,
                _Out_opt_ intptr_t* _pCooike,
                _Outptr_ WindowElement** _ppElement
                );

            HRESULT __YYAPI GetStyleSheet(_In_ u8StringView _szSheetResourceID, _Outptr_ StyleSheet** _ppSheet);

            Array<RefPtr<StyleSheet>> __YYAPI GetAllStyleSheet();

        protected:

            IControlInfo* __YYAPI FindControlInfo(_In_ u8StringView _szControlName, _Out_opt_ uint32_t* _pIndex = nullptr);

            const PropertyInfo* __YYAPI GetPropertyByName(_In_ IControlInfo* _pControlInfo, _In_ u8StringView _szPropName);

            HRESULT __YYAPI ParserElementNode(_In_ rapidxml::xml_node<u8char_t>* _pNote, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __YYAPI ParserElementProperty(_In_ rapidxml::xml_attribute<u8char_t>* _pAttribute, _In_ IControlInfo* _pControlInfo, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __YYAPI ParserValue(_In_ IControlInfo* _pControlInfo, _In_ const PropertyInfo* _pProp, _In_ u8StringView _szExpr, _Out_ Value* _pValue);

            static HRESULT __YYAPI ParserInt32Value(_In_ u8StringView _szValue, _Out_ ParsedArg* _pValue);

            static HRESULT __YYAPI ParserInt32Value(_In_ ExprNode* _pExprNode, _Out_ ParsedArg* _pValue);

            static HRESULT __YYAPI ParserInt32Value(_In_opt_ const EnumMap* pEnumMaps, _In_ ExprNode* _pExprNode, _Out_ Value* _pValue);
            
            static HRESULT __YYAPI ParserFloatValue(_In_ ExprNode* _pExprNode, _Out_ ParsedArg* _pValue);

            static HRESULT __YYAPI ParserFloatValue(_In_opt_ const EnumMap* pEnumMaps, _In_ ExprNode* _pExprNode, _Out_ Value* _pValue);

            static HRESULT __YYAPI ParserBoolValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __YYAPI ParserStringValue(ExprNode* _pExprNode, uString* _pValue);

            static HRESULT __YYAPI ParserStringValue(ExprNode* _pExprNode, Value* _pValue);
            
            static HRESULT __YYAPI ParserFunction(_In_ u8StringView _szFunctionName, _In_ ExprNode* _pExprNode, _Inout_cap_(_uArgCount) ParsedArg* _pArg, _In_ uint_t _uArgCount);
            
            template<uint_t _uArgCount>
            __inline static HRESULT ParserFunction(_In_ u8StringView _szFunctionName, _In_ ExprNode* _pExprNode, _Inout_ ParsedArg (&_Arg)[_uArgCount])
            {
                return ParserFunction(_szFunctionName, _pExprNode, _Arg, _uArgCount);
            }

            static HRESULT __YYAPI ParserPointValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __YYAPI ParserSizeValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __YYAPI ParserRectValue(ExprNode* _pExprNode, ParsedArg* _pValue);

            static HRESULT __YYAPI ParserRectValue(ExprNode* _pExprNode, Value* _pValue);
            
            static HRESULT __YYAPI ParserColorValue(ExprNode* _pExprNode, Color* _pValue);

            static HRESULT __YYAPI ParserColorValue(ExprNode* _pExprNode, Value* _pValue);

            HRESULT __YYAPI ParserStyleSheetValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __YYAPI ParserFontValue(ExprNode* _pExprNode, Value* _pValue);

            HRESULT __YYAPI Play(
                _In_ ArrayView<const uint8_t>& _ByteCode,
                _In_ UIParserPlayContext* _pContext,
                _In_opt_ Element* _pTopLevel,
                _Out_opt_ intptr_t* _pCooike,
                _Inout_ Array<Element*, AllocPolicy::SOO>* _ppElement
                );
            
            struct StyleSheetXmlOption
            {
                ValueCmpOperation Type = ValueCmpOperation::Invalid;
                u8StringView szPropName;
                u8StringView szPropValue;
            };

            /// <summary>
            /// 解析样式表，将结果设置到 _pStyleSheet
            /// </summary>
            /// <param name="_StyleSheetNode"></param>
            /// <param name="_XmlOption"></param>
            /// <param name="_pStyleSheet"></param>
            /// <returns>如果样式表是空的，那么返回 S_False。如果</returns>
            HRESULT __YYAPI ParserStyleSheetNode(_In_ rapidxml::xml_node<u8char_t>* _StyleSheetNode, Array<StyleSheetXmlOption, AllocPolicy::SOO>& _XmlOption, _Inout_ StyleSheet* _pStyleSheet);
            
            HRESULT __YYAPI ParserStyleSheetElementNode(_In_ rapidxml::xml_node<u8char_t>* _pElementValueNode, const Array<StyleSheetXmlOption, AllocPolicy::SOO>& _XmlOption, _Inout_ StyleSheet* _pStyleSheet);

        };
    }
} // namespace YY

#pragma pack(pop)
