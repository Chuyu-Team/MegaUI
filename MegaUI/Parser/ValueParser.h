#pragma once

#include "../base/MegaUITypeInt.h"
#include "../core/Element.h"
#include "../base/DynamicArray.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        enum class ExprNodeType
        {
            BaseIdentifier = 0,
            Funcall,
            Or,
            Root,
        };

        enum class ValueParserType
        {
            // 提取成功
            ParserSuccess,
            // 提取失败，因为遇到结束
            ParserEnd,
            // 提取失败，遇到其他字符
            ParserOtherChar,
        };

        struct ExprNode
        {
            // 0
            ExprNodeType Type;
            // 4
            u8StringView szValue;

            // 子节点
            DynamicArray<ExprNode> ChildExprNode;
            
            ExprNode()
                : Type(ExprNodeType::Root)
            {
            }

            ExprNode(ExprNodeType _Type, const u8StringView& _szValue)
                : Type(_Type)
                , szValue(_szValue)
            {
            }

            ExprNode(const ExprNode& _Other)
                : Type(_Other.Type)
                , szValue(_Other.szValue)
                , ChildExprNode(_Other.ChildExprNode)
            {
            }

            ExprNode(ExprNode&& _Other) noexcept
                : Type(_Other.Type)
                , szValue(_Other.szValue)
                , ChildExprNode(std::move(_Other.ChildExprNode))
            {
            }


            ExprNode& operator=(const ExprNode& _Other)
            {
                if (this != &_Other)
                {
                    Type = _Other.Type;
                    szValue = _Other.szValue;
                    ChildExprNode.SetArray(_Other.ChildExprNode);
                }

                return *this;
            }
            
            ExprNode& operator=(ExprNode&& _Other) noexcept
            {
                if (this != &_Other)
                {
                    Type = _Other.Type;
                    szValue = _Other.szValue;
                    ChildExprNode.SetArray(std::move(_Other.ChildExprNode));
                }

                return *this;
            }


            // 8
            //int Count;
            //// 0xC
            //union
            //{
            //    ExprNode* pExprNode;
            //    //未就绪时，指向的是一个Index
            //    DWORD nIndex;
            //};
            // 0x10
        };

        struct ValueParserContext
        {
            const u8char_t* szStart = nullptr;
            const u8char_t* szEnd = nullptr;
            const u8char_t* szCurrent = nullptr;
            aStringView TerminateChars;

            const u8char_t* __MEGA_UI_API SkipWhiteSpace()
            {
                for (; IsTerminate() == false; ++szCurrent)
                {
                    if (!(* szCurrent == '\t' || *szCurrent == '\r' || *szCurrent == '\n' || *szCurrent == ' '))
                    {
                        break;
                    }
                }
                
                return szCurrent;
            }

            /// <summary>
            /// 设置新的结束符。只支持纯ASCII字符。
            /// </summary>
            /// <returns>返回上一次的结果。</returns>
            aStringView __MEGA_UI_API SetTerminateChars(aStringView _NewTerminateChars)
            {
                auto _Tmp = TerminateChars;
                TerminateChars = _NewTerminateChars;
                return _Tmp;
            }

            bool __MEGA_UI_API IsTerminate()
            {
                if (szCurrent == szEnd)
                    return true;

                const auto _CurrentChar = *szCurrent;
                for (const auto _ch : TerminateChars)
                {
                    if (_CurrentChar == _ch)
                        return true;
                }

                return false;
            }
            
            // 已经到达末尾，但是 chTerminateChar 没有终止 认为是失败。
            bool __MEGA_UI_API HasError()
            {
                return szCurrent == szEnd && TerminateChars.GetSize();
            }

            u8char_t __MEGA_UI_API Next()
            {
                auto _ch = *szCurrent;
                ++szCurrent;
                return _ch;
            }

            u8char_t __MEGA_UI_API Current()
            {
                return szCurrent == szEnd ? '\0' : *szCurrent;
            }
        };

        class ValueParser
        {
        private:

        public:
            void __MEGA_UI_API Clear()
            {
            }

            HRESULT __MEGA_UI_API Parse(const u8StringView& _szString, ExprNode* _pExprNode)
            {
                if (!_pExprNode)
                    return E_INVALIDARG;

                _pExprNode->ChildExprNode.Clear();
                _pExprNode->szValue = _szString;
                _pExprNode->Type = ExprNodeType::Root;
                ValueParserContext Context;
                Context.szStart = Context.szCurrent = _szString.GetConstString();
                Context.szEnd = _szString.GetConstString() + _szString.GetSize();

                auto _hr = ParseWorker(&Context, _pExprNode);
                if (SUCCEEDED(_hr) || _hr != __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                {
                    return _hr;
                }

                _pExprNode->ChildExprNode.Clear();
                return S_OK;
            }

            HRESULT __MEGA_UI_API ParseWorker(ValueParserContext* _pContext, ExprNode* _pExprNode)
            {
                // 难以预测此节点是否包含 | 逻辑，所以我们首先假设这是 | 逻辑内。
                // 执行完毕后再根据实际情况合并。
                ExprNode ExprNodeOrOption(ExprNodeType::Or, u8StringView());
                auto _hr = ParseOr(_pContext, &ExprNodeOrOption);
                if (FAILED(_hr))
                    return _hr;
                
                auto _uSize = ExprNodeOrOption.ChildExprNode.GetSize();

                if (_uSize == 1)
                {
                    if (!_pExprNode->ChildExprNode.EmplacePtr(std::move(ExprNodeOrOption.ChildExprNode[0])))
                        return E_OUTOFMEMORY;
                }
                else if (_uSize > 1)
                {
                    if (!_pExprNode->ChildExprNode.EmplacePtr(std::move(ExprNodeOrOption)))
                        return E_OUTOFMEMORY;
                }

                return S_OK;
            }


            ValueParserType __MEGA_UI_API ParseIdentifier(ValueParserContext* _pContext, u8StringView* _pszIdentifier)
            {
                if (_pContext->IsTerminate())
                    return ValueParserType::ParserEnd;

                if (*_pContext->szCurrent == '\'')
                {
                    ++_pContext->szCurrent;
                    auto _szIdentifier = _pContext->SkipWhiteSpace();

                    // 以 '' 包裹
                    for (;; ++_pContext->szCurrent)
                    {
                        if (_pContext->IsTerminate())
                            return ValueParserType::ParserOtherChar;

                        auto _ch = *_pContext->szCurrent;

                        if (_ch == '\'')
                            break;
                    }

                    _pszIdentifier->SetString(_szIdentifier, _pContext->szCurrent - _szIdentifier);
                    ++_pContext->szCurrent;

                    return ValueParserType::ParserSuccess;
                }
                else
                {
                    auto _szIdentifier = _pContext->SkipWhiteSpace();
                    // 这是一个简化的 Identifier
                    // - 只允许开头，这可能表示一个负数。
                    if (_pContext->IsTerminate() == false && *_pContext->szCurrent == '-')
                        ++_pContext->szCurrent;

                    for (; _pContext->IsTerminate() == false; ++_pContext->szCurrent)
                    {
                        auto _ch = *_pContext->szCurrent;

                        if (!(_ch >= 'A' && _ch <= 'Z'
                            || _ch >= 'a' && _ch <= 'z'
                            || _ch >= '0' && _ch <= '9'
                            || _ch == '_'))
                        {
                            break;
                        }
                    }

                    if (_szIdentifier != _pContext->szCurrent)
                    {
                        _pszIdentifier->SetString(_szIdentifier, _pContext->szCurrent - _szIdentifier);

                        return ValueParserType::ParserSuccess;
                    }

                    if (_pContext->IsTerminate())
                        return ValueParserType::ParserEnd;
                    else
                        return ValueParserType::ParserOtherChar;
                }
            }

            HRESULT __MEGA_UI_API ParseFuncall(ValueParserContext* _pContext, ExprNode* _pExprNode)
            {
                auto _OldTerminateChars = _pContext->SetTerminateChars(")");

                _pContext->SkipWhiteSpace();

                for (;;)
                {
                    auto _OldTerminateChars = _pContext->SetTerminateChars(",)");
                    auto _hr = ParseWorker(_pContext, _pExprNode);
                    _pContext->SetTerminateChars(_OldTerminateChars);
                    if (FAILED(_hr))
                        return _hr;
                    
                    _pContext->SkipWhiteSpace();

                    if (_pContext->HasError())
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                    if (_pContext->IsTerminate())
                        break;

                    if (_pContext->Current() != ',')
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                    _pContext->Next();
                }
                _pContext->SetTerminateChars(_OldTerminateChars);

                return S_OK;
            }

            // Or 逻辑中
            HRESULT __MEGA_UI_API ParseOr(ValueParserContext* _pContext, ExprNode* _pExprNode)
            {
                _pContext->SkipWhiteSpace();

                u8StringView _szIdentifier;

                for (;;)
                {
                    auto _Type = ParseIdentifier(_pContext, &_szIdentifier);
                    if (_Type == ValueParserType::ParserSuccess)
                    {
                        _pContext->SkipWhiteSpace();

                        if (_pContext->HasError())
                            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                        auto _ch = _pContext->Current();
                        if (_pContext->IsTerminate() || _ch == '|')
                        {
                            auto pChildExprNode = _pExprNode->ChildExprNode.EmplacePtr(ExprNodeType::BaseIdentifier, _szIdentifier);
                            if (!pChildExprNode)
                                return E_OUTOFMEMORY;
                        }
                        else if (_ch == '(')
                        {
                            auto pChildExprNode = _pExprNode->ChildExprNode.EmplacePtr(ExprNodeType::Funcall, _szIdentifier);
                            if (!pChildExprNode)
                                return E_OUTOFMEMORY;
                            
                            _pContext->Next();

                            auto _hr = ParseFuncall(_pContext, pChildExprNode);
                            if (FAILED(_hr))
                                return _hr;

                            _pContext->Next();
                            _pContext->SkipWhiteSpace();

                            if (_pContext->HasError())
                                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        }
                        else
                        {
                            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        }
                    }
                    else if (_Type == ValueParserType::ParserOtherChar)
                    {
                        if (_pContext->Current() == '(')
                        {
                            _pContext->Next();

                            auto _OldTerminateChars = _pContext->SetTerminateChars(")");
                            auto _hr = ParseWorker(_pContext, _pExprNode);
                            _pContext->SetTerminateChars(_OldTerminateChars);

                            if (FAILED(_hr))
                                return _hr;

                            _pContext->Next();
                            _pContext->SkipWhiteSpace();

                            if (_pContext->HasError())
                                return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        }
                        else
                        {
                            return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                        }
                    }
                    else
                    {
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    }
                    if (_pContext->IsTerminate())
                        break;

                    if (_pContext->Current() != '|')
                        return __HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

                    _pContext->Next();
                }

                return S_OK;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
