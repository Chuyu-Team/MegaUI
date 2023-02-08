#pragma once
#include <wchar.h>

#include <Base/YY.h>

// 为C++提供不同字符类型的重载

namespace YY
{
    namespace Base
    {
        template<typename _CharT>
        struct CharConfing
        {
        };

        template<>
        struct CharConfing<achar_t>
        {
            typedef achar_t _CharT;
        };

        template<>
        struct CharConfing<u8char_t>
        {
            typedef u8char_t _CharT;
        };

        template<>
        struct CharConfing<u16char_t>
        {
            typedef u16char_t _CharT;
        };

        template<>
        struct CharConfing<u32char_t>
        {
            typedef u32char_t _CharT;
        };

        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const _CharT* _szSrc)
        {
            if (!_szSrc)
                return 0;
            auto _pScan = _szSrc;
            for (; *_pScan; ++_pScan)
                ;

            return _pScan - _szSrc;
        }

        template<>
        __forceinline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const achar_t* _szSrc)
        {
            return _szSrc ? strlen(_szSrc) : 0;
        }

        template<>
        __forceinline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const u8char_t* _szSrc)
        {
            return _szSrc ? strlen((const achar_t*)_szSrc) : 0;
        }

        template<>
        __forceinline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const wchar_t* _szSrc)
        {
            return _szSrc ? wcslen(_szSrc) : 0;
        }

        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline constexpr bool __YYAPI IsEmptyString(_In_opt_z_ const _CharT* _szSrc)
        {
            return _szSrc == nullptr || _szSrc[0] == _CharT('\0');
        }

        /// <summary>
        /// 按 ASCII 规则，将输入的字符转换为大写。
        /// </summary>
        /// <param name="_ch"></param>
        /// <returns></returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline constexpr _CharT __YYAPI CharUpperAsASCII(_CharT _ch)
        {
            if (_ch >= _CharT('a') && _ch <= _CharT('z'))
            {
                _ch = _ch - _CharT('a') + _CharT('A');
            }
            return _ch;
        }

        __forceinline auto __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ achar_t const* const _Format,
            va_list _ArgList)
        {
            return _vscprintf(_Format, _ArgList);
        }

        __forceinline auto __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ u8char_t const* const _Format,
            va_list _ArgList)
        {
            return _vscprintf((const achar_t*)_Format, _ArgList);
        }

        __forceinline auto __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            va_list _ArgList)
        {
            return _vscwprintf(_Format, _ArgList);
        }

        _Success_(return >= 0) __forceinline auto __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) achar_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ achar_t const* const _Format,
            va_list _ArgList)
        {
            return vsprintf_s(_Buffer, _BufferCount, _Format, _ArgList);
        }

        _Success_(return >= 0) __forceinline auto __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) u8char_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ u8char_t const* const _Format,
            va_list _ArgList)
        {
            return vsprintf_s((achar_t*)_Buffer, _BufferCount, (achar_t*)_Format, _ArgList);
        }

        _Success_(return >= 0) __forceinline auto __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            va_list _ArgList)
        {
            return vswprintf_s(_Buffer, _BufferCount, _Format, _ArgList);
        }

        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline int __YYAPI StringCompare(_In_opt_z_ const _CharT* _szLeft, _In_opt_z_ const _CharT* _szRight)
        {
            if (_szLeft == _szRight)
                return 0;

            if (!_szLeft)
            {
                return *_szRight ? -1 : 0;
            }

            if (!_szRight)
            {
                return *_szLeft ? 1 : 0;
            }

            for (;; ++_szLeft, ++_szRight)
            {
                auto _iResult = *_szLeft - *_szRight;
                if (_iResult != 0)
                    return _iResult;

                if (*_szLeft == _CharT('\0'))
                    break;
            }

            return 0;
        }

        template<>
        __forceinline int __YYAPI StringCompare(_In_opt_z_ const achar_t* _szLeft, _In_opt_z_ const achar_t* _szRight)
        {
            if (_szLeft == _szRight)
                return 0;

            if (!_szLeft)
            {
                return *_szRight ? -1 : 0;
            }

            if (!_szRight)
            {
                return *_szLeft ? 1 : 0;
            }

            return strcmp(_szLeft, _szRight);
        }

        template<>
        __forceinline int __YYAPI StringCompare(_In_opt_z_ const u8char_t* _szLeft, _In_opt_z_ const u8char_t* _szRight)
        {
            if (_szLeft == _szRight)
                return 0;

            if (!_szLeft)
            {
                return *_szRight ? -1 : 0;
            }

            if (!_szRight)
            {
                return *_szLeft ? 1 : 0;
            }

            return strcmp((achar_t*)_szLeft, (achar_t*)_szRight);
        }

        template<>
        __forceinline int __YYAPI StringCompare(_In_opt_z_ const wchar_t* _szLeft, _In_opt_z_ const wchar_t* _szRight)
        {
            if (_szLeft == _szRight)
                return 0;

            if (!_szLeft)
            {
                return *_szRight ? -1 : 0;
            }

            if (!_szRight)
            {
                return *_szLeft ? 1 : 0;
            }

            return wcscmp(_szLeft, _szRight);
        }

        /// <summary>
        /// 带长度的字符串比较。
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>如果 return == 0，那么_szLeft ==_szRight
        /// 如果 return 大于 0，那么 _szLeft 大于 _szRight
        /// 如果 return 小于 0，那么 _szLeft 小于_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline int __YYAPI StringCompare(
            _In_reads_(_uCount) const _CharT* _szLeft,
            _In_reads_(_uCount) const _CharT* _szRight,
            _In_ size_t _uCount)
        {
            if (_uCount == 0)
                return 0;

            if (_szLeft == _szRight)
                return 0;

            for (; _uCount && *_szLeft == *_szRight; --_uCount, ++_szLeft, ++_szRight)
                ;

            return *_szLeft - *_szRight;
        }

        template<>
        __forceinline int __YYAPI StringCompare(
            _In_reads_(_uCount) const achar_t* _szLeft,
            _In_reads_(_uCount) const achar_t* _szRight,
            _In_ size_t _uCount)
        {
            if (_uCount == 0)
                return 0;

            if (_szLeft == _szRight)
                return 0;

            return memcmp(_szLeft, _szRight, _uCount);
        }

        template<>
        __forceinline int __YYAPI StringCompare(
            _In_reads_(_uCount) const u8char_t* _szLeft,
            _In_reads_(_uCount) const u8char_t* _szRight,
            _In_ size_t _uCount)
        {
            if (_uCount == 0)
                return 0;

            if (_szLeft == _szRight)
                return 0;

            return memcmp(_szLeft, _szRight, _uCount);
        }

        template<>
        __forceinline int __YYAPI StringCompare(
            _In_reads_(_uCount) const wchar_t* _szLeft,
            _In_reads_(_uCount) const wchar_t* _szRight,
            _In_ size_t _uCount)
        {
            if (_uCount == 0)
                return 0;

            if (_szLeft == _szRight)
                return 0;

            return wmemcmp(_szLeft, _szRight, _uCount);
        }

        /// <summary>
        /// 按ASCII规则忽略大小写比较。
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>如果 return == 0，那么_szLeft ==_szRight
        /// 如果 return 大于 0，那么 _szLeft 大于 _szRight
        /// 如果 return 小于 0，那么 _szLeft 小于_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline int __YYAPI StringCompareIgnoreAsASCII(_In_opt_z_ const _CharT* _szLeft, _In_opt_z_ const _CharT* _szRight)
        {
            if (_szLeft == _szRight)
                return 0;

            if (!_szLeft)
                return *_szRight ? -1 : 0;

            if (!_szRight)
                return *_szLeft ? 1 : 0;

            for (;; ++_szLeft, ++_szRight)
            {
                auto _iResult = CharUpperAsASCII(*_szLeft) - CharUpperAsASCII(*_szRight);
                if (_iResult != 0)
                    return _iResult;

                if (*_szLeft == _CharT('\0'))
                    break;
            }

            return 0;
        }

        /// <summary>
        /// 按ASCII规则忽略大小写比较。
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>如果 return == 0，那么_szLeft ==_szRight
        /// 如果 return 大于 0，那么 _szLeft 大于 _szRight
        /// 如果 return 小于 0，那么 _szLeft 小于_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        __forceinline int __YYAPI StringCompareIgnoreAsASCII(
            _In_reads_(_uCount) const _CharT* _szLeft,
            _In_reads_(_uCount) const _CharT* _szRight,
            _In_ size_t _uCount)
        {
            if (_uCount == 0)
                return 0;

            if (_szLeft == _szRight)
                return 0;

            for (; _uCount; --_uCount, ++_szLeft, ++_szRight)
            {
                auto _iResult = CharUpperAsASCII(*_szLeft) - CharUpperAsASCII(*_szRight);
                if (_iResult != 0)
                    return _iResult;
            }

            return 0;
        }
    } // namespace Base
} // namespace YY