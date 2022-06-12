#include "pch.h"

#include "String.h"

#include <Windows.h>

namespace YY
{
    namespace MegaUI
    {
        __forceinline u16char_t __fastcall byteswap(u16char_t ch)
        {
            return _byteswap_ushort(ch);
        }

        __forceinline u32char_t __fastcall byteswap(u32char_t ch)
        {
            return _byteswap_ulong(ch);
        }

        class EndianHelper
        {
        public:
            template<typename StringA, typename StringB>
            static HRESULT __fastcall TransformEndian(const StringA& szSrc, StringB* pszDst)
            {
                if (!pszDst)
                    return E_INVALIDARG;

                auto cchSrc = szSrc.GetSize();
                if (cchSrc == 0)
                    return S_OK;

                const auto cchOldDst = pszDst->GetSize();

                auto cchDst = cchOldDst + cchSrc;

                auto pszDstBuffer = pszDst->LockBuffer(cchDst);
                if (!pszDstBuffer)
                    return E_OUTOFMEMORY;

                pszDstBuffer += cchOldDst;

                for (auto ch : szSrc)
                {
                    *pszDstBuffer = byteswap(ch);
                    ++pszDstBuffer;
                }

                pszDst->UnlockBuffer(cchDst);

                return S_OK;
            }

            template<typename StringA, typename StringB>
            static HRESULT __fastcall TransformEndian(StringA&& szSrc, StringB* pszDst)
            {
                if (!pszDst)
                    return E_INVALIDARG;

                auto cchSrc = szSrc.GetSize();
                if (cchSrc == 0)
                    return S_OK;

                const auto cchOldDst = pszDst->GetSize();
                auto cchDst = cchOldDst + cchSrc;

                if (cchOldDst == 0)
                {
                    // Dst初始状态为空，所以我们可以尝试使用move语义
                    auto pszSrcBuffer = szSrc.LockBuffer(cchDst);
                    if (!pszSrcBuffer)
                        return E_OUTOFMEMORY;

                    for (uint_t i = 0; i != cchSrc; ++i)
                    {
                        pszSrcBuffer[i] = byteswap(pszSrcBuffer[i]);
                    }

                    szSrc.UnlockBuffer(cchDst);

                    auto pStringData = reinterpret_cast<StringB::StringData*>(szSrc.Detach());
                    pStringData->eEncoding = uint16_t(StringB::eEncoding);
                    pszDst->Attach(pStringData);
                    pStringData->Release();
                }
                else
                {
                    auto pszSrcBuffer = szSrc.GetConstString();
                    auto pszDstBuffer = pszDst->LockBuffer(cchDst);
                    if (!pszDstBuffer)
                        return E_OUTOFMEMORY;

                    pszDstBuffer += cchOldDst;

                    for (uint_t i = 0; i != cchSrc; ++i)
                    {
                        pszDstBuffer[i] = byteswap(pszSrcBuffer[i]);
                    }

                    pszDst->UnlockBuffer(cchDst);
                }
                return S_OK;
            }
        };


        HRESULT __fastcall Transform(const aStringView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_POINTER;
            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }

        HRESULT __fastcall Transform(const aString& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_POINTER;
            
            return pszDst->AppendString(szSrc);
        }

        HRESULT __fastcall Transform(const u8StringView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            if (szSrc.GetSize() == 0)
                return S_OK;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto eEncoding = pszDst->GetEncoding();
            const auto cchOldDst = pszDst->GetSize();


            // 大多数场景，u16字符数量的2倍就足够容纳了。
            auto cchDstBuffer = cchOldDst + cchSrc * 2;

            for (;;)
            {
                auto pszDstBuffer = pszDst->LockBuffer(cchDstBuffer);
                if (!pszDstBuffer)
                {
                    return E_OUTOFMEMORY;
                }

                cchDstBuffer = pszDst->GetCapacity();
                auto cchAppendDst = WideCharToMultiByte(UINT(eEncoding), 0, szSrc.GetConstString(), cchSrc, pszDstBuffer + cchOldDst, cchDstBuffer - cchOldDst, nullptr, nullptr);
                pszDst->UnlockBuffer(cchOldDst + cchAppendDst);

                if (cchAppendDst != 0)
                {
                    break;
                }

                const auto lStatus = GetLastError();

                if (ERROR_INSUFFICIENT_BUFFER != lStatus)
                {
                    return __HRESULT_FROM_WIN32(lStatus);
                }

                cchDstBuffer *= 2;
            }

            return S_OK;
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;         

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, aString* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }


        HRESULT __fastcall Transform(const aStringView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;
            if (szSrc.GetSize() == 0)
                return S_OK;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(szTmp, pszDst);
        }

        HRESULT __fastcall Transform(const u8StringView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }

        HRESULT __fastcall Transform(const u8String& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc);
        }
        
        HRESULT __fastcall Transform(const aStringView& szStr, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            const auto cchSrc = szStr.GetSize();
            if (cchSrc == 0)
                return S_OK;
            const auto cchOldDst = pszDst->GetSize();

            // ANSI转UTF16，其缓冲区不会多于当前UTF16字节数量
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDst)
                return E_OUTOFMEMORY;
            pszDstBuffer += cchOldDst;

            auto cchDst = MultiByteToWideChar(UINT(szStr.GetEncoding()), 0, szStr.GetConstString(), cchSrc, pszDstBuffer, cchSrc);

            pszDst->UnlockBuffer(cchOldDst + cchDst);
            return S_OK;
        }

        HRESULT __fastcall Transform(const aStringView& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            if (szSrc.GetSize())
                return S_OK;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }
        
        HRESULT __fastcall Transform(const aStringView& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            if (szSrc.GetSize() == 0)
                return S_OK;

            u16String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(szTmp, pszDst);
        }

        HRESULT __fastcall Transform(const aStringView& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            if (szSrc.GetSize() == 0)
                return S_OK;

            u32String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }
        
        HRESULT __fastcall Transform(const u8StringView& szSrc, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            const auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16实际字符个数一定 <= u8
            const auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstLast = pszDstBuffer + cchOldDst;
            auto pszSrc = szSrc.GetConstString();
            const auto pszSrcEnd = pszSrc + cchSrc;

            for (; pszSrc < pszSrcEnd;)
            {
                uint8_t ch = *pszSrc;

                if (ch >= 0xF8u)
                {
                    // 理论上，目前的UTF8是不可能出现这样的情况的，也转不了UTF16
                    *pszDstLast++ = '?';
                    ++pszSrc;
                }
                else if (ch >= 0xF0u)
                {
                    // 4字节UTF8编码 -> 2 个u16
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x7) << 18) | ((pszSrc[1] & 0x3F) << 12) | ((pszSrc[2] & 0x3F) << 6) | ((pszSrc[3] & 0x3F) << 0);

                        Tmp -= 0x1'00'00;

                        *pszDstLast += (Tmp >> 10) + 0xD8'00u;
                        *pszDstLast += (Tmp & 0x3'FF) + 0xDC'00u;
                        pszSrc += 4;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }

                }
                else if (ch >= 0xE0u)
                {
                    // 3字节
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += ((pszSrc[0] & 0xF) << 12) | ((pszSrc[1] & 0x3F) << 6) | ((pszSrc[2] & 0x3F) << 0);
                        pszSrc += 3;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xC0u)
                {
                    // 2字节
                    if (pszSrcEnd - pszSrc < 2)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += ((pszSrc[0] & 0x1F) << 6) | ((pszSrc[1] & 0x3F) << 0);
                        pszSrc += 2;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0x80u)
                {
                    // 理论上，这是UTF8的中间字符，不应该出现。
                    *pszDstLast++ = '?';
                    ++pszSrc;
                }
                else
                {
                    // 1个字节
                    *pszDstLast++ = ch;
                    ++pszSrc;
                }
            }

            if(pszSrc != pszSrcEnd)
                *pszDstLast++ = '?';

            pszDst->UnlockBuffer(pszDstLast - pszDstBuffer);
            return S_OK;
        }

        HRESULT __fastcall Transform(const u8StringView& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            const auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16实际字符个数一定 <= u8
            const auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstLast = pszDstBuffer + cchOldDst;
            auto pszSrc = szSrc.GetConstString();
            const auto pszSrcEnd = pszSrc + cchSrc;

            for (; pszSrc < pszSrcEnd;)
            {
                uint8_t ch = *pszSrc;

                if (ch >= 0xF8u)
                {
                    // 理论上，目前的UTF8是不可能出现这样的情况的，也转不了UTF16
                    *pszDstLast++ = _byteswap_ushort('?');
                    ++pszSrc;
                }
                else if (ch >= 0xF0u)
                {
                    // 4字节UTF8编码 -> 2 个u16
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x7) << 18) | ((pszSrc[1] & 0x3F) << 12) | ((pszSrc[2] & 0x3F) << 6) | ((pszSrc[3] & 0x3F) << 0);

                        Tmp -= 0x1'00'00;

                        *pszDstLast += _byteswap_ushort((Tmp >> 10) + 0xD8'00u);
                        *pszDstLast += _byteswap_ushort((Tmp & 0x3'FF) + 0xDC'00u);
                        pszSrc += 4;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }

                }
                else if (ch >= 0xE0u)
                {
                    // 3字节
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += _byteswap_ushort(((pszSrc[0] & 0xF) << 12) | ((pszSrc[1] & 0x3F) << 6) | ((pszSrc[2] & 0x3F) << 0));
                        pszSrc += 3;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xC0u)
                {
                    // 2字节
                    if (pszSrcEnd - pszSrc < 2)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += _byteswap_ushort(((pszSrc[0] & 0x1F) << 6) | ((pszSrc[1] & 0x3F) << 0));
                        pszSrc += 2;
                    }
                    else
                    {
                        *pszDstLast++ = _byteswap_ushort('?');
                        ++pszSrc;
                    }
                }
                else if (ch >= 0x80u)
                {
                    // 理论上，这是UTF8的中间字符，不应该出现。
                    *pszDstLast++ = _byteswap_ushort('?');
                    ++pszSrc;
                }
                else
                {
                    // 1个字节
                    *pszDstLast++ = _byteswap_ushort(ch);
                    ++pszSrc;
                }
            }

            if (pszSrc != pszSrcEnd)
                *pszDstLast++ = _byteswap_ushort('?');

            pszDst->UnlockBuffer(pszDstLast - pszDstBuffer);
            return S_OK;
        }

        HRESULT __fastcall Transform(const u8StringView& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            const auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u32实际字符个数一定 <= u8
            const auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstLast = pszDstBuffer + cchOldDst;
            auto pszSrc = szSrc.GetConstString();
            const auto pszSrcEnd = pszSrc + cchSrc;

            for (; pszSrc < pszSrcEnd;)
            {
                uint8_t ch = *pszSrc;
                // 虽然utf8 最多使用 4字节，但是算法中，我们完整的实现到6字节
                if (ch >= 0xFEu)
                {
                    // 无效的UTF8编码
                    *pszDstLast++ = '?';
                    ++pszSrc;
                }
                else if (ch >= 0xFCu)
                {
                    // 6字节UTF8编码
                    if (pszSrcEnd - pszSrc < 6)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u && (pszSrc[4] & 0xC0u) == 0x80u && (pszSrc[5] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x1) << 30) | ((pszSrc[1] & 0x3F) << 24) | ((pszSrc[2] & 0x3F) << 18) | ((pszSrc[3] & 0x3F) << 12) | ((pszSrc[4] & 0x3F) << 6) | ((pszSrc[5] & 0x3F) << 0);
                        *pszDstLast += Tmp;
                        pszSrc += 6;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xF8u)
                {
                    // 5字节UTF8编码
                    if (pszSrcEnd - pszSrc < 5)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u && (pszSrc[4] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x3) << 24) | ((pszSrc[1] & 0x3F) << 18) | ((pszSrc[2] & 0x3F) << 12) | ((pszSrc[3] & 0x3F) << 6) | ((pszSrc[4] & 0x3F) << 0);
                        *pszDstLast += Tmp;
                        pszSrc += 5;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xF0u)
                {
                    // 4字节UTF8编码
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x7) << 18) | ((pszSrc[1] & 0x3F) << 12) | ((pszSrc[2] & 0x3F) << 6) | ((pszSrc[3] & 0x3F) << 0);
                        *pszDstLast += Tmp;
                        pszSrc += 4;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xE0u)
                {
                    // 3字节
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += ((pszSrc[0] & 0xF) << 12) | ((pszSrc[1] & 0x3F) << 6) | ((pszSrc[2] & 0x3F) << 0);
                        pszSrc += 3;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xC0u)
                {
                    // 2字节
                    if (pszSrcEnd - pszSrc < 2)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += ((pszSrc[0] & 0x1F) << 6) | ((pszSrc[1] & 0x3F) << 0);
                        pszSrc += 2;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0x80u)
                {
                    // 理论上，这是UTF8的中间字符，不应该出现。
                    *pszDstLast++ = '?';
                    ++pszSrc;
                }
                else
                {
                    // 1个字节
                    *pszDstLast++ = ch;
                    ++pszSrc;
                }
            }

            if (pszSrc != pszSrcEnd)
                *pszDstLast++ = '?';

            pszDst->UnlockBuffer(pszDstLast - pszDstBuffer);
            return S_OK;
        }

        HRESULT __fastcall Transform(const u8StringView& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            const auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u32实际字符个数一定 <= u8
            const auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstLast = pszDstBuffer + cchOldDst;
            auto pszSrc = szSrc.GetConstString();
            const auto pszSrcEnd = pszSrc + cchSrc;

            for (; pszSrc < pszSrcEnd;)
            {
                uint8_t ch = *pszSrc;
                // 虽然utf8 最多使用 4字节，但是算法中，我们完整的实现到6字节
                if (ch >= 0xFEu)
                {
                    // 无效的UTF8编码
                    *pszDstLast++ = _byteswap_ulong('?');
                    ++pszSrc;
                }
                else if (ch >= 0xFCu)
                {
                    // 6字节UTF8编码
                    if (pszSrcEnd - pszSrc < 6)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u && (pszSrc[4] & 0xC0u) == 0x80u && (pszSrc[5] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x1) << 30) | ((pszSrc[1] & 0x3F) << 24) | ((pszSrc[2] & 0x3F) << 18) | ((pszSrc[3] & 0x3F) << 12) | ((pszSrc[4] & 0x3F) << 6) | ((pszSrc[5] & 0x3F) << 0);
                        *pszDstLast += _byteswap_ulong(Tmp);
                        pszSrc += 6;
                    }
                    else
                    {
                        *pszDstLast++ = '?';
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xF8u)
                {
                    // 5字节UTF8编码
                    if (pszSrcEnd - pszSrc < 5)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u && (pszSrc[4] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x3) << 24) | ((pszSrc[1] & 0x3F) << 18) | ((pszSrc[2] & 0x3F) << 12) | ((pszSrc[3] & 0x3F) << 6) | ((pszSrc[4] & 0x3F) << 0);
                        *pszDstLast += _byteswap_ulong(Tmp);
                        pszSrc += 5;
                    }
                    else
                    {
                        *pszDstLast++ = _byteswap_ulong('?');
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xF0u)
                {
                    // 4字节UTF8编码
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u && (pszSrc[3] & 0xC0u) == 0x80u)
                    {
                        uint32_t Tmp = ((pszSrc[0] & 0x7) << 18) | ((pszSrc[1] & 0x3F) << 12) | ((pszSrc[2] & 0x3F) << 6) | ((pszSrc[3] & 0x3F) << 0);
                        *pszDstLast += _byteswap_ulong(Tmp);
                        pszSrc += 4;
                    }
                    else
                    {
                        *pszDstLast++ = _byteswap_ulong('?');
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xE0u)
                {
                    // 3字节
                    if (pszSrcEnd - pszSrc < 4)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u && (pszSrc[2] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += _byteswap_ulong(((pszSrc[0] & 0xF) << 12) | ((pszSrc[1] & 0x3F) << 6) | ((pszSrc[2] & 0x3F) << 0));
                        pszSrc += 3;
                    }
                    else
                    {
                        *pszDstLast++ = _byteswap_ulong('?');
                        ++pszSrc;
                    }
                }
                else if (ch >= 0xC0u)
                {
                    // 2字节
                    if (pszSrcEnd - pszSrc < 2)
                        break;

                    if ((pszSrc[1] & 0xC0u) == 0x80u)
                    {
                        *pszDstLast += _byteswap_ulong(((pszSrc[0] & 0x1F) << 6) | ((pszSrc[1] & 0x3F) << 0));
                        pszSrc += 2;
                    }
                    else
                    {
                        *pszDstLast++ = _byteswap_ulong('?');
                        ++pszSrc;
                    }
                }
                else if (ch >= 0x80u)
                {
                    // 理论上，这是UTF8的中间字符，不应该出现。
                    *pszDstLast++ = _byteswap_ulong('?');
                    ++pszSrc;
                }
                else
                {
                    // 1个字节
                    *pszDstLast++ = _byteswap_ulong(ch);
                    ++pszSrc;
                }
            }

            if (pszSrc != pszSrcEnd)
                *pszDstLast++ = '?';

            pszDst->UnlockBuffer(pszDstLast - pszDstBuffer);
            return S_OK;
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16String -> u8String，普遍上，3倍的数量的缓冲区个数是差不多的。
            // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
            auto cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc * 3);

            if (!pszDstBuffer)
            {
                return E_OUTOFMEMORY;
            }

            auto cchDstBuffer = pszDst->GetCapacity();

            uint32_t LastChar = 0;

            for (uint32_t ch : szSrc)
            {
                const auto cchDstNewMax = cchDst + 4;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }


                if (LastChar)
                {
                    if (ch >= 0xDC'00u && ch < 0xE0'00u)
                    {
                        ch = ((LastChar - 0xD8'00u) << 10 | (ch - 0xDC'00u)) + 0x1'00'00u;
                        LastChar = 0;

                        pszDstBuffer[cchDst++] = ((ch >> 18) & 0x07) | 0xF0;
                        pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                        pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                        pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                        continue;
                    }

                    // 上一个字符是未知的，直接输出一个 ?
                    pszDstBuffer[cchDst++] = '?';
                    LastChar = 0;
                }


                if (ch < 0x80u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x8'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 6) & 0x1F) | 0xC0;
                    pszDstBuffer[cchDst++] = ((ch >> 0) & 0x3F) | 0x80;
                }
                else if (ch < 0xD8'00u || ch > 0xE0'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x0F) | 0xE0;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else
                {
                    // 进入辅助平面，第二次才能顺利转换
                    LastChar = ch;
                }
            }

            if (pszDstBuffer)
            {
                // 还有一个不正确的结尾？
                if (LastChar)
                {
                    pszDstBuffer[cchDst++] = '?';
                    LastChar = 0;
                }

                pszDst->UnlockBuffer(cchDst);

                return S_OK;
            }

            pszDst->Clear();

            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16String -> u32String，字符数量不会多余 u16
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstOut = pszDstBuffer + cchOldDst;

            uint32_t LastChar = 0;

            for (uint32_t ch : szSrc)
            {
                if (LastChar)
                {
                    if (ch >= 0xDC'00u && ch < 0xE0'00u)
                    {
                        ch = ((LastChar - 0xD8'00u) << 10 | (ch - 0xDC'00u)) + 0x1'00'00u;
                        LastChar = 0;

                        *pszDstOut++ = ch;
                        continue;
                    }

                    // 上一个字符是未知的，直接输出一个 ?
                    *pszDstOut++ = '?';
                    LastChar = 0;
                }

                if(ch >= 0xD8'00u && ch <= 0xE0'00u)
                {
                    // 进入辅助平面，第二次才能顺利转换
                    LastChar = ch;
                }
                else
                {
                    *pszDstOut++ = ch;
                }
            }

            // 还存在一个不完整的字符
            if(LastChar)
                *pszDstOut++ = '?';

            pszDst->UnlockBuffer(pszDstOut - pszDstBuffer);

            return S_OK;
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            if (szSrc.GetSize() == 0)
                return S_OK;

            u32String szTmp;
            auto hr = Transform(szSrc, &szTmp);
            if (FAILED(hr))
                return hr;

            return Transform(std::move(szTmp), pszDst);
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            // u16String -> u8String，普遍上，3倍的数量的缓冲区个数是差不多的。
            // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
            const auto cchOldDst = pszDst->GetSize();
            auto cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchDst + cchSrc * 3);

            if (!pszDstBuffer)
            {
                return E_OUTOFMEMORY;
            }

            auto cchDstBuffer = pszDst->GetCapacity();

            uint32_t LastChar = 0;

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ushort(ch);

                const auto cchDstNewMax = cchDst + 4;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }


                if (LastChar)
                {
                    if (ch >= 0xDC'00u && ch < 0xE0'00u)
                    {
                        ch = ((LastChar - 0xD8'00u) << 10 | (ch - 0xDC'00u)) + 0x1'00'00u;
                        LastChar = 0;

                        pszDstBuffer[cchDst++] = ((ch >> 18) & 0x07) | 0xF0;
                        pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                        pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                        pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                        continue;
                    }

                    // 上一个字符是未知的，直接输出一个 ?
                    pszDstBuffer[cchDst++] = '?';
                    LastChar = 0;
                }


                if (ch < 0x80u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x8'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 6) & 0x1F) | 0xC0;
                    pszDstBuffer[cchDst++] = ((ch >> 0) & 0x3F) | 0x80;
                }
                else if (ch < 0xD8'00u || ch > 0xE0'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x0F) | 0xE0;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else
                {
                    // 进入辅助平面，第二次才能顺利转换
                    LastChar = ch;
                }
            }

            if (pszDstBuffer)
            {
                // 还有一个不正确的结尾？
                if (LastChar)
                {
                    pszDstBuffer[cchDst++] = '?';
                    LastChar = 0;
                }

                pszDst->UnlockBuffer(cchDst);

                return S_OK;
            }

            if(pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }

        HRESULT __fastcall Transform(const u16StringLEView& szSrc, u16StringBE* pszDst)
        {
            return EndianHelper::TransformEndian(szSrc, pszDst);
        }

        HRESULT __fastcall Transform(u16StringLE&& szSrc, u16StringBE* pszDst)
        {
            return EndianHelper::TransformEndian(std::move(szSrc), pszDst);
        }

        HRESULT __fastcall Transform(const u16StringBE& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc);
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16String -> u32String，字符数量不会多余 u16
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstOut = pszDstBuffer + cchOldDst;

            uint32_t LastChar = 0;

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ushort(ch);

                if (LastChar)
                {
                    if (ch >= 0xDC'00u && ch < 0xE0'00u)
                    {
                        ch = ((LastChar - 0xD8'00u) << 10 | (ch - 0xDC'00u)) + 0x1'00'00u;
                        LastChar = 0;

                        *pszDstOut++ = ch;
                        continue;
                    }

                    // 上一个字符是未知的，直接输出一个 ?
                    *pszDstOut++ = '?';
                    LastChar = 0;
                }

                if (ch >= 0xD8'00u && ch <= 0xE0'00u)
                {
                    // 进入辅助平面，第二次才能顺利转换
                    LastChar = ch;
                }
                else
                {
                    *pszDstOut++ = ch;
                }
            }

            // 还存在一个不完整的字符
            if (LastChar)
                *pszDstOut++ = '?';

            pszDst->UnlockBuffer(pszDstOut - pszDstBuffer);

            return S_OK;
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u16String -> u32String，字符数量不会多余 u16
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto pszDstOut = pszDstBuffer + cchOldDst;

            uint32_t LastChar = 0;

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ushort(ch);

                if (LastChar)
                {
                    if (ch >= 0xDC'00u && ch < 0xE0'00u)
                    {
                        ch = ((LastChar - 0xD8'00u) << 10 | (ch - 0xDC'00u)) + 0x1'00'00u;
                        LastChar = 0;

                        *pszDstOut++ = _byteswap_ulong(ch);
                        continue;
                    }

                    // 上一个字符是未知的，直接输出一个 ?
                    *pszDstOut++ = _byteswap_ulong('?');
                    LastChar = 0;
                }

                if (ch >= 0xD8'00u && ch <= 0xE0'00u)
                {
                    // 进入辅助平面，第二次才能顺利转换
                    LastChar = ch;
                }
                else
                {
                    *pszDstOut++ = _byteswap_ulong(ch);
                }
            }

            // 还存在一个不完整的字符
            if (LastChar)
                *pszDstOut++ = _byteswap_ulong('?');

            pszDst->UnlockBuffer(pszDstOut - pszDstBuffer);

            return S_OK;
        }

        HRESULT __fastcall Transform(const u16StringBEView& szSrc, u16StringLE* pszDst)
        {
            return EndianHelper::TransformEndian(szSrc, pszDst);
        }

        HRESULT __fastcall Transform(const u16StringLE& szSrc, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;
            return pszDst->AppendString(szSrc);
        }

        HRESULT __fastcall Transform(u16StringBE&& szSrc, u16StringLE* pszDst)
        {
            return EndianHelper::TransformEndian(std::move(szSrc), pszDst);
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u32String -> u8String，普遍上，3倍的数量的缓冲区。
            // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc * 3);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                const auto cchDstNewMax = cchDst + 6;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                // UTF8目前最多使用 4个字节，但是为了算法的连续性，目前直接实现到 6个字节。
                if (ch < 0x00'00'00'80u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x00'00'08'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 6) & 0x1F) | 0xC0;
                    pszDstBuffer[cchDst++] = ((ch >> 0) & 0x3F) | 0x80;
                }
                else if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x0F) | 0xE0;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x00'20'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x07) | 0xF0;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x04'00'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 24) & 0x03) | 0xF8;
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x80'00'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 30) & 0x01) | 0xFC;
                    pszDstBuffer[cchDst++] = ((ch >> 24) & 0x3F) | 0xF0;
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else
                {
                    pszDstBuffer[cchDst++] = '?';
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                const auto cchDstNewMax = cchDst + 2;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x00'11'00'00u)
                {
                    ch -= 0x1'00'00u;

                    pszDstBuffer[cchDst++] = (ch >> 10) + 0xD8'00u;
                    pszDstBuffer[cchDst++] = (ch & 0x3'FFu) + 0xDC'00u;
                }
                else
                {
                    // UTF32 不可能出现这样的情况
                    pszDstBuffer[cchDst++] = '?';
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                const auto cchDstNewMax = cchDst + 2;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = _byteswap_ushort(ch);
                }
                else if (ch < 0x00'11'00'00u)
                {
                    ch -= 0x1'00'00u;

                    pszDstBuffer[cchDst++] = _byteswap_ushort((ch >> 10) + 0xD8'00u);
                    pszDstBuffer[cchDst++] = _byteswap_ushort((ch & 0x3'FFu) + 0xDC'00u);
                }
                else
                {
                    // UTF32 不可能出现这样的情况
                    pszDstBuffer[cchDst++] = _byteswap_ushort('?');
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, u32StringBE* pszDst)
        {
            return EndianHelper::TransformEndian(szSrc, pszDst);
        }

        HRESULT __fastcall Transform(u32StringLE&& szSrc, u32StringBE* pszDst)
        {
            return EndianHelper::TransformEndian(std::move(szSrc), pszDst);
        }

        HRESULT __fastcall Transform(const u32StringBE& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc);
        }

        HRESULT __fastcall Transform(const u32StringLEView& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, u8String* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            // u32String -> u8String，普遍上，3倍的数量的缓冲区。
            // 所以我们这里使用一段经验值， 减少内存的重复开辟次数。
            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc * 3);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ulong(ch);

                const auto cchDstNewMax = cchDst + 6;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                // UTF8目前最多使用 4个字节，但是为了算法的连续性，目前直接实现到 6个字节。
                if (ch < 0x00'00'00'80u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x00'00'08'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 6) & 0x1F) | 0xC0;
                    pszDstBuffer[cchDst++] = ((ch >> 0) & 0x3F) | 0x80;
                }
                else if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x0F) | 0xE0;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x00'20'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x07) | 0xF0;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x04'00'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 24) & 0x03) | 0xF8;
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else if (ch < 0x80'00'00'00u)
                {
                    pszDstBuffer[cchDst++] = ((ch >> 30) & 0x01) | 0xFC;
                    pszDstBuffer[cchDst++] = ((ch >> 24) & 0x3F) | 0xF0;
                    pszDstBuffer[cchDst++] = ((ch >> 18) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 12) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 06) & 0x3F) | 0x80;
                    pszDstBuffer[cchDst++] = ((ch >> 00) & 0x3F) | 0x80;
                }
                else
                {
                    pszDstBuffer[cchDst++] = '?';
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, u16StringLE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ulong(ch);

                const auto cchDstNewMax = cchDst + 2;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = ch;
                }
                else if (ch < 0x00'11'00'00u)
                {
                    ch -= 0x1'00'00u;

                    pszDstBuffer[cchDst++] = (ch >> 10) + 0xD8'00u;
                    pszDstBuffer[cchDst++] = (ch & 0x3'FFu) + 0xDC'00u;
                }
                else
                {
                    // UTF32 不可能出现这样的情况
                    pszDstBuffer[cchDst++] = '?';
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, u16StringBE* pszDst)
        {
            if (!pszDst)
                return E_INVALIDARG;

            auto cchSrc = szSrc.GetSize();
            if (cchSrc == 0)
                return S_OK;

            const auto cchOldDst = pszDst->GetSize();

            uint_t cchDst = cchOldDst;
            auto pszDstBuffer = pszDst->LockBuffer(cchOldDst + cchSrc);
            if (!pszDstBuffer)
                return E_OUTOFMEMORY;

            auto cchDstBuffer = pszDst->GetCapacity();

            for (uint32_t ch : szSrc)
            {
                ch = _byteswap_ulong(ch);

                const auto cchDstNewMax = cchDst + 2;
                if (cchDstNewMax > cchDstBuffer)
                {
                    pszDst->UnlockBuffer(cchDst);
                    pszDstBuffer = pszDst->LockBuffer(cchDstNewMax);
                    if (!pszDstBuffer)
                        break;
                    cchDstBuffer = pszDst->GetCapacity();
                }

                if (ch < 0x00'01'00'00u)
                {
                    pszDstBuffer[cchDst++] = _byteswap_ushort(ch);
                }
                else if (ch < 0x00'11'00'00u)
                {
                    ch -= 0x1'00'00u;

                    pszDstBuffer[cchDst++] = _byteswap_ushort((ch >> 10) + 0xD8'00u);
                    pszDstBuffer[cchDst++] = _byteswap_ushort((ch & 0x3'FFu) + 0xDC'00u);
                }
                else
                {
                    // UTF32 不可能出现这样的情况
                    pszDstBuffer[cchDst++] = _byteswap_ushort('?');
                }
            }

            if (pszDstBuffer)
            {
                pszDst->UnlockBuffer(cchDst);
                return S_OK;
            }

            if (pszDst->LockBuffer(cchOldDst))
                pszDst->UnlockBuffer(cchOldDst);
            return E_OUTOFMEMORY;
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, u32StringLE* pszDst)
        {
            return EndianHelper::TransformEndian(szSrc, pszDst);
        }

        HRESULT __fastcall Transform(const u32StringLE& szSrc, u32StringLE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc);
        }

        HRESULT __fastcall Transform(u32StringBE&& szSrc, u32StringLE* pszDst)
        {
            return EndianHelper::TransformEndian(std::move(szSrc), pszDst);
        }

        HRESULT __fastcall Transform(const u32StringBEView& szSrc, u32StringBE* pszDst)
        {
            if (!pszDst)
                return E_POINTER;

            return pszDst->AppendString(szSrc.GetConstString(), szSrc.GetSize());
        }


    }
}