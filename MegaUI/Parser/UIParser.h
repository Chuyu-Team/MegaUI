#pragma once

#include <MegaUI/base/MegaUITypeInt.h>
#include <MegaUI/Window/WindowElement.h>
#include <MegaUI/base/DynamicArray.h>
#include <MegaUI/base/ArrayView.h>
#include <MegaUI/core/Property.h>

#pragma pack(push, __MEGA_UI_PACKING)

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
        private:
            struct UIParserRecorder
            {
                // 缩写为 resid
                u8String szResourceID;
                // 中间字节码，UIParser 会将 Xml中的内容转换为字节码，加速 Xml -> Element。
                // 使用者不需要关心字节码的实现。
                DynamicArray<uint8_t, false, false> ByteCode;
            };

            DynamicArray<Value> LocalValueCache;

            DynamicArray<UIParserRecorder> RecorderArray;
            
            DynamicArray<IControlInfo*> ControlInfoArray;
            DynamicArray<StyleSheet*> StyleSheets;
        public:
            void __MEGA_UI_API Clear();


            HRESULT __MEGA_UI_API ParserByXmlString(u8String&& _szXmlString);

            //HRESULT __MEGA_UI_API ParserByXmlString(u8StringView _szXmlString)
            //{
            //    u8String _szXmlStringBuffer;
            //    auto _hr = _szXmlStringBuffer.SetString(_szXmlString.GetConstString(), _szXmlString.GetSize());
            //    if (FAILED(_hr))
            //        return _hr;

            //    return ParserByXmlString(std::move(_szXmlStringBuffer));
            //}

            HRESULT __MEGA_UI_API Play(
                _In_ u8StringView _szResID,
                _In_opt_ UIParserPlayContext* _pContext,
                _Out_opt_ intptr_t* _pCooike,
                _Outptr_ WindowElement** _ppElement
                );

            
        protected:

            IControlInfo* __MEGA_UI_API FindControlInfo(_In_ raw_const_astring_t _szControlName, _Out_opt_ uint32_t* _pIndex = nullptr);

            const PropertyInfo* __MEGA_UI_API GetPropertyByName(_In_ IControlInfo* _pControlInfo, _In_ raw_const_astring_t _szPropName);

            HRESULT __MEGA_UI_API ParserElementNode(_In_ rapidxml::xml_node<char>* _pNote, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __MEGA_UI_API ParserElementProperty(_In_ rapidxml::xml_attribute<char>* _pAttribute, _In_ IControlInfo* _pControlInfo, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __MEGA_UI_API ParserValue(_In_ IControlInfo* _pControlInfo, _In_ const PropertyInfo* _pProp, _In_ u8StringView _szExpr, _Out_ Value* _pValue);

            static HRESULT __MEGA_UI_API ParserInt32Value(_In_ const u8StringView& _szValue, _Out_ ParsedArg* _pValue);

            static HRESULT __MEGA_UI_API ParserInt32Value(_In_ ExprNode* _pExprNode, _Out_ ParsedArg* _pValue);

            static HRESULT __MEGA_UI_API ParserInt32Value(_In_opt_ const EnumMap* pEnumMaps, _In_ ExprNode* _pExprNode, _Out_ Value* _pValue);
            
            static HRESULT __MEGA_UI_API ParserFloatValue(_In_ ExprNode* _pExprNode, _Out_ ParsedArg* _pValue);

            static HRESULT __MEGA_UI_API ParserFloatValue(_In_opt_ const EnumMap* pEnumMaps, _In_ ExprNode* _pExprNode, _Out_ Value* _pValue);

            static HRESULT __MEGA_UI_API ParserBoolValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserStringValue(ExprNode* _pExprNode, uString* _pValue);

            static HRESULT __MEGA_UI_API ParserStringValue(ExprNode* _pExprNode, Value* _pValue);
            
            static HRESULT __MEGA_UI_API ParserFunction(_In_ aStringView _szFunctionName, _In_ ExprNode* _pExprNode, _Inout_cap_(_uArgCount) ParsedArg* _pArg, _In_ uint_t _uArgCount);
            
            template<uint_t _uArgCount>
            __inline static HRESULT ParserFunction(_In_ aStringView _szFunctionName, _In_ ExprNode* _pExprNode, _Inout_ ParsedArg (&_Arg)[_uArgCount])
            {
                return ParserFunction(_szFunctionName, _pExprNode, _Arg, _uArgCount);
            }

            static HRESULT __MEGA_UI_API ParserPointValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserSizeValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserRectValue(ExprNode* _pExprNode, ParsedArg* _pValue);

            static HRESULT __MEGA_UI_API ParserRectValue(ExprNode* _pExprNode, Value* _pValue);
            
            static HRESULT __MEGA_UI_API ParserColorValue(ExprNode* _pExprNode, Color* _pValue);

            static HRESULT __MEGA_UI_API ParserColorValue(ExprNode* _pExprNode, Value* _pValue);

            HRESULT __MEGA_UI_API ParserStyleSheetValue(ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserFontValue(ExprNode* _pExprNode, Value* _pValue);

            HRESULT __MEGA_UI_API Play(
                _In_ ArrayView<uint8_t>& _ByteCode,
                _In_ UIParserPlayContext* _pContext,
                _Out_opt_ intptr_t* _pCooike,
                _Inout_ DynamicArray<Element*, false, false>* _ppElement
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
            HRESULT __MEGA_UI_API ParserStyleSheetNode(_In_ rapidxml::xml_node<char>* _StyleSheetNode, DynamicArray<StyleSheetXmlOption, false, false>& _XmlOption, _Inout_ StyleSheet* _pStyleSheet);
            
            HRESULT __MEGA_UI_API ParserStyleSheetElementNode(_In_ rapidxml::xml_node<char>* _pElementValueNode, const DynamicArray<StyleSheetXmlOption, false, false>& _XmlOption, _Inout_ StyleSheet* _pStyleSheet);

        };
    }
} // namespace YY

#pragma pack(pop)
