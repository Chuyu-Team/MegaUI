#include "pch.h"
#include "UIParser.h"

#include "../base/alloc.h"
#include "../base/StringTransform.h"
#include "ValueParser.h"
#include "../core/StyleSheet.h"

#define RAPIDXML_STATIC_POOL_SIZE 4 * 1024

#include <ThirdParty/rapidxml/rapidxml.hpp>


#define _RAPAIDXML_STAATIC_STRING(_STR) _STR, (std::size(_STR) - 1)

#define _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_NODE, _STR) (_NODE->name_size() == (std::size(_STR) - 1) && _stricmp(_NODE->name(), _STR) == 0)

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
#pragma pack(push, 1)
        struct ByteCodeBase
        {
            // 字节码大小
            uint8_t cbData;
            // 字节码类型
            uint8_t Type;

            // uint8_t Data[0];

            enum ByteCodeType
            {
                // 字节码开始
                ByteCodeBegin = 0,
                // 字节码结束
                ByteCodeEnd,
                // 开始解析子控件
                ChildElementBegin,
                // 子控件解析结束，这时会把子控件提交到父控件中
                ChildElementEnd,

                // 创建控件
                CreateElement,
                // 设置控件属性
                SetProperty,
            };
        };
        
        struct ByteCodeBegin
        {
            // 字节码大小
            uint8_t cbData = sizeof(ByteCodeBegin);
            // 字节码类型
            uint8_t Type = ByteCodeBase::ByteCodeBegin;
        };

        struct ByteCodeEnd
        {
            // 字节码大小
            uint8_t cbData = sizeof(ByteCodeEnd);
            // 字节码类型
            uint8_t Type = ByteCodeBase::ByteCodeEnd;
        };

        struct ChildElementBegin
        {
            // 字节码大小
            uint8_t cbData = sizeof(ChildElementBegin);
            // 字节码类型
            uint8_t Type = ByteCodeBase::ChildElementBegin;
        };
        
        struct ChildElementEnd
        {
            // 字节码大小
            uint8_t cbData = sizeof(ChildElementEnd);
            // 字节码类型
            uint8_t Type = ByteCodeBase::ChildElementEnd;
        };

        struct CreateElement
        {
            // 字节码大小
            uint8_t cbData = sizeof(CreateElement);
            // 字节码类型
            uint8_t Type = ByteCodeBase::CreateElement;
            uint16_t uIndexOfElementName = 0;
        };

        struct SetProperty
        {
            // 字节码大小
            uint8_t cbData = sizeof(SetProperty);
            // 字节码类型
            uint8_t Type = ByteCodeBase::SetProperty;
            uint16_t uIndexOfPropertyValue = 0;
            const PropertyInfo* pProp = nullptr;
        };
#pragma pack(pop)

        struct ParsedArg
        {
            const EnumMap* pEnumMaps;

            union
            {
                // , 跳过
                // * 忽略
                // I
                struct
                {
                    int32_t iNumber;
                    ValueSuffixType SuffixType;
                };
                // S
                u8StringView szString;
                // C
                Color cColor;
            };

            // 故意不初始化内存
            #pragma warning(suppress : 26495)
            ParsedArg(const EnumMap* _pEnumMaps = nullptr)
                : pEnumMaps(_pEnumMaps)
            {
            }
        };

        static void* rapidxml_alloc(std::size_t _uByteSize)
        {
            return HAlloc(_uByteSize);
        }

        static void rapidxml_free(void* _pAddress)
        {
            HFree(_pAddress);
        }

        void __MEGA_UI_API UIParser::Clear()
        {
            LocalValueCache.Clear();
            RecorderArray.Clear();
        }

        HRESULT __MEGA_UI_API UIParser::ParserByXmlString(u8String&& _szXmlString)
        {
            Clear();
            if (_szXmlString.GetSize() == 0)
                return E_INVALIDARG;

            auto _pStringBuffer = (char*) _szXmlString.LockBuffer();
            if (!_pStringBuffer)
                return E_OUTOFMEMORY;
            _szXmlString.UnlockBuffer();

            // 为了从兼容性考虑，这里直接用 char 代表 u8char_t，注意上下文。
            rapidxml::xml_document<char> _XmlDocument;
            _XmlDocument.set_allocator(rapidxml_alloc, rapidxml_free);
            try
            {
                _XmlDocument.parse<rapidxml::parse_default>(_pStringBuffer);
            } catch (const rapidxml::parse_error&)
            {
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            }

            auto _pRoot = _XmlDocument.first_node(_RAPAIDXML_STAATIC_STRING("MegaUI"), false);
            if (!_pRoot)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            HRESULT _hr = S_OK;

            do
            {
                // Windows的解析过程依赖样式表，所以样式表优先检测
                auto _pStyleSheetsNode = _pRoot->first_node(_RAPAIDXML_STAATIC_STRING("StyleSheets"), false);
                if (_pStyleSheetsNode)
                {
                    for (auto _pStyleSheetNote = _pStyleSheetsNode->first_node(_RAPAIDXML_STAATIC_STRING("StyleSheet"), false); _pStyleSheetNote; _pStyleSheetNote = _pStyleSheetNote->next_sibling(_RAPAIDXML_STAATIC_STRING("StyleSheet"), false))
                    {
                        if (_pStyleSheetNote->type() != rapidxml::node_element)
                            continue;

                        auto _pResourceIDAttribute = _pStyleSheetNote->first_attribute(_RAPAIDXML_STAATIC_STRING("ResourceID"), false);
                        if (!_pResourceIDAttribute)
                        {
                            _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                            break;
                        }

                        auto _pStyleSheet = HNew<StyleSheet>();
                        if (!_pStyleSheet)
                        {
                            _hr = E_OUTOFMEMORY;
                            break;
                        }

                        _hr = _pStyleSheet->SetSheetResourceID(u8String((u8char_t*)_pResourceIDAttribute->value(), _pResourceIDAttribute->value_size()));
                        _pStyleSheetNote->remove_attribute(_pResourceIDAttribute);

                        if (SUCCEEDED(_hr))
                        {
                            DynamicArray<StyleSheetXmlOption, false, false> _XmlOption;
                            _XmlOption.Reserve(128);

                            _hr = ParserStyleSheetNode(_pStyleSheetNote, _XmlOption, _pStyleSheet);
                        }

                        if (SUCCEEDED(_hr))
                        {
                            _pStyleSheet->MakeImmutable();

                            if(StyleSheets.EmplacePtr(_pStyleSheet))
                            {
                                continue;
                            }

                            _hr = E_OUTOFMEMORY;
                        }

                        _pStyleSheet->Release();
                        if (FAILED(_hr))
                            break;
                    }

                    if (FAILED(_hr))
                        break;
                }

                auto _pWindowsNode = _pRoot->first_node(_RAPAIDXML_STAATIC_STRING("Windows"), false);
                if (_pWindowsNode)
                {
                    for (auto _pNote = _pWindowsNode->first_node(); _pNote; _pNote = _pNote->next_sibling())
                    {
                        if (_pNote->type() != rapidxml::node_element)
                            continue;

                        auto _pResID = _pNote->first_attribute(_RAPAIDXML_STAATIC_STRING("ResourceID"), false);
                        if (!_pResID)
                        {
                            _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                            break;
                        }

                        UIParserRecorder _Recorder;
                        _hr = _Recorder.szResourceID.SetString((u8char_t*)_pResID->value(), _pResID->value_size());
                        if (FAILED(_hr))
                            return _hr;
                        _pNote->remove_attribute(_pResID);

                        ByteCodeBegin _ByteCodeBegin;
                        _hr = _Recorder.ByteCode.Add((uint8_t*)&_ByteCodeBegin, sizeof(_ByteCodeBegin));
                        if (FAILED(_hr))
                            return _hr;

                        _hr = ParserElementNode(_pNote, &_Recorder);
                        if (FAILED(_hr))
                            break;

                        ByteCodeEnd _ByteCodeEnd;
                        _hr = _Recorder.ByteCode.Add((uint8_t*)&_ByteCodeEnd, sizeof(_ByteCodeEnd));
                        if (FAILED(_hr))
                            break;

                        RecorderArray.EmplacePtr(std::move(_Recorder));
                    }
                }
            } while (false);

            if (FAILED(_hr))
                Clear();

            return _hr;
        }

        HRESULT __MEGA_UI_API UIParser::Play(u8StringView _szResID, UIParserPlayContext* _pContext, intptr_t* _pCooike, WindowElement** _ppElement)
        {
            if (!_ppElement)
                return E_INVALIDARG;

            *_ppElement = nullptr;

            for (auto& Recorder : RecorderArray)
            {
                if (Recorder.szResourceID.GetSize() != _szResID.GetSize() || _szResID.CompareI(Recorder.szResourceID.GetConstString()) != 0)
                    continue;
                ArrayView<uint8_t> _ByteCode(Recorder.ByteCode.GetData(), Recorder.ByteCode.GetSize());

                if (_ByteCode.GetSize() <= sizeof(ByteCodeBegin) + sizeof(ByteCodeEnd))
                {
                    // 一个有效的格式总应该包含 ByteCodeBegin + ByteCodeEnd；
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                }

                auto _pByteCodeBegin = (ByteCodeBegin*)_ByteCode.GetData();
                if (_pByteCodeBegin->Type != ByteCodeBase::ByteCodeBegin || _pByteCodeBegin->cbData < sizeof(ByteCodeBegin))
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _ByteCode.Slice(_pByteCodeBegin->cbData);

                UIParserPlayContext _Context;

                DynamicArray<Element*, false, false> _ElementList;
                auto _hr = Play(_ByteCode, _pContext ? _pContext : &_Context, _pCooike, &_ElementList);
                if (SUCCEEDED(_hr))
                {
                    // 正确的数据只可能存在一个顶级控件，顶级控件只能是 WindowElement。
                    if (_ElementList.GetSize() == 1)
                    {
                        auto _pWindowElement = (*_ElementList.GetData())->TryCast<WindowElement>();

                        if (_pWindowElement)
                        {
                            // 成功
                            *_ppElement = _pWindowElement;
                            return _hr;
                        }
                    }

                    _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                }
                
                for (auto _pItem : _ElementList)
                {
                    _pItem->Destroy(false);
                }

                return _hr;
            }

            return E_NOT_SET;
        }
        
        IControlInfo* __MEGA_UI_API UIParser::FindControlInfo(raw_const_astring_t _szControlName, uint32_t* _pIndex)
        {
            if (_pIndex)
                *_pIndex = uint32_max;

            if (_szControlName == nullptr || *_szControlName == '\0')
                return nullptr;

            auto _uSize = ControlInfoArray.GetSize();
            auto _pData = ControlInfoArray.GetData();

            if (_uSize > uint32_max)
                return nullptr;

            for (uint32_t _uIndex = 0; _uIndex != _uSize; ++_uIndex)
            {
                auto _pControlInfo = _pData[_uIndex];

                if (_strcmpi(_pControlInfo->GetName(), _szControlName) == 0)
                {
                    if (_pIndex)
                        *_pIndex = (uint32_t)_uIndex;
                    return _pControlInfo;
                }
            }

            auto _pControlInfo = GetRegisterControlInfo(_szControlName);
            if (!_pControlInfo)
                return nullptr;

            uint_t _uIndex;
            auto _ppBuffer = ControlInfoArray.AddAndGetPtr(&_uIndex);
            if (!_ppBuffer)
                return nullptr;

            if (_uIndex > uint32_max)
                throw Exception();

            *_ppBuffer = _pControlInfo;
            if (_pIndex)
                *_pIndex = (uint32_t)_uIndex;
            return _pControlInfo;
        }

        const PropertyInfo* __MEGA_UI_API UIParser::GetPropertyByName(IControlInfo* _pControlInfo, raw_const_astring_t _szPropName)
        {
            const PropertyInfo* _pProp = nullptr;
            // 搜索 PropertyInfo
            for (uint32_t _uIndex = 0; _pProp = _pControlInfo->EnumPropertyInfo(_uIndex); ++_uIndex)
            {
                if (_strcmpi(_pProp->pszName, _szPropName) == 0)
                {
                    break;
                }
            }

            return _pProp;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserElementNode(rapidxml::xml_node<char>* _pNote, UIParserRecorder* _pRecorder)
        {
            CreateElement _CreateElement;
            uint32_t _uIndex;
            auto _pControlInfo = FindControlInfo(_pNote->name(), &_uIndex);
            if (!_pControlInfo)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            _CreateElement.uIndexOfElementName = _uIndex;

            auto _hr = _pRecorder->ByteCode.Add((uint8_t*)&_CreateElement, sizeof(_CreateElement));
            if (FAILED(_hr))
                return _hr;

            for (auto _pAttr = _pNote->first_attribute(); _pAttr; _pAttr = _pAttr->next_attribute())
            {
                _hr = ParserElementProperty(_pAttr, _pControlInfo, _pRecorder);
                if (FAILED(_hr))
                    return _hr;
            }

            bool _bHasChild = false;
            
            for (auto _pChildNote = _pNote->first_node(); _pChildNote; _pChildNote = _pChildNote->next_sibling())
            {
                if (_pChildNote->type() != rapidxml::node_element)
                    continue;

                if (!_bHasChild)
                {
                    _bHasChild = true;
                    ChildElementBegin _ChildElementBegin;
                    _hr = _pRecorder->ByteCode.Add((uint8_t*)&_ChildElementBegin, sizeof(_ChildElementBegin));
                    if (FAILED(_hr))
                        return _hr;
                }

                _hr = ParserElementNode(_pChildNote, _pRecorder);
                if (FAILED(_hr))
                    return _hr;
            }

            if (_bHasChild)
            {
                ChildElementEnd _ChildElementEnd;
                _hr = _pRecorder->ByteCode.Add((uint8_t*)&_ChildElementEnd, sizeof(_ChildElementEnd));
                if (FAILED(_hr))
                    return _hr;
            }

            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserElementProperty(rapidxml::xml_attribute<char>* _pAttribute, IControlInfo* _pControlInfo, UIParserRecorder* _pRecorder)
        {
            auto _pProp = GetPropertyByName(_pControlInfo, _pAttribute->name());

            if (!_pProp)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            
            Value _Value;
            auto _hr = ParserValue(_pControlInfo, _pProp, u8StringView((u8char_t*)_pAttribute->value(), _pAttribute->value_size()), &_Value);
            if (FAILED(_hr))
                return _hr;


            if (!_pProp->ppValidValues)
                return E_UNEXPECTED;
            
            ValueParser _ValueParser;
            ExprNode _ExprNode;

            _hr = _ValueParser.Parse(
                u8StringView((u8char_t*)_pAttribute->value(), _pAttribute->value_size()),
                &_ExprNode);
            if (FAILED(_hr))
                return _hr;
            
            uint_t _uIndex;
            auto _ppBufer = LocalValueCache.AddAndGetPtr(&_uIndex);
            if (!_ppBufer)
                return E_OUTOFMEMORY;
            if (_uIndex > uint16_max)
                throw Exception();

            *_ppBufer = std::move(_Value);

            SetProperty SetPropertyByteCode;
            SetPropertyByteCode.uIndexOfPropertyValue = (uint16_t)_uIndex;
            SetPropertyByteCode.pProp = _pProp;
            _hr = _pRecorder->ByteCode.Add((uint8_t*)&SetPropertyByteCode, sizeof(SetPropertyByteCode));
            if (FAILED(_hr))
                return _hr;

            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserValue(IControlInfo* _pControlInfo, const PropertyInfo* _pProp, u8StringView _szExpr, Value* _pValue)
        {
            *_pValue = nullptr;

            if (!_pProp)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            if (!_pProp->ppValidValues)
                return E_UNEXPECTED;

            ValueParser _ValueParser;
            ExprNode _ExprNode;

            auto _hr = _ValueParser.Parse(
                _szExpr,
                &_ExprNode);

            if (FAILED(_hr))
                return _hr;

            Value _Value;

            for (auto pValidValue = _pProp->ppValidValues; *pValidValue != ValueType::Null; ++pValidValue)
            {
                HRESULT _hr = E_NOTIMPL;

                switch (*pValidValue)
                {
                case ValueType::int32_t:
                    _hr = ParserInt32Value(_pProp->pEnumMaps, &_ExprNode, &_Value);
                    break;
                case ValueType::float_t:
                    _hr = ParserFloatValue(_pProp->pEnumMaps, &_ExprNode, &_Value);
                    break;
                case ValueType::boolean:
                    _hr = ParserBoolValue(&_ExprNode, &_Value);
                    break;
                case ValueType::uString:
                    _hr = ParserStringValue(&_ExprNode, &_Value);
                    break;
                case ValueType::Point:
                    _hr = ParserPointValue(&_ExprNode, &_Value);
                    break;
                case ValueType::Size:
                    _hr = ParserSizeValue(&_ExprNode, &_Value);
                    break;
                case ValueType::Rect:
                    _hr = ParserRectValue(&_ExprNode, &_Value);
                    break;
                case ValueType::Color:
                    _hr = ParserColorValue(&_ExprNode, &_Value);
                    break;
                case ValueType::StyleSheet:
                    _hr = ParserStyleSheetValue(&_ExprNode, &_Value);
                    break;
                case ValueType::Font:
                    _hr = ParserFontValue(&_ExprNode, &_Value);
                    break;
                default:
                    break;
                }

                // 格式不支持，
                if (_hr == __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                    continue;

                if (FAILED(_hr))
                    return _hr;

                *_pValue = std::move(_Value);
                break;
            }
            
            return _hr;
        }

        HRESULT __MEGA_UI_API UIParser::ParserInt32Value(const u8StringView& _szValue, int32_t* _pValue, ValueSuffixType* _pParserSuffixType)
        {
            auto _cchValue = _szValue.GetSize();

            if (_pParserSuffixType)
            {
                *_pParserSuffixType = ValueSuffixType::None;
                if (_cchValue > 2)
                {
                    if (CharUpperASCII(_szValue[_cchValue - 2]) == 'P' && CharUpperASCII(_szValue[_cchValue - 1]) == 'X')
                    {
                        _cchValue -= 2;
                        *_pParserSuffixType = ValueSuffixType::Pixel;
                    }
                    else if (CharUpperASCII(_szValue[_cchValue - 2]) == 'D' && CharUpperASCII(_szValue[_cchValue - 1]) == 'P')
                    {
                        _cchValue -= 2;
                        *_pParserSuffixType = ValueSuffixType::DevicePixel;
                    }
                    else if (CharUpperASCII(_szValue[_cchValue - 2]) == 'P' && CharUpperASCII(_szValue[_cchValue - 1]) == 'T')
                    {
                        _cchValue -= 2;
                        *_pParserSuffixType = ValueSuffixType::FontPoint;
                    }
                }
            }

            int32_t _iValue = 0;

            // 16进制 0x???
            if (_cchValue >= 3 && _szValue[0] == '0' && (_szValue[1] == 'x' || _szValue[1] == 'X'))
            {
                for (uint_t _uIndex = 2; _uIndex != _cchValue; ++_uIndex)
                {
                    auto _ch = _szValue[_uIndex];

                    if (_ch >= '0' && _ch <= '9')
                    {
                        _iValue <<= 4;
                        _iValue |= _ch - '0';
                    }
                    else if (_ch >= 'A' && _ch <= 'F')
                    {
                        _iValue <<= 4;
                        _iValue |= _ch - 'A' + 10;
                    }
                    else if (_ch >= 'a' && _ch <= 'f')
                    {
                        _iValue <<= 4;
                        _iValue |= _ch - 'a' + 10;
                    }
                    else
                    {
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    }
                }
            }
            else
            {
                // 10进制
                uint_t _uIndex = 0;
                bool _bNegative = _szValue[0] == '-';
                if (_bNegative)
                    ++_uIndex;

                for (; _uIndex != _cchValue; ++_uIndex)
                {
                    auto _ch = _szValue[_uIndex];

                    if (_ch >= '0' && _ch <= '9')
                    {
                        _iValue *= 10;
                        _iValue += _ch - '0';
                    }
                    else
                    {
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    }
                }

                if (_bNegative)
                    _iValue *= -1;
            }

            *_pValue = _iValue;
            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserInt32Value(const EnumMap* pEnumMaps, ExprNode* _pExprNode, int32_t* _pValue, ValueSuffixType* _pParserSuffixType)
        {
            if (_pParserSuffixType)
                *_pParserSuffixType = ValueSuffixType::None;

            if (_pExprNode->Type == ExprNodeType::Or)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                
                int32_t _iValueOr = 0;
                for (auto& Expr : _pExprNode->ChildExprNode)
                {
                    int32_t _iValue = 0;
                    auto _hr = ParserInt32Value(pEnumMaps, &Expr, &_iValue, nullptr);
                    if (FAILED(_hr))
                        return _hr;

                    _iValueOr |= _iValue;
                }
                *_pValue = _iValueOr;
                return S_OK;
            }
            else if (_pExprNode->Type == ExprNodeType::BaseIdentifier)
            {
                auto szValue = _pExprNode->szValue;
                if (szValue.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                
                if (SUCCEEDED(ParserInt32Value(_pExprNode->szValue, _pValue, _pParserSuffixType)))
                    return S_OK;

                // 转换失败，但是Int32 可能映射到枚举值
                if (pEnumMaps)
                {
                    for (auto _pEnum = pEnumMaps; _pEnum->pszEnum; ++_pEnum)
                    {
                        if (_pExprNode->szValue.CompareI((u8char_t*)_pEnum->pszEnum) == 0)
                        {
                            *_pValue = _pEnum->nEnum;
                            return S_OK;
                        }
                    }
                }
            }
            
            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserStringValue(ExprNode* _pExprNode, u8StringView* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() != 0)
                {
                    auto _hr = ParserStringValue(&_pExprNode->ChildExprNode[0], _pValue);
                    if (SUCCEEDED(_hr) || _hr != __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                        return _hr;
                }
                *_pValue = _pExprNode->szValue;
                return S_OK;
            }
            else if (_pExprNode->Type == ExprNodeType::BaseIdentifier)
            {
                *_pValue = _pExprNode->szValue;
                return S_OK;
            }
            else
            {
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            }
        }

        HRESULT __MEGA_UI_API UIParser::ParserInt32Value(const EnumMap* pEnumMaps, ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                
                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            int32_t _iValue = 0;
            auto _hr = ParserInt32Value(pEnumMaps, _pExprNode, &_iValue, nullptr);
            if(FAILED(_hr))
                return _hr;

            auto _Value = Value::CreateInt32(_iValue);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;

            *_pValue = std::move(_Value);
            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserFloatValue(const EnumMap* pEnumMaps, ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            // 只接受整形输入，使用浮点只是为了减少运算精度损失
            int32_t _iValue = 0;
            ValueSuffixType _SuffixType;
            auto _hr = ParserInt32Value(pEnumMaps, _pExprNode, &_iValue, &_SuffixType);
            if (FAILED(_hr))
                return _hr;

            ValueSuffix _Suffix = {_SuffixType, ValueSuffixType::None, ValueSuffixType::None, ValueSuffixType::None, 96};
            auto _Value = Value::CreateFloat((float)_iValue, _Suffix);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;

            *_pValue = std::move(_Value);
            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserBoolValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            if (_pExprNode->Type == ExprNodeType::BaseIdentifier)
            {
                if (_pExprNode->szValue.CompareI((u8char_t*)"true") == 0)
                {
                    *_pValue = Value::CreateBool(true);
                    return S_OK;
                }
                else if (_pExprNode->szValue.CompareI((u8char_t*)"false") == 0)
                {
                    *_pValue = Value::CreateBool(false);
                    return S_OK;
                }
            }

            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserStringValue(ExprNode* _pExprNode, Value* _pValue)
        {
            u8StringView _ValueView;
            auto _hr = ParserStringValue(_pExprNode, &_ValueView);
            if (FAILED(_hr))
                return _hr;

            uString _szTmp;
            _hr = Transform(_ValueView, &_szTmp);
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreateString(_szTmp);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = _Value;
            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserFunction(aStringView _szFunctionName, ExprNode* _pExprNode, aStringView _szFormat, ParsedArg* _pArg, uint_t _uArgCount)
        {
            if (_pExprNode->Type != ExprNodeType::Funcall)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            if (_pExprNode->szValue.GetSize() != _szFunctionName.GetSize() || _strnicmp((char*)_pExprNode->szValue.GetConstString(), _szFunctionName.GetConstString(), _szFunctionName.GetSize()) != 0)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            if (_pExprNode->ChildExprNode.GetSize() != _szFormat.GetSize())
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            HRESULT _hr = S_OK;

            uint_t _uArgIndex = 0;

            for (auto _ch : _szFormat)
            {
                // 跳过
                if (_ch == ',')
                    continue;

                if (_uArgIndex == _uArgCount)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                switch (_ch)
                {
                case '*':
                    // 纯占位
                    ++_uArgIndex;
                    break;
                case 'I':
                    // int32_t
                    _hr = ParserInt32Value(_pArg[_uArgIndex].pEnumMaps, &_pExprNode->ChildExprNode[_uArgIndex], &_pArg[_uArgIndex].iNumber, &_pArg[_uArgIndex].SuffixType);
                    ++_uArgIndex;
                    break;
                case 'S':
                    _hr = ParserStringValue(&_pExprNode->ChildExprNode[_uArgIndex], &_pArg[_uArgIndex].szString);
                    ++_uArgIndex;
                    break;
                case 'C':
                    _hr = ParserColorValue(&_pExprNode->ChildExprNode[_uArgIndex], &_pArg[_uArgIndex].cColor);
                    ++_uArgIndex;
                    break;
                default:
                    _hr = E_NOTIMPL;
                    break;
                }
                
                if (FAILED(_hr))
                    return _hr;
            }

            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserPointValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            ParsedArg _Arg[2] = {};

            auto _hr = ParserFunction("Point", _pExprNode, "II", _Arg, std::size(_Arg));
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreatePoint((float)_Arg[0].iNumber, (float)_Arg[1].iNumber);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserSizeValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            ParsedArg _Arg[2] = {};

            auto _hr = ParserFunction("Size", _pExprNode, "II", _Arg);
            if (FAILED(_hr))
                return _hr;

            ValueSuffix _Suffix = {_Arg[0].SuffixType, _Arg[1].SuffixType, ValueSuffixType::None, ValueSuffixType::None, 96};
            auto _Value = Value::CreateSize((float)_Arg[0].iNumber, (float)_Arg[1].iNumber, _Suffix);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserRectValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            ParsedArg _Arg[4] = {};

            auto _hr = ParserFunction("Rect", _pExprNode, "IIII", _Arg);
            if (FAILED(_hr))
                return _hr;

            ValueSuffix _Suffix = {_Arg[0].SuffixType, _Arg[1].SuffixType, _Arg[2].SuffixType, _Arg[3].SuffixType, 96};
            auto _Value = Value::CreateRect((float)_Arg[0].iNumber, (float)_Arg[1].iNumber, (float)_Arg[2].iNumber, (float)_Arg[3].iNumber, _Suffix);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserColorValue(ExprNode* _pExprNode, Color* _pValue)
        {
            Color _Color;

            ParsedArg _Arg[4] = {};
            auto _hr = ParserFunction("ARGB", _pExprNode, "IIII", _Arg);
            if (SUCCEEDED(_hr))
            {
                _Color = Color::MakeARGB(_Arg[0].iNumber, _Arg[1].iNumber, _Arg[2].iNumber, _Arg[3].iNumber);
            }
            else if (_hr == __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
            {
                _hr = ParserFunction("RGB", _pExprNode, "III", _Arg);
                if (SUCCEEDED(_hr))
                    _Color = Color::MakeRGB(_Arg[0].iNumber, _Arg[1].iNumber, _Arg[2].iNumber);
            }

            if (FAILED(_hr))
                return _hr;

            *_pValue = _Color;
            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserColorValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            Color _Color;
            auto _hr = ParserColorValue(_pExprNode, &_Color);
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreateColor(_Color);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);
            return S_OK;
        }

        HRESULT __MEGA_UI_API UIParser::ParserStyleSheetValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }

            if (_pExprNode->Type != ExprNodeType::BaseIdentifier)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            auto _szSheetResidNeed = _pExprNode->szValue;

            for (auto pStyleSheet : StyleSheets)
            {
                auto _szSheetResidTarget = pStyleSheet->GetSheetResourceID();
                if (_szSheetResidNeed.GetSize() == _szSheetResidTarget.GetSize() && _szSheetResidNeed.CompareI(_szSheetResidTarget.GetConstString()) == 0)
                {
                    auto _StyleSheetValue = Value::CreateStyleSheet(pStyleSheet);
                    if (_StyleSheetValue == nullptr)
                        return E_OUTOFMEMORY;
                    *_pValue = _StyleSheetValue;
                    return S_OK;
                    break;
                }
            }

            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }

        HRESULT __MEGA_UI_API UIParser::ParserFontValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = &_pExprNode->ChildExprNode[0];
            }
            
            ParsedArg _Arg[] = {{}, {}, {GetFontWeightEnumMap()}, {GetFontStyleEnumMap()}, {}};

            auto _hr = ParserFunction("Font", _pExprNode, "SIIIC", _Arg);
            if (FAILED(_hr))
                return _hr;

            uString _szFace;
            _hr = Transform(_Arg[0].szString, &_szFace);
            if (FAILED(_hr))
                return _hr;

            ValueSuffix _Suffix = {_Arg[1].SuffixType, ValueSuffixType::None, ValueSuffixType::None, ValueSuffixType::None, 96};
            auto _FontValue = Value::CreateFont(_szFace, abs((float)_Arg[1].iNumber), _Arg[2].iNumber, _Arg[3].iNumber, _Arg[4].cColor, _Suffix);
            if (_FontValue == nullptr)
                return E_OUTOFMEMORY;

            *_pValue = std::move(_FontValue);
            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::Play(ArrayView<uint8_t>& _ByteCode, UIParserPlayContext* _pContext, intptr_t* _pCooike, DynamicArray<Element*, false, false>* _pElementArray)
        {
            if (_pCooike)
                *_pCooike = 0;

            Element* _pCurrentElement = nullptr;
            HRESULT _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            for (;;)
            {
                if (_ByteCode.GetSize() == 0)
                {
                    _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    break;
                }

                auto _pByteCodeBase = (ByteCodeBase*)_ByteCode.GetData();

                if (_ByteCode.GetSize() < sizeof(ByteCodeBase) || _pByteCodeBase->cbData > _ByteCode.GetSize() || _pByteCodeBase->cbData < sizeof(ByteCodeBase))
                {
                    _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    break;
                }

                if (_pByteCodeBase->Type == ByteCodeBase::ChildElementBegin)
                {
                    if (_pCurrentElement == nullptr || _pByteCodeBase->cbData < sizeof(ChildElementBegin))
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    _ByteCode.Slice(_pByteCodeBase->cbData);

                    DynamicArray<Element*, false, false> _Child;

                    _hr = Play(_ByteCode, _pContext, _pCooike, &_Child);
                    if (FAILED(_hr))
                        break;

                    _hr = _pCurrentElement->Add(_Child.GetData(), _Child.GetSize());
                    if (FAILED(_hr))
                        break;
                }
                else if (_pByteCodeBase->Type == ByteCodeBase::ChildElementEnd)
                {
                    if (_pByteCodeBase->cbData < sizeof(ChildElementEnd))
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    _ByteCode.Slice(_pByteCodeBase->cbData);

                    if (_ByteCode.GetSize() == 0)
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }

                    return S_OK;
                }
                else if (_pByteCodeBase->Type == ByteCodeBase::ByteCodeEnd)
                {
                    if (_pByteCodeBase->cbData < sizeof(ByteCodeEnd))
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    _ByteCode.Slice(_pByteCodeBase->cbData);

                    if (_ByteCode.GetSize() != 0)
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }

                    return S_OK;
                }
                else if (_pByteCodeBase->Type == ByteCodeBase::CreateElement)
                {
                    if (_pByteCodeBase->cbData < sizeof(CreateElement))
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    _ByteCode.Slice(_pByteCodeBase->cbData);

                    auto _pCreateElement = (CreateElement*)_pByteCodeBase;
                    auto _ppClass = ControlInfoArray.GetItemPtr(_pCreateElement->uIndexOfElementName);

                    if (!_ppClass)
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    auto _pClass = *_ppClass;

                    _hr = _pClass->CreateInstance(_pContext->iDPI, _pContext->pTopElement, _pCooike, &_pCurrentElement);
                    if (FAILED(_hr))
                        break;

                    if (!_pElementArray->EmplacePtr(_pCurrentElement))
                    {
                        HDelete(_pCurrentElement);
                        _pCurrentElement = nullptr;
                        _hr = E_OUTOFMEMORY;
                        break;
                    }
                }
                else if (_pByteCodeBase->Type == ByteCodeBase::SetProperty)
                {
                    if (_pCurrentElement == nullptr || _pByteCodeBase->cbData < sizeof(SetProperty))
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }
                    _ByteCode.Slice(_pByteCodeBase->cbData);

                    auto _pSetProperty = (SetProperty*)_pByteCodeBase;

                    auto _ppValue = LocalValueCache.GetItemPtr(_pSetProperty->uIndexOfPropertyValue);
                    if (!_ppValue)
                    {
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        break;
                    }

                    // 忽略错误
                    _pCurrentElement->SetValue(*_pSetProperty->pProp, PropertyIndicies::PI_Local, _ppValue->UpdateDpi(_pCurrentElement->GetDpi()));
                }
                else
                {
                    _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    break;
                }
            }

            for (auto _pElement : *_pElementArray)
            {
                _pElement->Destroy(false);
            }

            _pElementArray->Clear();

            return _hr;
        }
        
        static ValueCmpOperation __MEGA_UI_API TryParserStyleSheetOptionXmlType(
            _In_ rapidxml::xml_node<char>* _OptionNode,
            _Out_ u8StringView* _pszValue
            )
        {
            _pszValue->SetString(nullptr, 0);
            auto _pAttr = _OptionNode->first_attribute();
            if (!_pAttr)
                return ValueCmpOperation::Invalid;
            
            ValueCmpOperation _Operation = ValueCmpOperation::Invalid;

            do
            {
                if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "Equal") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "EQU"))
                {
                    _Operation = ValueCmpOperation::Equal;
                }
                else if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "GreaterThan") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "GTR"))
                {
                    _Operation = ValueCmpOperation::GreaterThan;
                }
                else if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "GreaterThanOrEqual") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "GEQ"))
                {
                    _Operation = ValueCmpOperation::GreaterThanOrEqual;
                }
                else if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "LessThan") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "LSS"))
                {
                    _Operation = ValueCmpOperation::LessThan;
                }
                else if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "LessThanOrEqual") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "LEQ"))
                {
                    _Operation = ValueCmpOperation::LessThanOrEqual;
                }
                else if (_RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "NotEqual") || _RAPAIDXML_NODE_NAME_EQUAL_STATIC_STRING(_pAttr, "NEQ"))
                {
                    _Operation = ValueCmpOperation::NotEqual;
                }
                else
                {
                    return ValueCmpOperation::Invalid;
                }
            } while (false);
            
            _pszValue->SetString((u8char_t*)_pAttr->value(), _pAttr->value_size());
            return _Operation;
        }

        HRESULT __MEGA_UI_API UIParser::ParserStyleSheetNode(rapidxml::xml_node<char>* _StyleSheetNode, DynamicArray<StyleSheetXmlOption, false, false>& _XmlOption, StyleSheet* _pStyleSheet)
        {
            auto _uOldlOptionSize = _XmlOption.GetSize();

            // Default 是备选值，当其他条件无法命中时，它将生效。
            auto _pDefaultNode = _StyleSheetNode->first_node(_RAPAIDXML_STAATIC_STRING("Default"), false);
            if (_pDefaultNode)
            {                
                auto _hr = ParserStyleSheetNode(_pDefaultNode, _XmlOption, _pStyleSheet);
                if (FAILED(_hr))
                    return _hr;

                _StyleSheetNode->remove_node(_pDefaultNode);
            }

            for (auto _pItem = _StyleSheetNode->first_node(); _pItem; _pItem = _pItem->next_sibling())
            {
                if (_pItem->type() != rapidxml::node_element)
                    continue;

                u8StringView _szPropValue;
                auto _XmlType = TryParserStyleSheetOptionXmlType(_pItem, &_szPropValue);

                if (_XmlType != ValueCmpOperation::Invalid)
                {
                    auto _pOption = _XmlOption.EmplacePtr();
                    if (!_pOption)
                        return E_OUTOFMEMORY;

                    _pOption->Type = _XmlType;
                    _pOption->szPropName.SetString((u8char_t*)_pItem->name(), _pItem->name_size());
                    _pOption->szPropValue = _szPropValue;

                    auto _hr = ParserStyleSheetNode(_pItem, _XmlOption, _pStyleSheet);
                    if (FAILED(_hr))
                        return _hr;
                    
                    _XmlOption.Resize(_uOldlOptionSize);
                }
                else
                {
                    // 这应该是一个控件
                    auto _hr = ParserStyleSheetElementNode(_pItem, _XmlOption, _pStyleSheet);
                    if (FAILED(_hr))
                        return _hr;
                }
            }

            _XmlOption.Resize(_uOldlOptionSize);


            return S_OK;
        }
        
        HRESULT __MEGA_UI_API UIParser::ParserStyleSheetElementNode(rapidxml::xml_node<char>* _pElementValueNode, const DynamicArray<StyleSheetXmlOption, false, false>& _XmlOption, StyleSheet* _pStyleSheet)
        {
            uint32_t _uIndex;

            auto _pControlInfo = FindControlInfo(_pElementValueNode->name(), &_uIndex);
            if (!_pControlInfo)
            {
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            }

            // 首先将匹配条件 _XmlOption 转换 到 StyleSheet::Cond
            DynamicArray<Cond, true> CondArray;
            if (auto _uSize = _XmlOption.GetSize())
            {
                auto _hr = CondArray.Reserve(_uSize);
                if (FAILED(_hr))
                    return _hr;
            }

            for (auto& _XmlOptionItem : _XmlOption)
            {
                auto _pProp = GetPropertyByName(_pControlInfo, (raw_const_astring_t)_XmlOptionItem.szPropName.GetConstString());
                if (!_pProp)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                Value _Value;
                auto _hr = ParserValue(_pControlInfo, _pProp, _XmlOptionItem.szPropValue, &_Value);
                if (FAILED(_hr))
                    return _hr;

                auto _pCond = CondArray.EmplacePtr();
                if (!_pCond)
                    return E_OUTOFMEMORY;

                _pCond->pProp = _pProp;
                _pCond->OperationType = _XmlOptionItem.Type;
                _pCond->Value = std::move(_Value);
            }

            // 属性转换到 _DeclArray，作为样式表的值
            DynamicArray<Decl, false> _DeclArray;
            for (auto _pPropXml = _pElementValueNode->first_attribute(); _pPropXml; _pPropXml = _pPropXml->next_attribute())
            {
                auto _pProp = GetPropertyByName(_pControlInfo, _pPropXml->name());
                if (!_pProp)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                Value _Value;
                auto _hr = ParserValue(_pControlInfo, _pProp, u8StringView((u8char_t*)_pPropXml->value(), _pPropXml->value_size()), &_Value);
                if (FAILED(_hr))
                    return _hr;

                auto _pDecl = _DeclArray.EmplacePtr();
                if (!_pDecl)
                    return E_OUTOFMEMORY;

                _pDecl->pProp = _pProp;
                _pDecl->Value = std::move(_Value);
            }

            return _pStyleSheet->AddRule(uString(), _pControlInfo, CondArray, ArrayView<Decl>(_DeclArray.GetData(), _DeclArray.GetSize()));
        }
    } // namespace MegaUI
} // namespace YY