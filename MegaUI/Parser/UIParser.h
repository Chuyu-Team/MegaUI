#pragma once

#include "../base/MegaUITypeInt.h"
#include "../core/Element.h"
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

        // 从 XML 或者 二进制XML反序列出 Element
        class UIParser
        {
        private:
            DynamicArray<Value> LocalValueCache;

            DynamicArray<UIParserRecorder> RecorderArray;
            
            DynamicArray<IClassInfo*> ClassArray;
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

            HRESULT __MEGA_UI_API Play(u8StringView _szResID, Element* _pTopElement, intptr_t* _pCooike, Element** _ppElement);
        protected:

            IClassInfo* __MEGA_UI_API FindClassInfo(_In_ raw_const_astring_t _szClassName, _Out_opt_ uint32_t* _pIndex = nullptr);

            HRESULT __MEGA_UI_API ParserElementNode(_In_ rapidxml::xml_node<char>* _pNote, _Inout_ UIParserRecorder* _pRecorder);

            HRESULT __MEGA_UI_API ParserElementProperty(_In_ rapidxml::xml_attribute<char>* _pAttribute, _In_ IClassInfo* _pClassInfo, _Inout_ UIParserRecorder* _pRecorder);

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


            HRESULT __MEGA_UI_API Play(ArrayView<uint8_t>& _ByteCode, Element* _pTopElement, intptr_t* _pCooike, DynamicArray<Element*, false, false>* _ppElement);

        };
    }
} // namespace YY

#pragma pack(pop)
