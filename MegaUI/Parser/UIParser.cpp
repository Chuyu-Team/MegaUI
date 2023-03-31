#include "pch.h"
#include "UIParser.h"

#include <MegaUI/base/alloc.h>
#include <Base/Strings/StringTransform.h>
#include "ValueParser.h"
#include <MegaUI/core/StyleSheet.h>
#include <MegaUI/Render/FontEnumMap.h>

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

        enum class ParsedArgType : uint16_t
        {
            None,
            int32_t,
            float_t,
            uString,
            Color,
            Rect,
        };

        struct ParsedArg
        {
            const EnumMap* pEnumMaps;
            union
            {
                struct
                {
                    ValueSuffixType Type1 : 4;
                    ValueSuffixType Type2 : 4;
                    ValueSuffixType Type3 : 4;
                    ValueSuffixType Type4 : 4;

                    ParsedArgType eArgType;
                };

                uint16_t uRawView;
            };
            
            union
            {
                // , 跳过
                // * 忽略
                // I
                int32_t iNumber;

                // F
                float FloatNumber;

                // S
                uString szString;
                // C
                Color crColor;
                // R 
                Rect rcRect;
            };

            ParsedArg(ParsedArgType _eArgType, const EnumMap* _pEnumMaps = nullptr)
                : pEnumMaps(_pEnumMaps)
                , Type1 {ValueSuffixType::None}
                , Type2 {ValueSuffixType::None}
                , Type3 {ValueSuffixType::None}
                , Type4 {ValueSuffixType::None}
                , eArgType(_eArgType)
            {
                switch (_eArgType)
                {
                case ParsedArgType::int32_t:
                    iNumber = 0;
                    break;
                case ParsedArgType::float_t:
                    FloatNumber = 0;
                    break;
                case ParsedArgType::uString:
                    new (&szString) uString();
                    break;
                case ParsedArgType::Color:
                    crColor = 0;
                    break;
                case ParsedArgType::Rect:
                    rcRect = {};
                    break;
                default:
                    break;
                }
            }

            ~ParsedArg()
            {
                if (eArgType == ParsedArgType::uString)
                {
                    szString.~uString();
                }
            }

            ValueSuffix __YYAPI GetSuffix() const
            {
                ValueSuffix _Suffix;
                _Suffix.RawView = uRawView;
                _Suffix.Dpi = uRawView ? 96 : 0;
                return _Suffix;
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

        void __YYAPI UIParser::Clear()
        {
            LocalValueCache.Clear();
            RecorderArray.Clear();

            StyleSheets.Clear();
        }

        HRESULT __YYAPI UIParser::ParserByXmlString(u8String&& _szXmlString)
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

                        ComPtr<StyleSheet> _pInherit;
                        auto _pInheritAttribute = _pStyleSheetNote->first_attribute(_RAPAIDXML_STAATIC_STRING("Inherit"), false);
                        if (_pInheritAttribute && _pInheritAttribute->value_size())
                        {
                            GetStyleSheet(u8StringView((u8char_t*)_pInheritAttribute->value(), _pInheritAttribute->value_size()), &_pInherit);
                        }

                        ComPtr<StyleSheet> _pStyleSheet;
                        _pStyleSheet.Attach(HNew<StyleSheet>(_pInherit));
                        if (!_pStyleSheet)
                        {
                            _hr = E_OUTOFMEMORY;
                            break;
                        }

                        _hr = _pStyleSheet->SetSheetResourceID(u8String((u8char_t*)_pResourceIDAttribute->value(), _pResourceIDAttribute->value_size()));
                        _pStyleSheetNote->remove_attribute(_pResourceIDAttribute);

                        if (SUCCEEDED(_hr))
                        {
                            Array<StyleSheetXmlOption, AllocPolicy::SOO> _XmlOption;
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

        HRESULT __YYAPI UIParser::Play(u8StringView _szResID, UIParserPlayContext* _pContext, intptr_t* _pCooike, WindowElement** _ppElement)
        {
            if (_pCooike)
                *_pCooike = 0;

            if (!_ppElement)
                return E_INVALIDARG;

            *_ppElement = nullptr;

            for (auto& Recorder : RecorderArray)
            {
                if (Recorder.szResourceID.GetSize() != _szResID.GetSize() || _szResID.CompareI(Recorder.szResourceID.GetConstString()) != 0)
                    continue;
                ArrayView<const uint8_t> _ByteCode(Recorder.ByteCode.GetData(), Recorder.ByteCode.GetSize());

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
                if (!_pContext)
                    _pContext = &_Context;

                Array<Element*, AllocPolicy::SOO> _ElementList;
                intptr_t _Cooike = 0;
                auto _hr = Play(_ByteCode, _pContext, _pContext->pTopElement, &_Cooike, &_ElementList);
                if (SUCCEEDED(_hr))
                {
                    // 正确的数据只可能存在一个顶级控件，顶级控件只能是 WindowElement。
                    if (_ElementList.GetSize() == 1)
                    {
                        auto _pWindowElement = (*_ElementList.GetData())->TryCast<WindowElement>();

                        if (_pWindowElement)
                        {
                            if (_pCooike)
                            {
                                *_pCooike = _Cooike;
                            }
                            else
                            {
                                _pWindowElement->EndDefer(_Cooike);
                            }

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
        
        HRESULT UIParser::GetStyleSheet(u8StringView _szSheetResourceID, StyleSheet** _ppSheet)
        {
            *_ppSheet = nullptr;

            for (StyleSheet* _pStyleSheet : StyleSheets)
            {
                auto _szSheetResidTarget = _pStyleSheet->GetSheetResourceID();
                if (_szSheetResourceID.GetSize() == _szSheetResidTarget.GetSize() && _szSheetResourceID.CompareI(_szSheetResidTarget.GetConstString()) == 0)
                {
                    _pStyleSheet->AddRef();
                    *_ppSheet = _pStyleSheet;
                    return S_OK;
                }
            }

            return StyleSheet::GetGlobalStyleSheet(_szSheetResourceID, _ppSheet);
        }

        Array<ComPtr<StyleSheet>> __YYAPI UIParser::GetAllStyleSheet()
        {
            return StyleSheets;
        }

        IControlInfo* __YYAPI UIParser::FindControlInfo(raw_const_astring_t _szControlName, uint32_t* _pIndex)
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

            auto _ppBuffer = ControlInfoArray.EmplacePtr();
            if (!_ppBuffer)
                return nullptr;

            uint_t _uIndex = ControlInfoArray.GetItemIndex(_ppBuffer);
            if (_uIndex > uint32_max)
                throw Exception();

            *_ppBuffer = _pControlInfo;
            if (_pIndex)
                *_pIndex = (uint32_t)_uIndex;
            return _pControlInfo;
        }

        const PropertyInfo* __YYAPI UIParser::GetPropertyByName(IControlInfo* _pControlInfo, raw_const_astring_t _szPropName)
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
        
        HRESULT __YYAPI UIParser::ParserElementNode(rapidxml::xml_node<char>* _pNote, UIParserRecorder* _pRecorder)
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

            // Note内容转换为 Content
            if (_pNote->value_size())
            {
                rapidxml::xml_attribute<char> _ContentAttribute;
                _ContentAttribute.name(_RAPAIDXML_STAATIC_STRING("Content"));
                _ContentAttribute.value(_pNote->value(), _pNote->value_size());
                _hr = ParserElementProperty(&_ContentAttribute, _pControlInfo, _pRecorder);
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
        
        HRESULT __YYAPI UIParser::ParserElementProperty(rapidxml::xml_attribute<char>* _pAttribute, IControlInfo* _pControlInfo, UIParserRecorder* _pRecorder)
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
            
            auto _ppBufer = LocalValueCache.EmplacePtr();
            if (!_ppBufer)
                return E_OUTOFMEMORY;

            uint_t _uIndex = LocalValueCache.GetItemIndex(_ppBufer);
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

        HRESULT __YYAPI UIParser::ParserValue(IControlInfo* _pControlInfo, const PropertyInfo* _pProp, u8StringView _szExpr, Value* _pValue)
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

        HRESULT __YYAPI UIParser::ParserInt32Value(const u8StringView& _szValue, ParsedArg* _pValue)
        {
            auto _cchValue = _szValue.GetSize();
            _pValue->uRawView = 0;
            if (_cchValue > 2)
            {
                if (CharUpperAsASCII(_szValue[_cchValue - 2]) == 'P' && CharUpperAsASCII(_szValue[_cchValue - 1]) == 'X')
                {
                    _cchValue -= 2;
                    _pValue->Type1 = ValueSuffixType::Pixel;
                }
                else if (CharUpperAsASCII(_szValue[_cchValue - 2]) == 'D' && CharUpperAsASCII(_szValue[_cchValue - 1]) == 'P')
                {
                    _cchValue -= 2;
                    _pValue->Type1 = ValueSuffixType::DevicePixel;
                }
                else if (CharUpperAsASCII(_szValue[_cchValue - 2]) == 'P' && CharUpperAsASCII(_szValue[_cchValue - 1]) == 'T')
                {
                    _cchValue -= 2;
                    _pValue->Type1 = ValueSuffixType::FontPoint;
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

            _pValue->iNumber = _iValue;
            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserInt32Value(ExprNode* _pExprNode, ParsedArg* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
            }

            _pValue->uRawView = 0;

            if (_pExprNode->Type == ExprNodeType::Or)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                
                int32_t _iValueOr = 0;
                for (auto& Expr : _pExprNode->ChildExprNode)
                {
                    auto _hr = ParserInt32Value(&Expr, _pValue);
                    if (FAILED(_hr))
                        return _hr;
                    if (_pValue->uRawView != 0)
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                    _iValueOr |= _pValue->iNumber;
                }
                _pValue->iNumber = _iValueOr;
                return S_OK;
            }
            else if (_pExprNode->Type == ExprNodeType::BaseIdentifier)
            {
                auto szValue = _pExprNode->szValue;
                if (szValue.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                
                if (SUCCEEDED(ParserInt32Value(_pExprNode->szValue, _pValue)))
                    return S_OK;

                // 转换失败，但是Int32 可能映射到枚举值
                if (_pValue->pEnumMaps)
                {
                    for (auto _pEnum = _pValue->pEnumMaps; _pEnum->pszEnum; ++_pEnum)
                    {
                        if (_pExprNode->szValue.CompareI((u8char_t*)_pEnum->pszEnum) == 0)
                        {
                            _pValue->iNumber = _pEnum->nEnum;
                            return S_OK;
                        }
                    }
                }
            }
            else if (_pExprNode->Type == ExprNodeType::Funcall)
            {
                ParsedArg Args[1] = {ParsedArgType::int32_t};
                Args->pEnumMaps = GetSystemFontEnumMap();

                do
                {
                    Font _Font;
                    auto _hr = ParserFunction("GetSystemFontSize", _pExprNode, Args);
                    if (SUCCEEDED(_hr))
                    {
                        _hr = GetSystemFont((SystemFont)Args[0].iNumber, &_Font);
                        if (FAILED(_hr))
                            return _hr;
                        _pValue->iNumber = (int32_t)_Font.iSize;
                        break;
                    }

                    if (_hr != __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                    {
                        return _hr;
                    }

                    _hr = ParserFunction("GetSystemFontWeight", _pExprNode, Args);
                    if (SUCCEEDED(_hr))
                    {
                        _hr = GetSystemFont((SystemFont)Args[0].iNumber, &_Font);
                        if (FAILED(_hr))
                            return _hr;
                        _pValue->iNumber = (int32_t)_Font.uWeight;
                        break;
                    }

                    if (_hr != __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                    {
                        return _hr;
                    }

                    _hr = ParserFunction("GetSystemFontStyle", _pExprNode, Args);
                    if (SUCCEEDED(_hr))
                    {
                        _hr = GetSystemFont((SystemFont)Args[0].iNumber, &_Font);
                        if (FAILED(_hr))
                            return _hr;
                        _pValue->iNumber = (int32_t)_Font.fStyle;
                        break;
                    }
                    
                    return _hr;
                    
                } while (false);

                return S_OK;
            }
            
            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        
        HRESULT __YYAPI UIParser::ParserStringValue(ExprNode* _pExprNode, uString* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() != 0)
                {
                    auto _hr = ParserStringValue(_pExprNode->ChildExprNode.GetData(), _pValue);
                    if (SUCCEEDED(_hr) || _hr != __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                        return _hr;
                }

                return Transform(_pExprNode->szValue, _pValue);
            }
            else if (_pExprNode->Type == ExprNodeType::BaseIdentifier)
            {
                return Transform(_pExprNode->szValue, _pValue);
            }
            else if (_pExprNode->Type == ExprNodeType::Funcall)
            {
                ParsedArg Args[1] = {ParsedArgType::int32_t};
                Args->pEnumMaps = GetSystemFontEnumMap();

                do
                {
                    Font _Font;
                    auto _hr = ParserFunction("GetSystemFontFamily", _pExprNode, Args);
                    if (SUCCEEDED(_hr))
                    {
                        _hr = GetSystemFont((SystemFont)Args[0].iNumber, &_Font);
                        if (FAILED(_hr))
                            return _hr;
                        *_pValue = _Font.szFace;
                        return S_OK;
                    }
                } while (false);
            }
            
            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }

        HRESULT __YYAPI UIParser::ParserInt32Value(const EnumMap* pEnumMaps, ExprNode* _pExprNode, Value* _pValue)
        {
            ParsedArg _iValue(ParsedArgType::int32_t, pEnumMaps);
            auto _hr = ParserInt32Value(_pExprNode, &_iValue);
            if(FAILED(_hr))
                return _hr;

            if (_iValue.uRawView != 0)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT); 

            auto _Value = Value::CreateInt32(_iValue.iNumber);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;

            *_pValue = std::move(_Value);
            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserFloatValue(ExprNode* _pExprNode, ParsedArg* _pValue)
        {
            _pValue->uRawView = 0;

            // 只接受整形输入，使用浮点只是为了减少运算精度损失
            ParsedArg _iValue(ParsedArgType::int32_t, _pValue->pEnumMaps);
            auto _hr = ParserInt32Value(_pExprNode, &_iValue);
            if (FAILED(_hr))
                return _hr;

            _pValue->FloatNumber = (float)_iValue.iNumber;
            _pValue->Type1 = _iValue.Type1;

            switch (_iValue.Type1)
            {
            case ValueSuffixType::FontPoint:
                _pValue->FloatNumber = PointToPixel(_pValue->FloatNumber, 96);
            case ValueSuffixType::DevicePixel:
                break;
            }

            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserFloatValue(const EnumMap* _pEnumMaps, ExprNode* _pExprNode, Value* _pValue)
        {
            ParsedArg _fValue(ParsedArgType::float_t, _pEnumMaps);
            auto _hr = ParserFloatValue(_pExprNode, &_fValue);
            if (FAILED(_hr))
                return _hr;
            auto _Value = Value::CreateFloat(_fValue.FloatNumber, _fValue.GetSuffix());
            if (_Value == nullptr)
                return E_OUTOFMEMORY;

            *_pValue = std::move(_Value);
            return S_OK;
        }
        
        HRESULT __YYAPI UIParser::ParserBoolValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
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
        
        HRESULT __YYAPI UIParser::ParserStringValue(ExprNode* _pExprNode, Value* _pValue)
        {
            uString _ValueView;
            auto _hr = ParserStringValue(_pExprNode, &_ValueView);
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreateString(_ValueView);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = _Value;
            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserFunction(aStringView _szFunctionName, ExprNode* _pExprNode, ParsedArg* _pArgs, uint_t _uArgsCount)
        {
            if (_pExprNode->Type != ExprNodeType::Funcall)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            if (_pExprNode->szValue.GetSize() != _szFunctionName.GetSize() || _strnicmp((char*)_pExprNode->szValue.GetConstString(), _szFunctionName.GetConstString(), _szFunctionName.GetSize()) != 0)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            if (_pExprNode->ChildExprNode.GetSize() != _uArgsCount)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            HRESULT _hr = S_OK;

            for (auto& ExprNode : _pExprNode->ChildExprNode)
            {
                switch (_pArgs->eArgType)
                {
                case ParsedArgType::None:
                    // 纯占位
                    break;
                case ParsedArgType::int32_t:
                    _hr = ParserInt32Value(&ExprNode, _pArgs);
                    // 整形不支持缩放后缀
                    if (_pArgs->uRawView)
                        _hr = __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    break;
                case ParsedArgType::float_t:
                    _hr = ParserFloatValue(&ExprNode, _pArgs);
                    break;
                case ParsedArgType::uString:
                    _hr = ParserStringValue(&ExprNode, &_pArgs->szString);
                    break;
                case ParsedArgType::Color:
                    _hr = ParserColorValue(&ExprNode, &_pArgs->crColor);
                    break;
                case ParsedArgType::Rect:
                    _hr = ParserRectValue(&ExprNode, _pArgs);
                    break;
                default:
                    _hr = E_NOTIMPL;
                    break;
                }
                
                if (FAILED(_hr))
                    return _hr;
                
                ++_pArgs;
            }

            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserPointValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
            }

            ParsedArg _Arg[2] = {ParsedArgType::int32_t, ParsedArgType::int32_t};

            auto _hr = ParserFunction("Point", _pExprNode, _Arg, std::size(_Arg));
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreatePoint((float)_Arg[0].iNumber, (float)_Arg[1].iNumber);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }
        
        HRESULT __YYAPI UIParser::ParserSizeValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
            }

            ParsedArg _Args[] = {ParsedArgType::float_t, ParsedArgType::float_t};

            auto _hr = ParserFunction("Size", _pExprNode, _Args);
            if (FAILED(_hr))
                return _hr;

            ValueSuffix _Suffix = {_Args[0].Type1, _Args[1].Type1, ValueSuffixType::None, ValueSuffixType::None, 0};
            _Suffix.Dpi = _Suffix.RawView ? 96 : 0;

            auto _Value = Value::CreateSize(_Args[0].FloatNumber, _Args[1].FloatNumber, _Suffix);
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserRectValue(ExprNode* _pExprNode, ParsedArg* _pValue)
        {
            _pValue->uRawView = 0;

            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
            }

            ParsedArg _Args[] =
            {
                ParsedArgType::float_t,
                ParsedArgType::float_t,
                ParsedArgType::float_t,
                ParsedArgType::float_t,
            };

            auto _hr = ParserFunction("Rect", _pExprNode, _Args);
            if (FAILED(_hr))
                return _hr;

            _pValue->rcRect.Left = _Args[0].FloatNumber;
            _pValue->rcRect.Top = _Args[1].FloatNumber;
            _pValue->rcRect.Right = _Args[2].FloatNumber;
            _pValue->rcRect.Bottom = _Args[3].FloatNumber;
            
            _pValue->Type1 = _Args[0].Type1;
            _pValue->Type2 = _Args[1].Type1;
            _pValue->Type3 = _Args[2].Type1;
            _pValue->Type4 = _Args[3].Type1;

            return S_OK;
        }
        
        HRESULT __YYAPI UIParser::ParserRectValue(ExprNode* _pExprNode, Value* _pValue)
        {
            ParsedArg _rcValue(ParsedArgType::Rect);
            auto _hr = ParserRectValue(_pExprNode, &_rcValue);
            if (FAILED(_hr))
                return _hr;

            auto _Value = Value::CreateRect(_rcValue.rcRect, _rcValue.GetSuffix());
            if (_Value == nullptr)
                return E_OUTOFMEMORY;
            *_pValue = std::move(_Value);

            return S_OK;
        }

        HRESULT __YYAPI UIParser::ParserColorValue(ExprNode* _pExprNode, Color* _pValue)
        {
            Color _Color;

            ParsedArg _Arg[] = {ParsedArgType::int32_t, ParsedArgType::int32_t, ParsedArgType::int32_t, ParsedArgType::int32_t};
            auto _hr = ParserFunction("ARGB", _pExprNode, _Arg);
            if (SUCCEEDED(_hr))
            {
                _Color = Color::MakeARGB(_Arg[0].iNumber, _Arg[1].iNumber, _Arg[2].iNumber, _Arg[3].iNumber);
            }
            else if (_hr == __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
            {
                _hr = ParserFunction("RGB", _pExprNode, _Arg, 3);
                if (SUCCEEDED(_hr))
                    _Color = Color::MakeRGB(_Arg[0].iNumber, _Arg[1].iNumber, _Arg[2].iNumber);
            }

            if (FAILED(_hr))
                return _hr;

            *_pValue = _Color;
            return S_OK;
        }
        
        HRESULT __YYAPI UIParser::ParserColorValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
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

        HRESULT __YYAPI UIParser::ParserStyleSheetValue(ExprNode* _pExprNode, Value* _pValue)
        {
            if (_pExprNode->Type == ExprNodeType::Root)
            {
                if (_pExprNode->ChildExprNode.GetSize() == 0)
                    return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                _pExprNode = _pExprNode->ChildExprNode.GetData();
            }

            if (_pExprNode->Type != ExprNodeType::BaseIdentifier)
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

            ComPtr<StyleSheet> _pStyleSheet;
            if(SUCCEEDED(GetStyleSheet(_pExprNode->szValue, &_pStyleSheet)))
            {
                auto _StyleSheetValue = Value::CreateStyleSheet(_pStyleSheet);
                if (_StyleSheetValue == nullptr)
                    return E_OUTOFMEMORY;
                *_pValue = _StyleSheetValue;
                return S_OK;
            }

            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        
        HRESULT __YYAPI UIParser::Play(ArrayView<const uint8_t>& _ByteCode, UIParserPlayContext* _pContext, Element* _pTopLevel, intptr_t* _pCooike, Array<Element*, AllocPolicy::SOO>* _pElementArray)
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

                    Array<Element*, AllocPolicy::SOO> _Child;

                    _hr = Play(_ByteCode, _pContext, _pTopLevel, nullptr, &_Child);
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

                    _hr = _pClass->CreateInstance(_pContext->iDPI, _pTopLevel, _pCooike, &_pCurrentElement);
                    if (FAILED(_hr))
                        break;
                    _pCooike = nullptr;

                    // 第一个创建的作为 TopLevel
                    if (!_pTopLevel)
                        _pTopLevel = _pCurrentElement;

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
                    _pCurrentElement->SetValue(*_pSetProperty->pProp, _ppValue->UpdateDpi(_pCurrentElement->GetDpi()));
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
        
        static ValueCmpOperation __YYAPI TryParserStyleSheetOptionXmlType(
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

        HRESULT __YYAPI UIParser::ParserStyleSheetNode(rapidxml::xml_node<char>* _StyleSheetNode, Array<StyleSheetXmlOption, AllocPolicy::SOO>& _XmlOption, StyleSheet* _pStyleSheet)
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
        
        HRESULT __YYAPI UIParser::ParserStyleSheetElementNode(rapidxml::xml_node<char>* _pElementValueNode, const Array<StyleSheetXmlOption, AllocPolicy::SOO>& _XmlOption, StyleSheet* _pStyleSheet)
        {
            uint32_t _uIndex;

            auto _pControlInfo = FindControlInfo(_pElementValueNode->name(), &_uIndex);
            if (!_pControlInfo)
            {
                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            }

            // 首先将匹配条件 _XmlOption 转换 到 StyleSheet::Cond
            Array<Cond> CondArray;
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
            Array<Decl, AllocPolicy::SOO> _DeclArray;
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