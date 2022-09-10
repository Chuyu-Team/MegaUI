#pragma once

#include "StringBase.h"
#include "StringTransform.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // 万能String类
        class String
        {
        private:
            union
            {
                aString szANSI;
                u8String szUTF8;
                u16String szUTF16;
                u16StringLE szUTF16LE;
                u16StringBE szUTF16BE;
                u32String szUTF32;
                u32StringLE szUTF32LE;
                u32StringBE szUTF32BE;
                wString szUTFW;
            };

        public:
            String()
                : szANSI()
            {
            }

            String(const aString& _szSrc)
                : szANSI(_szSrc)
            {
            }

            String(aString&& _szSrc)
                : szANSI(std::move(_szSrc))
            {
            }

            String(const u8String& _szSrc)
                : szUTF8(_szSrc)
            {
            }

            String(u8String&& _szSrc)
                : szUTF8(std::move(_szSrc))
            {
            }

            String(_In_reads_opt_(_cchSrc) const wchar_t* _szSrc, _In_ uint_t _cchSrc)
                : szUTFW(_szSrc, _cchSrc)
            {
            }

            String(_In_opt_z_ const wchar_t* _szSrc)
                : szUTFW(_szSrc)
            {
            }

            template<uint_t _uArrayCount>
            String(const wchar_t (&_szSrc)[_uArrayCount])
                : szUTFW(_szSrc, _uArrayCount - 1)
            {
            }

            String(_In_reads_opt_(_cchSrc) const char16_t* _szSrc, _In_ uint_t _cchSrc)
                : szUTF16((u16char_t*)_szSrc, _cchSrc)
            {
            }

            String(_In_opt_z_ const char16_t* _szSrc)
                : szUTF16((u16char_t*)_szSrc)
            {
            }

            template<uint_t _uArrayCount>
            String(const char16_t (&_szSrc)[_uArrayCount])
                : szUTF16((u16char_t*)_szSrc, _uArrayCount - 1)
            {
            }

            String(_In_reads_opt_(_cchSrc) const char32_t* _szSrc, _In_ uint_t _cchSrc)
                : szUTF32((u32char_t*)_szSrc, _cchSrc)
            {
            }

            String(_In_opt_z_ const char32_t* _szSrc)
                : szUTF32((u32char_t*)_szSrc)
            {
            }

            template<uint_t _uArrayCount>
            String(const char32_t (&_szSrc)[_uArrayCount])
                : szUTF32((u32char_t*)_szSrc, _uArrayCount - 1)
            {
            }

            String(const u16StringLE& _szSrc)
                : szUTF16LE(_szSrc)
            {
            }

            String(u16StringLE&& _szSrc)
                : szUTF16LE(std::move(_szSrc))
            {
            }

            String(const u16StringBE& _szSrc)
                : szUTF16BE(_szSrc)
            {
            }

            String(u16StringBE&& _szSrc)
                : szUTF16BE(std::move(_szSrc))
            {
            }

            String(const u32StringView& _szSrc)
                : szUTF32(_szSrc.GetConstString(), _szSrc.GetSize())
            {
            }

            String(const u32StringLE& _szSrc)
                : szUTF32LE(_szSrc)
            {
            }

            String(u32StringLE&& _szSrc)
                : szUTF32LE(std::move(_szSrc))
            {
            }

            String(const u32StringBE& _szSrc)
                : szUTF32BE(_szSrc)
            {
            }

            String(u32StringBE&& _szSrc)
                : szUTF32BE(std::move(_szSrc))
            {
            }

            ~String()
            {
                szANSI.Detach()->Release();
            }

            Encoding __MEGA_UI_API GetEncoding()
            {
                return Encoding(GetInternalStringData()->eEncoding);
            }

            HRESULT __MEGA_UI_API SetEncoding(Encoding _eEncoding)
            {
                auto _pStringData = GetInternalStringData();
                const auto _eOldEncoding = Encoding(_pStringData->eEncoding);

                if (_eOldEncoding == _eEncoding)
                    return S_OK;

                if (_pStringData->uSize == 0)
                {
                    ClearAndUpdateEncoding(_eEncoding);

                    if (_pStringData->IsShared() == false)
                    {
                        // 当数据独占时，我们直接更新 eEncoding 值即可。
                        const auto OldCharSize = GetCharSize(_eOldEncoding);
                        const auto NewCharSize = GetCharSize(_eEncoding);

                        if (OldCharSize != NewCharSize)
                        {
                            _pStringData->uCapacity = (_pStringData->uCapacity + 1) * OldCharSize / NewCharSize - 1;
                        }

                        _pStringData->eEncoding = (uint16_t)_eEncoding;
                        return S_OK;
                    }
                    else
                    {
                        // 尝试重置到默认值
                        switch (_eEncoding)
                        {
                        case YY::MegaUI::Encoding::UTF16LE:
                            szUTF16LE.Detach()->Release();
                            return S_OK;
                        case YY::MegaUI::Encoding::UTF16BE:
                            szUTF16BE.Detach()->Release();
                            return S_OK;
                        case YY::MegaUI::Encoding::UTF32LE:
                            szUTF32LE.Detach()->Release();
                            return S_OK;
                        case YY::MegaUI::Encoding::UTF32BE:
                            szUTF32BE.Detach()->Release();
                            return S_OK;
                        case YY::MegaUI::Encoding::UTF8:
                            szUTF8.Detach()->Release();
                            return S_OK;
                        default:
                            szANSI.Detach()->Release();
                            return szANSI.SetANSIEncoding(_eEncoding);
                        }
                    }
                }

                switch (_eOldEncoding)
                {
                case YY::MegaUI::Encoding::UTF8:
                {
                    auto _szSrc(std::move(szUTF8));

                    if (_eEncoding == Encoding::UTF16LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16LE);
                    }
                    else if (_eEncoding == Encoding::UTF16BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16BE);
                    }
                    else if (_eEncoding == Encoding::UTF32LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32LE);
                    }
                    else if (_eEncoding == Encoding::UTF32BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32BE);
                    }
                    else
                    {
                        // ANSI
                        szANSI.Clear();
                        if (!szANSI.LockBuffer(_szSrc.GetSize() * 2))
                            return E_OUTOFMEMORY;
                        szANSI.UnlockBuffer(0);
                        szANSI.SetANSIEncoding(_eEncoding);

                        return Transform(std::move(_szSrc), &szANSI);
                    }
                }
                case YY::MegaUI::Encoding::UTF16LE:
                {
                    auto _szSrc(std::move(szUTF16LE));

                    if (_eEncoding == Encoding::UTF8)
                    {
                        return Transform(std::move(_szSrc), &szUTF8);
                    }
                    else if (_eEncoding == Encoding::UTF16BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16BE);
                    }
                    else if (_eEncoding == Encoding::UTF32LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32LE);
                    }
                    else if (_eEncoding == Encoding::UTF32BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32BE);
                    }
                    else
                    {
                        // ANSI
                        szANSI.Clear();
                        if (!szANSI.LockBuffer(_szSrc.GetSize() * 2))
                            return E_OUTOFMEMORY;
                        szANSI.UnlockBuffer(0);
                        szANSI.SetANSIEncoding(_eEncoding);

                        return Transform(std::move(_szSrc), &szANSI);
                    }
                }
                case YY::MegaUI::Encoding::UTF16BE:
                {
                    auto _szSrc(std::move(szUTF16BE));

                    if (_eEncoding == Encoding::UTF8)
                    {
                        return Transform(std::move(_szSrc), &szUTF8);
                    }
                    else if (_eEncoding == Encoding::UTF16LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16LE);
                    }
                    else if (_eEncoding == Encoding::UTF32LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32LE);
                    }
                    else if (_eEncoding == Encoding::UTF32BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32BE);
                    }
                    else
                    {
                        // ANSI
                        szANSI.Clear();
                        if (!szANSI.LockBuffer(_szSrc.GetSize() * 2))
                            return E_OUTOFMEMORY;
                        szANSI.UnlockBuffer(0);
                        szANSI.SetANSIEncoding(_eEncoding);
                        return Transform(std::move(_szSrc), &szANSI);
                    }
                }
                case YY::MegaUI::Encoding::UTF32LE:
                {
                    auto _szSrc(std::move(szUTF32LE));

                    if (_eEncoding == Encoding::UTF8)
                    {
                        return Transform(std::move(_szSrc), &szUTF8);
                    }
                    else if (_eEncoding == Encoding::UTF16LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16LE);
                    }
                    else if (_eEncoding == Encoding::UTF16BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16BE);
                    }
                    else if (_eEncoding == Encoding::UTF32BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32BE);
                    }
                    else
                    {
                        // ANSI
                        szANSI.Clear();
                        if (!szANSI.LockBuffer(_szSrc.GetSize() * 2))
                            return E_OUTOFMEMORY;
                        szANSI.UnlockBuffer(0);
                        szANSI.SetANSIEncoding(_eEncoding);
                        return Transform(std::move(_szSrc), &szANSI);
                    }
                }
                case YY::MegaUI::Encoding::UTF32BE:
                {
                    auto _szSrc(std::move(szUTF32BE));

                    if (_eEncoding == Encoding::UTF8)
                    {
                        return Transform(std::move(_szSrc), &szUTF8);
                    }
                    else if (_eEncoding == Encoding::UTF16LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16LE);
                    }
                    else if (_eEncoding == Encoding::UTF16BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16BE);
                    }
                    else if (_eEncoding == Encoding::UTF32LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32LE);
                    }
                    else
                    {
                        // ANSI
                        szANSI.Clear();
                        if (!szANSI.LockBuffer(_szSrc.GetSize() * 2))
                            return E_OUTOFMEMORY;
                        szANSI.UnlockBuffer(0);
                        szANSI.SetANSIEncoding(_eEncoding);
                        return Transform(std::move(_szSrc), &szANSI);
                    }
                }
                default:
                {
                    // ANSI
                    auto _szSrc(std::move(szANSI));

                    if (_eEncoding == Encoding::UTF8)
                    {
                        return Transform(std::move(_szSrc), &szUTF8);
                    }
                    else if (_eEncoding == Encoding::UTF16LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16LE);
                    }
                    else if (_eEncoding == Encoding::UTF16BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF16BE);
                    }
                    else if (_eEncoding == Encoding::UTF32LE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32LE);
                    }
                    else if (_eEncoding == Encoding::UTF32BE)
                    {
                        return Transform(std::move(_szSrc), &szUTF32BE);
                    }
                    else
                    {
                        // ANSI
                        auto _hr = _szSrc.SetANSIEncoding(_eEncoding);
                        if (FAILED(_hr))
                            return _hr;

                        return szANSI.SetString(std::move(_szSrc));
                    }
                }
                }
            }

            HRESULT __MEGA_UI_API SetString(const aStringView& _szSrc)
            {
                if (!IsANSI())
                    ClearAndUpdateEncoding(Encoding::ANSI);

                auto _hr = szANSI.SetString(_szSrc);
                if (FAILED(_hr))
                    return _hr;

                return szANSI.SetANSIEncoding(_szSrc.GetEncoding());
            }
            HRESULT __MEGA_UI_API SetString(const aString& _szSrc)
            {
                if (!IsANSI())
                    ClearAndUpdateEncoding(Encoding::ANSI);

                return szANSI.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(aString&& _szSrc)
            {
                return szANSI.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(const u8StringView& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF8);

                return szUTF8.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u8String& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF8);

                return szUTF8.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(u8String&& _szSrc)
            {
                return szUTF8.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(_In_reads_opt_(_cchSrc) const char16_t* _szSrc, _In_ uint_t _cchSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16LE);

                return szUTF16.SetString((u16char_t*)_szSrc, _cchSrc);
            }

            HRESULT __MEGA_UI_API SetString(_In_opt_z_ const char16_t* _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16LE);

                return szUTF16.SetString((u16char_t*)_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u16StringLEView& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16LE);

                return szUTF16LE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u16StringBEView& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16BE);

                return szUTF16BE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u16StringLE& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16LE);

                return szUTF16LE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(u16StringLE&& _szSrc)
            {
                return szUTF16LE.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(const u16StringBE& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF16BE);

                return szUTF16BE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(u16StringBE&& _szSrc)
            {
                return szUTF16BE.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(_In_reads_opt_(_cchSrc) const char32_t* _szSrc, _In_ uint_t _cchSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32LE);

                return szUTF32.SetString(_szSrc, _cchSrc);
            }

            HRESULT __MEGA_UI_API SetString(_In_opt_z_ const char32_t* _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32LE);

                return szUTF32.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u32StringLEView& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32LE);

                return szUTF32LE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u32StringBEView& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32BE);

                return szUTF32BE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(const u32StringLE& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32LE);

                return szUTF32LE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(u32StringLE&& _szSrc)
            {
                return szUTF32LE.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(const u32StringBE& _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTF32BE);

                return szUTF32BE.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API SetString(u32StringBE&& _szSrc)
            {
                return szUTF32BE.SetString(std::move(_szSrc));
            }

            HRESULT __MEGA_UI_API SetString(_In_reads_opt_(_cchSrc) const wchar_t* _szSrc, _In_ uint_t _cchSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTFW);

                return szUTFW.SetString(_szSrc, _cchSrc);
            }

            HRESULT __MEGA_UI_API SetString(_In_opt_z_ const wchar_t* _szSrc)
            {
                ClearAndUpdateEncoding(Encoding::UTFW);

                return szUTFW.SetString(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const aStringView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const u8StringView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const u16StringLEView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const u16StringBEView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const u32StringLEView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API AppendString(const u32StringBEView& _szSrc)
            {
                return AppendStringT(_szSrc);
            }

            HRESULT __MEGA_UI_API GetString(_Out_ aString* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                #pragma warning(suppress : 6001)
                const auto _eDstEncoding = _pszDst->GetEncoding();
                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    _pszDst->SetANSIEncoding(_eDstEncoding);
                    return Transform(szUTF8, _pszDst);
                case Encoding::UTF16LE:
                    _pszDst->SetANSIEncoding(_eDstEncoding);
                    return Transform(szUTF16LE, _pszDst);
                case Encoding::UTF16BE:
                    _pszDst->SetANSIEncoding(_eDstEncoding);
                    return Transform(szUTF16BE, _pszDst);
                case Encoding::UTF32LE:
                    _pszDst->SetANSIEncoding(_eDstEncoding);
                    return Transform(szUTF32LE, _pszDst);
                case Encoding::UTF32BE:
                    _pszDst->SetANSIEncoding(_eDstEncoding);
                    return Transform(szUTF32BE, _pszDst);
                default:
                    return _pszDst->SetString(szANSI);
                }
            }

            HRESULT __MEGA_UI_API GetString(_Out_ u8String* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    return _pszDst->SetString(szUTF8);
                case Encoding::UTF16LE:
                    return Transform(szUTF16LE, _pszDst);
                case Encoding::UTF16BE:
                    return Transform(szUTF16BE, _pszDst);
                case Encoding::UTF32LE:
                    return Transform(szUTF32LE, _pszDst);
                case Encoding::UTF32BE:
                    return Transform(szUTF32BE, _pszDst);
                default:
                    return Transform(szANSI, _pszDst);
                }
            }

            HRESULT __MEGA_UI_API GetString(_Out_ u16StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    return Transform(szUTF8, _pszDst);
                case Encoding::UTF16LE:
                    return _pszDst->SetString(szUTF16LE);
                case Encoding::UTF16BE:
                    return Transform(szUTF16BE, _pszDst);
                case Encoding::UTF32LE:
                    return Transform(szUTF32LE, _pszDst);
                case Encoding::UTF32BE:
                    return Transform(szUTF32BE, _pszDst);
                default:
                    return Transform(szANSI, _pszDst);
                }
            }

            HRESULT __MEGA_UI_API GetString(_Out_ u16StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    return Transform(szUTF8, _pszDst);
                case Encoding::UTF16LE:
                    return Transform(szUTF16LE, _pszDst);
                case Encoding::UTF16BE:
                    return _pszDst->SetString(szUTF16BE);
                case Encoding::UTF32LE:
                    return Transform(szUTF32LE, _pszDst);
                case Encoding::UTF32BE:
                    return Transform(szUTF32BE, _pszDst);
                default:
                    return Transform(szANSI, _pszDst);
                }
            }

            HRESULT __MEGA_UI_API GetString(_Out_ u32StringLE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    return Transform(szUTF8, _pszDst);
                case Encoding::UTF16LE:
                    return Transform(szUTF16LE, _pszDst);
                case Encoding::UTF16BE:
                    return Transform(szUTF16BE, _pszDst);
                case Encoding::UTF32LE:
                    return _pszDst->SetString(szUTF32LE);
                case Encoding::UTF32BE:
                    return Transform(szUTF32BE, _pszDst);
                default:
                    return Transform(szANSI, _pszDst);
                }
            }

            HRESULT __MEGA_UI_API GetString(_Out_ u32StringBE* _pszDst)
            {
                if (!_pszDst)
                    return E_POINTER;

                _pszDst->Clear();

                switch (GetEncoding())
                {
                case Encoding::UTF8:
                    return Transform(szUTF8, _pszDst);
                case Encoding::UTF16LE:
                    return Transform(szUTF16LE, _pszDst);
                case Encoding::UTF16BE:
                    return Transform(szUTF16BE, _pszDst);
                case Encoding::UTF32LE:
                    return Transform(szUTF32LE, _pszDst);
                case Encoding::UTF32BE:
                    return _pszDst->SetString(szUTF32BE);
                default:
                    return Transform(szANSI, _pszDst);
                }
            }

            uint_t __MEGA_UI_API GetSize()
            {
                return GetInternalStringData()->uSize;
            }

            void __MEGA_UI_API Clear()
            {
                auto _pStringData = GetInternalStringData();
                if (_pStringData->uSize == 0)
                    return;

                const auto OldEncoding = Encoding(_pStringData->eEncoding);

                switch (OldEncoding)
                {
                case Encoding::UTF8:
                    szUTF8.Clear();
                    break;
                case Encoding::UTF16LE:
                    szUTF16LE.Clear();
                    break;
                case Encoding::UTF16BE:
                    szUTF16BE.Clear();
                    break;
                case Encoding::UTF32LE:
                    szUTF32LE.Clear();
                    break;
                case Encoding::UTF32BE:
                    szUTF32BE.Clear();
                    break;
                case Encoding::ANSI:
                    szANSI.Clear();
                    break;
                default:
                    szANSI.Clear();
                    szANSI.SetANSIEncoding(OldEncoding);
                    break;
                }
            }

        private:
            aString::StringData* __MEGA_UI_API GetInternalStringData()
            {
                // 因为所有编码的字符串长度 以及 缓冲区大小格式都是一样的，
                // 所以随便取一个 GetInternalStringData 就可以了。
                return szANSI.GetInternalStringData();
            }

            static constexpr uint32_t __MEGA_UI_API GetCharSize(Encoding eEncoding)
            {
                // 因为独占缓冲区，所以直接切换 Encoding 类型
                switch (eEncoding)
                {
                case Encoding::UTF16LE:
                case Encoding::UTF16BE:
                    return sizeof(u16char_t);
                case Encoding::UTF32LE:
                case Encoding::UTF32BE:
                    return sizeof(u32char_t);
                default:
                    return sizeof(achar_t);
                }
            }

            bool __MEGA_UI_API IsANSI()
            {
                return YY::MegaUI::IsANSI(GetEncoding());
            }

            HRESULT __MEGA_UI_API ClearAndUpdateEncoding(Encoding _eEncoding)
            {
                auto _pStringData = GetInternalStringData();
                auto _eOldEncoding = Encoding(_pStringData->eEncoding);
                if (_eOldEncoding == _eEncoding)
                    return S_OK;

                if (_pStringData->IsShared())
                {
                    switch (_eEncoding)
                    {
                    case YY::MegaUI::Encoding::UTF16LE:
                        szUTF16LE.Detach()->Release();
                        return S_OK;
                    case YY::MegaUI::Encoding::UTF16BE:
                        szUTF16BE.Detach()->Release();
                        return S_OK;
                    case YY::MegaUI::Encoding::UTF32LE:
                        szUTF32LE.Detach()->Release();
                        return S_OK;
                    case YY::MegaUI::Encoding::UTF32BE:
                        szUTF16BE.Detach()->Release();
                        return S_OK;
                    case YY::MegaUI::Encoding::UTF8:
                        szUTF8.Detach()->Release();
                        return S_OK;
                    default:
                        szANSI.Detach()->Release();
                        return szANSI.SetANSIEncoding(_eEncoding);
                    }
                }
                else
                {
                    const auto _uOldCharSize = GetCharSize(_eOldEncoding);
                    const auto _uNewCharSize = GetCharSize(_eEncoding);

                    if (_uOldCharSize != _uNewCharSize)
                    {
                        _pStringData->uCapacity = (_pStringData->uCapacity + 1) * _uOldCharSize / _uNewCharSize - 1;
                    }

                    _pStringData->eEncoding = uint16_t(_eEncoding);
                    _pStringData->uSize = 0;
                    memset(_pStringData->GetStringBuffer(), 0, _uNewCharSize);
                    return S_OK;
                }
            }

            template<class string_t>
            __forceinline HRESULT __MEGA_UI_API AppendStringT(const string_t& _szSrc)
            {
                if (_szSrc.GetSize() == 0)
                    return S_OK;

                const auto _eDstEncoding = GetEncoding();

                switch (_eDstEncoding)
                {
                case Encoding::UTF8:
                    return Transform(_szSrc, &szUTF8);
                case Encoding::UTF16LE:
                    return Transform(_szSrc, &szUTF16LE);
                case Encoding::UTF16BE:
                    return Transform(_szSrc, &szUTF16BE);
                case Encoding::UTF32LE:
                    return Transform(_szSrc, &szUTF32LE);
                case Encoding::UTF32BE:
                    return Transform(_szSrc, &szUTF32BE);
                default:
                    // ANSI
                    return Transform(_szSrc, &szANSI);
                }
            }
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
