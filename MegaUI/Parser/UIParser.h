#pragma once

#include "../base/MegaUITypeInt.h"
#include "../Window/WindowElement.h"
#include "../base/DynamicArray.h"
#include "../base/ArrayView.h"

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

        struct UIParserRecorder
        {
            // 缩写为 resid
            u8String szResourceID;
            // 中间字节码，UIParser 会将 Xml中的内容转换为字节码，加速 Xml -> Element。
            // 使用者不需要关心字节码的实现。
            DynamicArray<uint8_t, false, false> ByteCode;

            //UIParserRecorder()
            //{
            //}

            //UIParserRecorder(const UIParserRecorder&) = default;

            //UIParserRecorder(UIParserRecorder&&) = default;

        };
        
        union ParsedArg
        {
            // , 跳过
            // * 忽略
            // I
            int32_t iNumber;
            // S
            u8StringView szString;
            // C
            Color cColor;

            ParsedArg()
            {
            }
        };

        struct StyleSheetXmlOption
        {
            ValueCmpOperation Type;
            u8StringView szPropName;
            u8StringView szPropValue;
        };


        // 从 XML 或者 二进制XML反序列出 Element
        class UIParser
        {
        private:
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
                _In_opt_ Element* _pTopElement,
                _Out_opt_ intptr_t* _pCooike,
                _Outptr_ WindowElement** _ppElement
                );
        protected:

            IControlInfo* __MEGA_UI_API FindControlInfo(_In_ raw_const_astring_t _szControlName, _Out_opt_ uint32_t* _pIndex = nullptr);

            const PropertyInfo* __MEGA_UI_API GetPropertyByName(_In_ IControlInfo* _pControlInfo, _In_ raw_const_astring_t _szPropName);

            HRESULT __MEGA_UI_API ParserElementNode(_In_ rapidxml::xml_node<char>* _pNote, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __MEGA_UI_API ParserElementProperty(_In_ rapidxml::xml_attribute<char>* _pAttribute, _In_ IControlInfo* _pControlInfo, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __MEGA_UI_API ParserValue(_In_ IControlInfo* _pControlInfo, _In_ const PropertyInfo* _pProp, _In_ u8StringView _szExpr, _Out_ Value* _pValue);

            static HRESULT __MEGA_UI_API ParserInt32Value(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserBoolValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserStringValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);
            
            static HRESULT __MEGA_UI_API ParserFunction(aStringView _szFunctionName, ExprNode* _pExprNode, aStringView _szFormat, _Out_cap_(_uArgCount) ParsedArg* _pArg, _In_ uint_t _uArgCount);
            
            template<uint_t _uArgCount>
            __inline static HRESULT __MEGA_UI_API ParserFunction(aStringView _szFunctionName, ExprNode* _pExprNode, aStringView _szFormat, _Out_ ParsedArg (&_Arg)[_uArgCount])
            {
                return ParserFunction(_szFunctionName, _pExprNode, _szFormat, _Arg, _uArgCount);
            }

            static HRESULT __MEGA_UI_API ParserPointValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserSizeValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserRectValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            static HRESULT __MEGA_UI_API ParserColorValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);

            HRESULT __MEGA_UI_API ParserStyleSheetValue(const PropertyInfo* _pProp, ExprNode* _pExprNode, Value* _pValue);


            HRESULT __MEGA_UI_API Play(
                _In_ ArrayView<uint8_t>& _ByteCode,
                _In_opt_ Element* _pTopElement,
                _Out_opt_ intptr_t* _pCooike,
                _Inout_ DynamicArray<Element*, false, false>* _ppElement
                );
            
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
