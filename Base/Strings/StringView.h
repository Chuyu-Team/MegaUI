﻿#pragma once
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include <Base/YY.h>
#include <Base/Encoding.h>
#include <Base/tchar.h>
#include <MegaUI/Base/ErrorCode.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Strings
        {
            template<typename _char_t, Encoding _eEncoding>
            class StringView
            {
            public:
                using char_t = _char_t;

            private:
                const char_t* szString;
                size_t cchString;
                constexpr static Encoding eEncoding = _eEncoding;

            public:
                explicit StringView()
                    : szString(nullptr)
                    , cchString(0)
                {
                }

                explicit StringView(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
                    : szString(_szSrc)
                    , cchString(_szSrc ? _cchSrc : 0)
                {
                }

                StringView(_In_opt_z_ const char_t* _szSrc)
                    : szString(_szSrc)
                    , cchString(GetStringLength(_szSrc))
                {
                }

                template<size_t _uArrayCount>
                StringView(const char_t (&_szSrc)[_uArrayCount])
                    : szString(_szSrc)
                    , cchString(_uArrayCount - 1)
                {
                }

                HRESULT __YYAPI SetString(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
                {
                    if (_szSrc == nullptr && _cchSrc)
                        return E_INVALIDARG;

                    szString = _szSrc;
                    cchString = _cchSrc;
                    return S_OK;
                }

                constexpr static Encoding __YYAPI GetEncoding()
                {
                    return eEncoding;
                }

                inline size_t __YYAPI GetSize() const
                {
                    return cchString;
                }

                inline _Ret_notnull_ _Post_readable_size_(cchString)
                    const char_t* __YYAPI GetConstString() const
                {
                    return szString;
                }

                inline char_t __YYAPI operator[](_In_ size_t _uIndex) const
                {
                    assert(_uIndex < GetSize());

                    return szString[_uIndex];
                }

                inline const char_t* __YYAPI begin() const
                {
                    return this->GetConstString();
                }

                inline const char_t* __YYAPI end() const
                {
                    return this->GetConstString() + this->GetSize();
                }

                bool __YYAPI operator==(StringView _Other) const
                {
                    if (cchString != _Other.cchString)
                        return false;

                    return memcmp(szString, _Other.szString, cchString * sizeof(szString[0])) == 0;
                }

                bool __YYAPI operator==(_In_z_ const char_t* _Other) const
                {
                    return Compare(_Other) == 0;
                }

                int __YYAPI Compare(_In_z_ const char_t* _Other) const
                {
                    if (_Other == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    size_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = szString[_uIndex] - _Other[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _Other[_uIndex];
                }

                int __YYAPI Compare(_In_ StringView _szOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _szOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = szString[_uIndex] - _szOther[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _szOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _szOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                int32_t __YYAPI CompareI(_In_z_ const char_t* _Other) const
                {
                    if (_Other == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    uint_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(szString[_uIndex]) - CharUpperAsASCII(_Other[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _Other[_uIndex];
                }

                int32_t __YYAPI CompareI(_In_ StringView _szOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _szOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(szString[_uIndex]) - CharUpperAsASCII(_szOther[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _szOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _szOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }
            };

            template<>
            class StringView<YY::Base::achar_t, Encoding::ANSI>
            {
            public:
                using char_t = YY::Base::achar_t;

            private:
                _Field_size_(cchString) const char_t* szString;
                size_t cchString;
                Encoding eEncoding;

            public:
                explicit StringView()
                    : szString(nullptr)
                    , cchString(0)
                    , eEncoding(Encoding::ANSI)
                {
                }

                explicit StringView(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc, _In_ Encoding _eEncoding = Encoding::ANSI)
                    : szString(_szSrc)
                    , cchString(_cchSrc)
                    , eEncoding(_eEncoding)
                {
                }

                StringView(_In_opt_z_ const char_t* _szSrc, _In_ Encoding _eEncoding = Encoding::ANSI)
                    : szString(_szSrc)
                    , cchString(YY::Base::GetStringLength(_szSrc))
                    , eEncoding(_eEncoding)
                {
                }

                template<size_t _uArrayCount>
                StringView(const char_t (&_szSrc)[_uArrayCount], _In_ Encoding _eEncoding = Encoding::ANSI)
                    : szString(_szSrc)
                    , cchString(_uArrayCount - 1)
                    , eEncoding(_eEncoding)
                {
                }

                inline Encoding __YYAPI GetEncoding() const
                {
                    return eEncoding;
                }

                inline size_t __YYAPI GetSize() const
                {
                    return cchString;
                }

                inline _Ret_notnull_ _Post_readable_size_(cchString)
                    const char_t* __YYAPI GetConstString() const
                {
                    return szString;
                }

                inline char_t __YYAPI operator[](_In_ size_t _uIndex) const
                {
                    assert(_uIndex < GetSize());

                    return szString[_uIndex];
                }

                inline const char_t* __YYAPI begin() const
                {
                    return this->GetConstString();
                }

                inline const char_t* __YYAPI end() const
                {
                    return this->GetConstString() + this->GetSize();
                }
            };

            typedef StringView<YY::Base::achar_t, Encoding::ANSI> aStringView;
            typedef StringView<YY::Base::u8char_t, Encoding::UTF8> u8StringView;
            typedef StringView<YY::Base::u16char_t, Encoding::UTF16LE> u16StringLEView;
            typedef StringView<YY::Base::u16char_t, Encoding::UTF16BE> u16StringBEView;
            typedef StringView<YY::Base::u32char_t, Encoding::UTF32LE> u32StringLEView;
            typedef StringView<YY::Base::u32char_t, Encoding::UTF32BE> u32StringBEView;
            typedef StringView<YY::Base::uchar_t, Encoding::UTFN> uStringView;

            typedef StringView<YY::Base::u16char_t, Encoding::UTF16> u16StringView;
            typedef StringView<YY::Base::u32char_t, Encoding::UTF32> u32StringView;

        } // namespace Strings
    } // namespace Base

    using namespace YY::Base::Strings;
} // namespace YY

#pragma pack(pop)
