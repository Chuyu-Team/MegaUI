#include "pch.h"

#include <Base/Strings/StringTransform.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <MegaUI/Base/ErrorCode.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Strings
        {
            inline u16char_t __YYAPI byteswap(u16char_t _ch)
            {
#ifdef _WIN32
                return _byteswap_ushort(_ch);
#else
                return __builtin_bswap16(_ch);
#endif
            }

            inline u32char_t __YYAPI byteswap(u32char_t _ch)
            {
#ifdef _WIN32
                return _byteswap_ulong(_ch);
#else
                return __builtin_bswap32(_ch);
#endif
            }

            class EndianHelper
            {
            public:
                template<typename StringA, typename StringB>
                static HRESULT __YYAPI TransformEndian(_In_ const StringA& _szSrc, _Inout_ StringB* _pszDst)
                {
                    if (!_pszDst)
                        return E_POINTER;

                    auto _cchSrc = _szSrc.GetSize();
                    if (_cchSrc == 0)
                        return S_OK;

                    const auto _cchOldDst = _pszDst->GetSize();

                    auto _cchDst = _cchOldDst + _cchSrc;

                    auto _pszDstBuffer = _pszDst->LockBuffer(_cchDst);
                    if (!_pszDstBuffer)
                        return E_OUTOFMEMORY;

                    _pszDstBuffer += _cchOldDst;

                    for (auto _ch : _szSrc)
                    {
                        *_pszDstBuffer = byteswap(_ch);
                        ++_pszDstBuffer;
                    }

                    _pszDst->UnlockBuffer(_cchDst);

                    return S_OK;
                }

                template<typename StringA, typename StringB>
                static HRESULT __YYAPI TransformEndian(_In_ StringA&& _szSrc, _Inout_ StringB* _pszDst)
                {
                    if (!_pszDst)
                        return E_POINTER;

                    auto _cchSrc = _szSrc.GetSize();
                    if (_cchSrc == 0)
                        return S_OK;

                    const auto _cchOldDst = _pszDst->GetSize();
                    auto _cchDst = _cchOldDst + _cchSrc;

                    if (_cchOldDst == 0)
                    {
                        // Dst初始状态为空，所以我们可以尝试使用move语义
                        auto _szSrcBuffer = _szSrc.LockBuffer(_cchSrc);
                        if (!_szSrcBuffer)
                            return E_OUTOFMEMORY;

                        for (size_t i = 0; i != _cchSrc; ++i)
                        {
                            _szSrcBuffer[i] = byteswap(_szSrcBuffer[i]);
                        }

                        _szSrc.UnlockBuffer(_cchDst);

                        auto _pStringData = reinterpret_cast<typename StringB::StringData*>(_szSrc.Detach());
                        _pStringData->eEncoding = uint16_t(StringB::eEncoding);
                        _pszDst->Attach(_pStringData);
                        _pStringData->Release();
                    }
                    else
                    {
                        auto _szSrcBuffer = _szSrc.GetConstString();
                        auto _szDstBuffer = _pszDst->LockBuffer(_cchDst);
                        if (!_szDstBuffer)
                            return E_OUTOFMEMORY;

                        _szDstBuffer += _cchOldDst;

                        for (size_t i = 0; i != _cchSrc; ++i)
                        {
                            _szDstBuffer[i] = byteswap(_szSrcBuffer[i]);
                        }

                        _pszDst->UnlockBuffer(_cchDst);
                    }
                    return S_OK;
                }
            };


            HRESULT __YYAPI Transform(const aStringView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;
                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

            HRESULT __YYAPI Transform(const aString& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;
            
                return _pszDst->AppendString(_szSrc);
            }

            HRESULT __YYAPI Transform(const u8StringView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                if (_szSrc.GetSize() == 0)
                    return S_OK;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }

#ifdef _WIN32
            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                uint64_t _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                // WinAPI限制，无法处理大于 int32_max。
                if (_cchSrc > YY::Base::int32_max)
                    return E_UNEXPECTED;

                const auto _eEncoding = _pszDst->GetEncoding();
                const uint64_t _cchOldDst = _pszDst->GetSize();


                // 大多数场景，u16字符数量的2倍就足够容纳了。
                auto _cchDstBuffer = _cchOldDst + _cchSrc * 2;

                for (;;)
                {
                    if (_cchDstBuffer >= uint_max)
                    {
                        return E_OUTOFMEMORY;
                    }

                    auto _szDstBuffer = _pszDst->LockBuffer((size_t)_cchDstBuffer);
                    if (!_szDstBuffer)
                    {
                        return E_OUTOFMEMORY;
                    }

                    _cchDstBuffer = _pszDst->GetCapacity();

                    auto _cchOutBufferSize = _cchDstBuffer - _cchOldDst;
                    if (_cchOutBufferSize > YY::Base::int32_max)
                        _cchOutBufferSize = YY::Base::int32_max;

                    auto _cchAppendDst = WideCharToMultiByte(UINT(_eEncoding), 0, _szSrc.GetConstString(), (int)_cchSrc, _szDstBuffer + _cchOldDst, (int)_cchOutBufferSize, nullptr, nullptr);
                    _pszDst->UnlockBuffer((size_t)(_cchOldDst + _cchAppendDst));

                    if (_cchAppendDst != 0)
                    {
                        break;
                    }

                    const auto _ls = GetLastError();

                    if (ERROR_INSUFFICIENT_BUFFER != _ls)
                    {
                        return HRESULT_From_LSTATUS(_ls);
                    }

                    _cchDstBuffer *= 2;
                }

                return S_OK;
            }
#endif

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;         

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }


            HRESULT __YYAPI Transform(const aStringView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;
                if (_szSrc.GetSize() == 0)
                    return S_OK;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(_szTmp, _pszDst);
            }

            HRESULT __YYAPI Transform(const u8StringView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

            HRESULT __YYAPI Transform(const u8String& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc);
            }
        
#ifdef _WIN32
            HRESULT __YYAPI Transform(const aStringView& _szStr, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                const auto _cchSrc = _szStr.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                // WinAPI 限制，无法处理超过 int32_max
                if (_cchSrc > YY::Base::int32_max)
                    return E_UNEXPECTED;

                const auto _cchOldDst = _pszDst->GetSize();

                // ANSI转UTF16，其缓冲区不会多于当前UTF16字节数量
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_pszDst)
                    return E_OUTOFMEMORY;
                _szDstBuffer += _cchOldDst;

                auto _cchDst = MultiByteToWideChar(UINT(_szStr.GetEncoding()), 0, _szStr.GetConstString(), (int)_cchSrc, _szDstBuffer, (int)_cchSrc);

                _pszDst->UnlockBuffer(_cchOldDst + _cchDst);
                return S_OK;
            }
#endif

            HRESULT __YYAPI Transform(const aStringView& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                if (_szSrc.GetSize())
                    return S_OK;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }
        
            HRESULT __YYAPI Transform(const aStringView& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                if (_szSrc.GetSize() == 0)
                    return S_OK;

                u16String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(_szTmp, _pszDst);
            }

            HRESULT __YYAPI Transform(const aStringView& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                if (_szSrc.GetSize() == 0)
                    return S_OK;

                u32String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }
        
            HRESULT __YYAPI Transform(const u8StringView& _szSrc, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                const auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16实际字符个数一定 <= u8
                const auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _szDstLast = _szDstBuffer + _cchOldDst;
                auto _szSrcBuffer = _szSrc.GetConstString();
                const auto _szSrcEnd = _szSrcBuffer + _cchSrc;

                for (; _szSrcBuffer < _szSrcEnd;)
                {
                    uint8_t _ch = *_szSrcBuffer;

                    if (_ch >= 0xF8u)
                    {
                        // 理论上，目前的UTF8是不可能出现这样的情况的，也转不了UTF16
                        *_szDstLast++ = '?';
                        ++_szSrcBuffer;
                    }
                    else if (_ch >= 0xF0u)
                    {
                        // 4字节UTF8编码 -> 2 个u16
                        if (_szSrcEnd - _szSrcBuffer < 4)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x7) << 18) | ((_szSrcBuffer[1] & 0x3F) << 12) | ((_szSrcBuffer[2] & 0x3F) << 6) | ((_szSrcBuffer[3] & 0x3F) << 0);

                            _uTmp -= 0x010000;

                            *_szDstLast++ = u16char_t((_uTmp >> 10) + 0xD800u);
                            *_szDstLast++ = u16char_t((_uTmp & 0x03FF) + 0xDC00u);
                            _szSrcBuffer += 4;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }

                    }
                    else if (_ch >= 0xE0u)
                    {
                        // 3字节
                        if (_szSrcEnd - _szSrcBuffer < 3)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = u16char_t(((_szSrcBuffer[0] & 0xF) << 12) | ((_szSrcBuffer[1] & 0x3F) << 6) | ((_szSrcBuffer[2] & 0x3F) << 0));
                            _szSrcBuffer += 3;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xC0u)
                    {
                        // 2字节
                        if (_szSrcEnd - _szSrcBuffer < 2)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = u16char_t(((_szSrcBuffer[0] & 0x1Fu) << 6) | ((_szSrcBuffer[1] & 0x3Fu) << 0));
                            _szSrcBuffer += 2;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0x80u)
                    {
                        // 理论上，这是UTF8的中间字符，不应该出现。
                        *_szDstLast++ = '?';
                        ++_szSrcBuffer;
                    }
                    else
                    {
                        // 1个字节
                        *_szDstLast++ = _ch;
                        ++_szSrcBuffer;
                    }
                }

                if(_szSrcBuffer != _szSrcEnd)
                    *_szDstLast++ = '?';

                _pszDst->UnlockBuffer(_szDstLast - _szDstBuffer);
                return S_OK;
            }

            HRESULT __YYAPI Transform(const u8StringView& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                const auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16实际字符个数一定 <= u8
                const auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _szDstLast = _szDstBuffer + _cchOldDst;
                auto _szSrcBuffer = _szSrc.GetConstString();
                const auto _szSrcEnd = _szSrcBuffer + _cchSrc;

                for (; _szSrcBuffer < _szSrcEnd;)
                {
                    uint8_t _ch = *_szSrcBuffer;

                    if (_ch >= 0xF8u)
                    {
                        // 理论上，目前的UTF8是不可能出现这样的情况的，也转不了UTF16
                        *_szDstLast++ = byteswap((u16char_t)'?');
                        ++_szSrcBuffer;
                    }
                    else if (_ch >= 0xF0u)
                    {
                        // 4字节UTF8编码 -> 2 个u16
                        if (_szSrcEnd - _szSrcBuffer < 4)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x7) << 18) | ((_szSrcBuffer[1] & 0x3F) << 12) | ((_szSrcBuffer[2] & 0x3F) << 6) | ((_szSrcBuffer[3] & 0x3F) << 0);

                            _uTmp -= 0x010000;

                            *_szDstLast++ = byteswap((u16char_t)((_uTmp >> 10) + 0xD800u));
                            *_szDstLast++ = byteswap((u16char_t)((_uTmp & 0x3FF) + 0xDC00u));
                            _szSrcBuffer += 4;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u16char_t)'?');
                            ++_szSrcBuffer;
                        }

                    }
                    else if (_ch >= 0xE0u)
                    {
                        // 3字节
                        if (_szSrcEnd - _szSrcBuffer < 3)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = byteswap((u16char_t)(((_szSrcBuffer[0] & 0xF) << 12) | ((_szSrcBuffer[1] & 0x3F) << 6) | ((_szSrcBuffer[2] & 0x3F) << 0)));
                            _szSrcBuffer += 3;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u16char_t)'?');
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xC0u)
                    {
                        // 2字节
                        if (_szSrcEnd - _szSrcBuffer < 2)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = byteswap((u16char_t)(((_szSrcBuffer[0] & 0x1F) << 6) | ((_szSrcBuffer[1] & 0x3F) << 0)));
                            _szSrcBuffer += 2;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u16char_t)'?');
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0x80u)
                    {
                        // 理论上，这是UTF8的中间字符，不应该出现。
                        *_szDstLast++ = byteswap((u16char_t)'?');
                        ++_szSrcBuffer;
                    }
                    else
                    {
                        // 1个字节
                        *_szDstLast++ = byteswap((u16char_t)_ch);
                        ++_szSrcBuffer;
                    }
                }

                if (_szSrcBuffer != _szSrcEnd)
                    *_szDstLast++ = byteswap((u16char_t)'?');

                _pszDst->UnlockBuffer(_szDstLast - _szDstBuffer);
                return S_OK;
            }

            HRESULT __YYAPI Transform(const u8StringView& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                const auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u32实际字符个数一定 <= u8
                const auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _szDstLast = _szDstBuffer + _cchOldDst;
                auto _szSrcBuffer = _szSrc.GetConstString();
                const auto _szSrcEnd = _szSrcBuffer + _cchSrc;

                for (; _szSrcBuffer < _szSrcEnd;)
                {
                    uint8_t _ch = *_szSrcBuffer;
                    // 虽然utf8 最多使用 4字节，但是算法中，我们完整的实现到6字节
                    if (_ch >= 0xFEu)
                    {
                        // 无效的UTF8编码
                        *_szDstLast++ = '?';
                        ++_szSrcBuffer;
                    }
                    else if (_ch >= 0xFCu)
                    {
                        // 6字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 6)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u && (_szSrcBuffer[4] & 0xC0u) == 0x80u && (_szSrcBuffer[5] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x1) << 30) | ((_szSrcBuffer[1] & 0x3F) << 24) | ((_szSrcBuffer[2] & 0x3F) << 18) | ((_szSrcBuffer[3] & 0x3F) << 12) | ((_szSrcBuffer[4] & 0x3F) << 6) | ((_szSrcBuffer[5] & 0x3F) << 0);
                            *_szDstLast++ = _uTmp;
                            _szSrcBuffer += 6;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xF8u)
                    {
                        // 5字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 5)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u && (_szSrcBuffer[4] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x3) << 24) | ((_szSrcBuffer[1] & 0x3F) << 18) | ((_szSrcBuffer[2] & 0x3F) << 12) | ((_szSrcBuffer[3] & 0x3F) << 6) | ((_szSrcBuffer[4] & 0x3F) << 0);
                            *_szDstLast++ = _uTmp;
                            _szSrcBuffer += 5;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xF0u)
                    {
                        // 4字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 4)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x7) << 18) | ((_szSrcBuffer[1] & 0x3F) << 12) | ((_szSrcBuffer[2] & 0x3F) << 6) | ((_szSrcBuffer[3] & 0x3F) << 0);
                            *_szDstLast++ = _uTmp;
                            _szSrcBuffer += 4;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xE0u)
                    {
                        // 3字节
                        if (_szSrcEnd - _szSrcBuffer < 3)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = ((_szSrcBuffer[0] & 0xF) << 12) | ((_szSrcBuffer[1] & 0x3F) << 6) | ((_szSrcBuffer[2] & 0x3F) << 0);
                            _szSrcBuffer += 3;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xC0u)
                    {
                        // 2字节
                        if (_szSrcEnd - _szSrcBuffer < 2)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = ((_szSrcBuffer[0] & 0x1F) << 6) | ((_szSrcBuffer[1] & 0x3F) << 0);
                            _szSrcBuffer += 2;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0x80u)
                    {
                        // 理论上，这是UTF8的中间字符，不应该出现。
                        *_szDstLast++ = '?';
                        ++_szSrcBuffer;
                    }
                    else
                    {
                        // 1个字节
                        *_szDstLast++ = _ch;
                        ++_szSrcBuffer;
                    }
                }

                if (_szSrcBuffer != _szSrcEnd)
                    *_szDstLast++ = '?';

                _pszDst->UnlockBuffer(_szDstLast - _szDstBuffer);
                return S_OK;
            }

            HRESULT __YYAPI Transform(const u8StringView& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                const auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u32实际字符个数一定 <= u8
                const auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _szDstLast = _szDstBuffer + _cchOldDst;
                auto _szSrcBuffer = _szSrc.GetConstString();
                const auto _szSrcEnd = _szSrcBuffer + _cchSrc;

                for (; _szSrcBuffer < _szSrcEnd;)
                {
                    uint8_t _ch = *_szSrcBuffer;
                    // 虽然utf8 最多使用 4字节，但是算法中，我们完整的实现到6字节
                    if (_ch >= 0xFEu)
                    {
                        // 无效的UTF8编码
                        *_szDstLast++ = byteswap((u32char_t)'?');
                        ++_szSrcBuffer;
                    }
                    else if (_ch >= 0xFCu)
                    {
                        // 6字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 6)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u && (_szSrcBuffer[4] & 0xC0u) == 0x80u && (_szSrcBuffer[5] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x1) << 30) | ((_szSrcBuffer[1] & 0x3F) << 24) | ((_szSrcBuffer[2] & 0x3F) << 18) | ((_szSrcBuffer[3] & 0x3F) << 12) | ((_szSrcBuffer[4] & 0x3F) << 6) | ((_szSrcBuffer[5] & 0x3F) << 0);
                            *_szDstLast++ = byteswap((u32char_t)(_uTmp));
                            _szSrcBuffer += 6;
                        }
                        else
                        {
                            *_szDstLast++ = '?';
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xF8u)
                    {
                        // 5字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 5)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u && (_szSrcBuffer[4] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x3) << 24) | ((_szSrcBuffer[1] & 0x3F) << 18) | ((_szSrcBuffer[2] & 0x3F) << 12) | ((_szSrcBuffer[3] & 0x3F) << 6) | ((_szSrcBuffer[4] & 0x3F) << 0);
                            *_szDstLast++ = byteswap((u32char_t)(_uTmp));
                            _szSrcBuffer += 5;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u32char_t)('?'));
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xF0u)
                    {
                        // 4字节UTF8编码
                        if (_szSrcEnd - _szSrcBuffer < 4)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u && (_szSrcBuffer[3] & 0xC0u) == 0x80u)
                        {
                            uint32_t _uTmp = ((_szSrcBuffer[0] & 0x7) << 18) | ((_szSrcBuffer[1] & 0x3F) << 12) | ((_szSrcBuffer[2] & 0x3F) << 6) | ((_szSrcBuffer[3] & 0x3F) << 0);
                            *_szDstLast++ = byteswap((u32char_t)(_uTmp));
                            _szSrcBuffer += 4;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u32char_t)('?'));
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xE0u)
                    {
                        // 3字节
                        if (_szSrcEnd - _szSrcBuffer < 3)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u && (_szSrcBuffer[2] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = byteswap((u32char_t)(((_szSrcBuffer[0] & 0xF) << 12) | ((_szSrcBuffer[1] & 0x3F) << 6) | ((_szSrcBuffer[2] & 0x3F) << 0)));
                            _szSrcBuffer += 3;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u32char_t)('?'));
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0xC0u)
                    {
                        // 2字节
                        if (_szSrcEnd - _szSrcBuffer < 2)
                            break;

                        if ((_szSrcBuffer[1] & 0xC0u) == 0x80u)
                        {
                            *_szDstLast++ = byteswap((u32char_t)(((_szSrcBuffer[0] & 0x1F) << 6) | ((_szSrcBuffer[1] & 0x3F) << 0)));
                            _szSrcBuffer += 2;
                        }
                        else
                        {
                            *_szDstLast++ = byteswap((u32char_t)('?'));
                            ++_szSrcBuffer;
                        }
                    }
                    else if (_ch >= 0x80u)
                    {
                        // 理论上，这是UTF8的中间字符，不应该出现。
                        *_szDstLast++ = byteswap((u32char_t)('?'));
                        ++_szSrcBuffer;
                    }
                    else
                    {
                        // 1个字节
                        *_szDstLast++ = byteswap((u32char_t)(_ch));
                        ++_szSrcBuffer;
                    }
                }

                if (_szSrcBuffer != _szSrcEnd)
                    *_szDstLast++ = '?';

                _pszDst->UnlockBuffer(_szDstLast - _szDstBuffer);
                return S_OK;
            }

            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16String -> u8String，普遍上，3倍的数量的缓冲区个数是差不多的。
                // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
                auto _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc * 3);

                if (!_szDstBuffer)
                {
                    return E_OUTOFMEMORY;
                }

                auto _cchDstBuffer = _pszDst->GetCapacity();

                uint32_t _uLastChar = 0;

                for (uint32_t _ch : _szSrc)
                {
                    const auto cchDstNewMax = _cchDst + 4;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }


                    if (_uLastChar)
                    {
                        if (_ch >= 0xDC00u && _ch < 0xE000u)
                        {
                            _ch = ((_uLastChar - 0xD800u) << 10 | (_ch - 0xDC00u)) + 0x010000u;
                            _uLastChar = 0;

                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x07) | 0xF0);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                            continue;
                        }

                        // 上一个字符是未知的，直接输出一个 ?
                        _szDstBuffer[_cchDst++] = '?';
                        _uLastChar = 0;
                    }


                    if (_ch < 0x80u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(_ch);
                    }
                    else if (_ch < 0x0800u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 6) & 0x1F) | 0xC0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 0) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0xD800u || _ch > 0xE000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x0F) | 0xE0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else
                    {
                        // 进入辅助平面，第二次才能顺利转换
                        _uLastChar = _ch;
                    }
                }

                if (_szDstBuffer)
                {
                    // 还有一个不正确的结尾？
                    if (_uLastChar)
                    {
                        _szDstBuffer[_cchDst++] = '?';
                        _uLastChar = 0;
                    }

                    _pszDst->UnlockBuffer(_cchDst);

                    return S_OK;
                }

                _pszDst->Clear();

                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16String -> u32String，字符数量不会多余 u16
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto pszDstOut = _szDstBuffer + _cchOldDst;

                uint32_t _uLastChar = 0;

                for (uint32_t _ch : _szSrc)
                {
                    if (_uLastChar)
                    {
                        if (_ch >= 0xDC00u && _ch < 0xE000u)
                        {
                            _ch = ((_uLastChar - 0xD800u) << 10 | (_ch - 0xDC00u)) + 0x010000u;
                            _uLastChar = 0;

                            *pszDstOut++ = _ch;
                            continue;
                        }

                        // 上一个字符是未知的，直接输出一个 ?
                        *pszDstOut++ = '?';
                        _uLastChar = 0;
                    }

                    if(_ch >= 0xD800u && _ch <= 0xE000u)
                    {
                        // 进入辅助平面，第二次才能顺利转换
                        _uLastChar = _ch;
                    }
                    else
                    {
                        *pszDstOut++ = _ch;
                    }
                }

                // 还存在一个不完整的字符
                if(_uLastChar)
                    *pszDstOut++ = '?';

                _pszDst->UnlockBuffer(pszDstOut - _szDstBuffer);

                return S_OK;
            }

            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                if (_szSrc.GetSize() == 0)
                    return S_OK;

                u32String _szTmp;
                auto _hr = Transform(_szSrc, &_szTmp);
                if (FAILED(_hr))
                    return _hr;

                return Transform(std::move(_szTmp), _pszDst);
            }

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                // u16String -> u8String，普遍上，3倍的数量的缓冲区个数是差不多的。
                // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
                const auto _cchOldDst = _pszDst->GetSize();
                auto _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchDst + _cchSrc * 3);

                if (!_szDstBuffer)
                {
                    return E_OUTOFMEMORY;
                }

                auto _cchDstBuffer = _pszDst->GetCapacity();

                uint32_t _uLastChar = 0;

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u16char_t)_ch);

                    const auto cchDstNewMax = _cchDst + 4;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }


                    if (_uLastChar)
                    {
                        if (_ch >= 0xDC00u && _ch < 0xE000u)
                        {
                            _ch = ((_uLastChar - 0xD800u) << 10 | (_ch - 0xDC00u)) + 0x010000u;
                            _uLastChar = 0;

                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x07) | 0xF0);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                            _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                            continue;
                        }

                        // 上一个字符是未知的，直接输出一个 ?
                        _szDstBuffer[_cchDst++] = '?';
                        _uLastChar = 0;
                    }


                    if (_ch < 0x80u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(_ch);
                    }
                    else if (_ch < 0x0800u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 6) & 0x1F) | 0xC0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 0) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0xD800u || _ch > 0xE000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x0F) | 0xE0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else
                    {
                        // 进入辅助平面，第二次才能顺利转换
                        _uLastChar = _ch;
                    }
                }

                if (_szDstBuffer)
                {
                    // 还有一个不正确的结尾？
                    if (_uLastChar)
                    {
                        _szDstBuffer[_cchDst++] = '?';
                        _uLastChar = 0;
                    }

                    _pszDst->UnlockBuffer(_cchDst);

                    return S_OK;
                }

                if(_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

            HRESULT __YYAPI Transform(const u16StringLEView& _szSrc, u16StringBE* _pszDst)
            {
                return EndianHelper::TransformEndian(_szSrc, _pszDst);
            }

            HRESULT __YYAPI Transform(u16StringLE&& _szSrc, u16StringBE* _pszDst)
            {
                return EndianHelper::TransformEndian(std::move(_szSrc), _pszDst);
            }

            HRESULT __YYAPI Transform(const u16StringBE& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc);
            }

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16String -> u32String，字符数量不会多余 u16
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto pszDstOut = _szDstBuffer + _cchOldDst;

                uint32_t _uLastChar = 0;

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u16char_t)_ch);

                    if (_uLastChar)
                    {
                        if (_ch >= 0xDC00u && _ch < 0xE000u)
                        {
                            _ch = ((_uLastChar - 0xD800u) << 10 | (_ch - 0xDC00u)) + 0x010000u;
                            _uLastChar = 0;

                            *pszDstOut++ = _ch;
                            continue;
                        }

                        // 上一个字符是未知的，直接输出一个 ?
                        *pszDstOut++ = '?';
                        _uLastChar = 0;
                    }

                    if (_ch >= 0xD800u && _ch <= 0xE000u)
                    {
                        // 进入辅助平面，第二次才能顺利转换
                        _uLastChar = _ch;
                    }
                    else
                    {
                        *pszDstOut++ = _ch;
                    }
                }

                // 还存在一个不完整的字符
                if (_uLastChar)
                    *pszDstOut++ = '?';

                _pszDst->UnlockBuffer(pszDstOut - _szDstBuffer);

                return S_OK;
            }

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u16String -> u32String，字符数量不会多余 u16
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto pszDstOut = _szDstBuffer + _cchOldDst;

                uint32_t _uLastChar = 0;

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u16char_t)_ch);

                    if (_uLastChar)
                    {
                        if (_ch >= 0xDC00u && _ch < 0xE000u)
                        {
                            _ch = ((_uLastChar - 0xD800u) << 10 | (_ch - 0xDC00u)) + 0x010000u;
                            _uLastChar = 0;

                            *pszDstOut++ = byteswap((u32char_t)(_ch));
                            continue;
                        }

                        // 上一个字符是未知的，直接输出一个 ?
                        *pszDstOut++ = byteswap((u32char_t)('?'));
                        _uLastChar = 0;
                    }

                    if (_ch >= 0xD800u && _ch <= 0xE000u)
                    {
                        // 进入辅助平面，第二次才能顺利转换
                        _uLastChar = _ch;
                    }
                    else
                    {
                        *pszDstOut++ = byteswap((u32char_t)(_ch));
                    }
                }

                // 还存在一个不完整的字符
                if (_uLastChar)
                    *pszDstOut++ = byteswap((u32char_t)('?'));

                _pszDst->UnlockBuffer(pszDstOut - _szDstBuffer);

                return S_OK;
            }

            HRESULT __YYAPI Transform(const u16StringBEView& _szSrc, u16StringLE* _pszDst)
            {
                return EndianHelper::TransformEndian(_szSrc, _pszDst);
            }

            HRESULT __YYAPI Transform(const u16StringLE& _szSrc, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;
                return _pszDst->AppendString(_szSrc);
            }

            HRESULT __YYAPI Transform(u16StringBE&& _szSrc, u16StringLE* _pszDst)
            {
                return EndianHelper::TransformEndian(std::move(_szSrc), _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u32String -> u8String，普遍上，3倍的数量的缓冲区。
                // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc * 3);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    const auto cchDstNewMax = _cchDst + 6;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    // UTF8目前最多使用 4个字节，但是为了算法的连续性，目前直接实现到 6个字节。
                    if (_ch < 0x80u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(_ch);
                    }
                    else if (_ch < 0x0800u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 6) & 0x1F) | 0xC0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 0) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x010000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x0F) | 0xE0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x200000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x07) | 0xF0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x04000000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 24) & 0x03) | 0xF8);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x80000000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 30) & 0x01) | 0xFC);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 24) & 0x3F) | 0xF0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else
                    {
                        _szDstBuffer[_cchDst++] = '?';
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    const auto cchDstNewMax = _cchDst + 2;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    if (_ch < 0x00010000u)
                    {
                        _szDstBuffer[_cchDst++] = u16char_t(_ch);
                    }
                    else if (_ch < 0x00110000u)
                    {
                        _ch -= 0x10000u;

                        _szDstBuffer[_cchDst++] = u8char_t((_ch >> 10) + 0xD800u);
                        _szDstBuffer[_cchDst++] = u8char_t((_ch & 0x03FFu) + 0xDC00u);
                    }
                    else
                    {
                        // UTF32 不可能出现这样的情况
                        _szDstBuffer[_cchDst++] = '?';
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    const auto cchDstNewMax = _cchDst + 2;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    if (_ch < 0x00010000u)
                    {
                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)_ch);
                    }
                    else if (_ch < 0x00110000u)
                    {
                        _ch -= 0x010000u;

                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)((_ch >> 10) + 0xD800u));
                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)((_ch & 0x03FFu) + 0xDC00u));
                    }
                    else
                    {
                        // UTF32 不可能出现这样的情况
                        _szDstBuffer[_cchDst++] = '?';
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, u32StringBE* _pszDst)
            {
                return EndianHelper::TransformEndian(_szSrc, _pszDst);
            }

            HRESULT __YYAPI Transform(u32StringLE&& _szSrc, u32StringBE* _pszDst)
            {
                return EndianHelper::TransformEndian(std::move(_szSrc), _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringBE& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc);
            }

            HRESULT __YYAPI Transform(const u32StringLEView& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                // u32String -> u8String，普遍上，3倍的数量的缓冲区。
                // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc * 3);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u32char_t)(_ch));

                    const auto cchDstNewMax = _cchDst + 6;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    // UTF8目前最多使用 4个字节，但是为了算法的连续性，目前直接实现到 6个字节。
                    if (_ch < 0x00000080u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(_ch);
                    }
                    else if (_ch < 0x00000800u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 6) & 0x1F) | 0xC0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 0) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x00010000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x0F) | 0xE0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x00200000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x07) | 0xF0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x04000000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 24) & 0x03) | 0xF8);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else if (_ch < 0x80000000u)
                    {
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 30) & 0x01) | 0xFC);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 24) & 0x3F) | 0xF0);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 18) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 12) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 06) & 0x3F) | 0x80);
                        _szDstBuffer[_cchDst++] = u8char_t(((_ch >> 00) & 0x3F) | 0x80);
                    }
                    else
                    {
                        _szDstBuffer[_cchDst++] = '?';
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u32char_t)(_ch));

                    const auto cchDstNewMax = _cchDst + 2;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    if (_ch < 0x00010000u)
                    {
                        _szDstBuffer[_cchDst++] = u16char_t(_ch);
                    }
                    else if (_ch < 0x00110000u)
                    {
                        _ch -= 0x010000u;

                        _szDstBuffer[_cchDst++] = u16char_t((_ch >> 10) + 0xD800u);
                        _szDstBuffer[_cchDst++] = u16char_t((_ch & 0x03FFu) + 0xDC00u);
                    }
                    else
                    {
                        // UTF32 不可能出现这样的情况
                        _szDstBuffer[_cchDst++] = '?';
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                auto _cchSrc = _szSrc.GetSize();
                if (_cchSrc == 0)
                    return S_OK;

                const auto _cchOldDst = _pszDst->GetSize();

                size_t _cchDst = _cchOldDst;
                auto _szDstBuffer = _pszDst->LockBuffer(_cchOldDst + _cchSrc);
                if (!_szDstBuffer)
                    return E_OUTOFMEMORY;

                auto _cchDstBuffer = _pszDst->GetCapacity();

                for (uint32_t _ch : _szSrc)
                {
                    _ch = byteswap((u32char_t)(_ch));

                    const auto cchDstNewMax = _cchDst + 2;
                    if (cchDstNewMax > _cchDstBuffer)
                    {
                        _pszDst->UnlockBuffer(_cchDst);
                        _szDstBuffer = _pszDst->LockBuffer(cchDstNewMax);
                        if (!_szDstBuffer)
                            break;
                        _cchDstBuffer = _pszDst->GetCapacity();
                    }

                    if (_ch < 0x00010000u)
                    {
                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)_ch);
                    }
                    else if (_ch < 0x00110000u)
                    {
                        _ch -= 0x010000u;

                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)((_ch >> 10) + 0xD800u));
                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)((_ch & 0x03FFu) + 0xDC00u));
                    }
                    else
                    {
                        // UTF32 不可能出现这样的情况
                        _szDstBuffer[_cchDst++] = byteswap((u16char_t)'?');
                    }
                }

                if (_szDstBuffer)
                {
                    _pszDst->UnlockBuffer(_cchDst);
                    return S_OK;
                }

                if (_pszDst->LockBuffer(_cchOldDst))
                    _pszDst->UnlockBuffer(_cchOldDst);
                return E_OUTOFMEMORY;
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, u32StringLE* _pszDst)
            {
                return EndianHelper::TransformEndian(_szSrc, _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringLE& _szSrc, u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc);
            }

            HRESULT __YYAPI Transform(u32StringBE&& _szSrc, u32StringLE* _pszDst)
            {
                return EndianHelper::TransformEndian(std::move(_szSrc), _pszDst);
            }

            HRESULT __YYAPI Transform(const u32StringBEView& _szSrc, u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                return _pszDst->AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
            }

        }
    }
}
