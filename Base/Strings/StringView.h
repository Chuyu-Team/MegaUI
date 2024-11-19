#pragma once
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include <Base/YY.h>
#include <Base/Encoding.h>
#include <Base/tchar.h>
#include <Base/ErrorCode.h>

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
                // 不一定指向 0结尾！！
                _Field_size_(cchString) const char_t* sString;
                size_t cchString;
                constexpr static Encoding eEncoding = _eEncoding;

            public:
                explicit StringView()
                    : sString(nullptr)
                    , cchString(0)
                {
                }

                explicit StringView(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
                    : sString(_szSrc)
                    , cchString(_szSrc ? _cchSrc : 0)
                {
                }

                StringView(_In_opt_z_ const char_t* _szSrc)
                    : sString(_szSrc)
                    , cchString(GetStringLength(_szSrc))
                {
                }

                template<size_t _uArrayCount>
                StringView(const char_t (&_szSrc)[_uArrayCount])
                    : sString(_szSrc)
                    , cchString(_uArrayCount - 1)
                {
                }

                HRESULT __YYAPI SetString(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
                {
                    if (_szSrc == nullptr && _cchSrc)
                        return E_INVALIDARG;

                    sString = _szSrc;
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
                    return sString;
                }

                inline char_t __YYAPI operator[](_In_ size_t _uIndex) const
                {
                    assert(_uIndex < GetSize());

                    return sString[_uIndex];
                }

                inline const char_t* __YYAPI begin() const
                {
                    return this->GetConstString();
                }

                inline const char_t* __YYAPI end() const
                {
                    return this->GetConstString() + this->GetSize();
                }

                bool __YYAPI operator==(StringView _sOther) const
                {
                    if (cchString != _sOther.cchString)
                        return false;

                    return memcmp(sString, _sOther.sString, cchString * sizeof(sString[0])) == 0;
                }

                bool __YYAPI operator==(_In_z_ const char_t* _Other) const
                {
                    return Compare(_Other) == 0;
                }

                int __YYAPI Compare(_In_z_ const char_t* _szOther) const
                {
                    if (_szOther == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    size_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = sString[_uIndex] - _szOther[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _szOther[_uIndex];
                }

                int __YYAPI Compare(_In_ StringView _sOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _sOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = sString[_uIndex] - _sOther[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _sOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _sOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                int32_t __YYAPI CompareI(_In_z_ const char_t* _szOther) const
                {
                    if (_szOther == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    uint_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(sString[_uIndex]) - CharUpperAsASCII(_szOther[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _szOther[_uIndex];
                }

                int32_t __YYAPI CompareI(_In_ StringView _sOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _sOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(sString[_uIndex]) - CharUpperAsASCII(_sOther[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _sOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _sOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                size_t __YYAPI Find(char_t _ch, size_t _uIndex = 0) noexcept
                {
                    for (; _uIndex < GetSize(); ++_uIndex)
                    {
                        if (sString[_uIndex] == _ch)
                        {
                            return _uIndex;
                        }
                    }

                    return -1;
                }

                StringView& __YYAPI Slice(size_t _uRemoveStart, size_t _uRemoveEnd = 0u) noexcept
                {
                    if (_uRemoveStart + _uRemoveEnd >= cchString)
                    {
                        cchString = 0;
                    }
                    else
                    {
                        sString += _uRemoveStart;
                        cchString -= _uRemoveStart;
                        cchString -= _uRemoveEnd;
                    }

                    return *this;
                }

                StringView __YYAPI Substring(size_t _uStartIndex) const noexcept
                {
                    if (_uStartIndex < cchString)
                    {
                        return StringView(sString + _uStartIndex, cchString - _uStartIndex);
                    }
                    else
                    {
                        return StringView();
                    }
                }

                StringView __YYAPI Substring(size_t _uStartIndex, size_t _cchLength) const noexcept
                {
                    if (_uStartIndex < cchString)
                    {
                        return StringView(sString + _uStartIndex, min(cchString - _uStartIndex, _cchLength));
                    }
                    else
                    {
                        return StringView();
                    }
                }
            };

            template<>
            class StringView<YY::Base::achar_t, Encoding::ANSI>
            {
            public:
                using char_t = YY::Base::achar_t;

            private:
                // 不一定指向 0结尾！！
                _Field_size_(cchString) const char_t* sString;
                size_t cchString;
                Encoding eEncoding;

            public:
                explicit StringView()
                    : sString(nullptr)
                    , cchString(0)
                    , eEncoding(Encoding::ANSI)
                {
                }

                explicit StringView(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc, _In_ Encoding _eEncoding = Encoding::ANSI)
                    : sString(_szSrc)
                    , cchString(_cchSrc)
                    , eEncoding(_eEncoding)
                {
                }

                StringView(_In_opt_z_ const char_t* _szSrc, _In_ Encoding _eEncoding = Encoding::ANSI)
                    : sString(_szSrc)
                    , cchString(YY::Base::GetStringLength(_szSrc))
                    , eEncoding(_eEncoding)
                {
                }

                template<size_t _uArrayCount>
                StringView(const char_t (&_szSrc)[_uArrayCount], _In_ Encoding _eEncoding = Encoding::ANSI)
                    : sString(_szSrc)
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
                    return sString;
                }

                inline char_t __YYAPI operator[](_In_ size_t _uIndex) const
                {
                    assert(_uIndex < GetSize());

                    return sString[_uIndex];
                }

                inline const char_t* __YYAPI begin() const
                {
                    return this->GetConstString();
                }

                inline const char_t* __YYAPI end() const
                {
                    return this->GetConstString() + this->GetSize();
                }

                bool __YYAPI operator==(StringView _sOther) const
                {
                    if (cchString != _sOther.cchString)
                        return false;

                    return memcmp(sString, _sOther.sString, cchString * sizeof(sString[0])) == 0;
                }

                bool __YYAPI operator==(_In_z_ const char_t* _szOther) const
                {
                    return Compare(_szOther) == 0;
                }

                int __YYAPI Compare(_In_z_ const char_t* _szOther) const
                {
                    if (_szOther == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    size_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = sString[_uIndex] - _szOther[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _szOther[_uIndex];
                }

                int __YYAPI Compare(_In_ StringView _sOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _sOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = sString[_uIndex] - _sOther[_uIndex];
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _sOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _sOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                int32_t __YYAPI CompareI(_In_z_ const char_t* _szOther) const
                {
                    if (_szOther == nullptr)
                    {
                        return cchString ? 1 : 0;
                    }

                    uint_t _uIndex = 0;
                    for (; _uIndex != GetSize(); ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(sString[_uIndex]) - CharUpperAsASCII(_szOther[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    return char_t('\0') - _szOther[_uIndex];
                }

                int32_t __YYAPI CompareI(_In_ StringView _sOther) const
                {
                    const auto _uMinSize = (std::min)(GetSize(), _sOther.GetSize());

                    uint_t _uIndex = 0;
                    for (; _uIndex != _uMinSize; ++_uIndex)
                    {
                        auto _result = CharUpperAsASCII(sString[_uIndex]) - CharUpperAsASCII(_sOther[_uIndex]);
                        if (_result != 0)
                            return _result;
                    }

                    if (GetSize() > _sOther.GetSize())
                    {
                        return 1;
                    }
                    else if (GetSize() < _sOther.GetSize())
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }

                size_t __YYAPI Find(char_t _ch, size_t _uIndex = 0) noexcept
                {
                    for (; _uIndex < GetSize(); ++_uIndex)
                    {
                        if (sString[_uIndex] == _ch)
                        {
                            return _uIndex;
                        }
                    }

                    return -1;
                }

                StringView& __YYAPI Slice(size_t _uRemoveStart, size_t _uRemoveEnd = 0u) noexcept
                {
                    if (_uRemoveStart + _uRemoveEnd >= cchString)
                    {
                        cchString = 0;
                    }
                    else
                    {
                        sString += _uRemoveStart;
                        cchString -= _uRemoveStart;
                        cchString -= _uRemoveEnd;
                    }

                    return *this;
                }

                StringView __YYAPI Substring(size_t _uStartIndex) const noexcept
                {
                    if (_uStartIndex < cchString)
                    {
                        return StringView(sString + _uStartIndex, cchString - _uStartIndex);
                    }
                    else
                    {
                        return StringView();
                    }
                }

                StringView __YYAPI Substring(size_t _uStartIndex, size_t _cchLength) const noexcept
                {
                    if (_uStartIndex < cchString)
                    {
                        return StringView(sString + _uStartIndex, min(cchString - _uStartIndex, _cchLength));
                    }
                    else
                    {
                        return StringView();
                    }
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
