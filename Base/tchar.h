#pragma once
#include <wchar.h>

#include <Base/YY.h>

// ΪC++�ṩ��ͬ�ַ����͵�����

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
        inline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const _CharT* _szSrc)
        {
            if (!_szSrc)
                return 0;
            auto _pScan = _szSrc;
            for (; *_pScan; ++_pScan)
                ;

            return _pScan - _szSrc;
        }

        template<>
        inline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const achar_t* _szSrc)
        {
            return _szSrc ? strlen((const char*)_szSrc) : 0;
        }

        template<>
        inline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const u8char_t* _szSrc)
        {
            return _szSrc ? strlen((const char*)_szSrc) : 0;
        }

        template<>
        inline constexpr size_t __YYAPI GetStringLength(_In_opt_z_ const wchar_t* _szSrc)
        {
            return _szSrc ? wcslen(_szSrc) : 0;
        }

        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline constexpr bool __YYAPI IsEmptyString(_In_opt_z_ const _CharT* _szSrc)
        {
            return _szSrc == nullptr || _szSrc[0] == _CharT('\0');
        }

        /// <summary>
        /// �� ASCII ���򣬽�������ַ�ת��Ϊ��д��
        /// </summary>
        /// <param name="_ch"></param>
        /// <returns></returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline constexpr _CharT __YYAPI CharUpperAsASCII(_CharT _ch)
        {
            if (_ch >= _CharT('a') && _ch <= _CharT('z'))
            {
                _ch = _ch - _CharT('a') + _CharT('A');
            }
            return _ch;
        }

        inline int __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ achar_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return _vscprintf((const char*)_Format, _ArgList);
#else
            return vsnprintf(nullptr, 0, (const char*)_Format, _ArgList);
#endif
        }

        inline int __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ u8char_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return _vscprintf((const char*)_Format, _ArgList);
#else
            return vsnprintf(nullptr, 0, (const char*)_Format, _ArgList);
#endif

        }

        inline int __YYAPI GetStringFormatLength(
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return _vscwprintf(_Format, _ArgList);
#else
            return vswprintf(nullptr, 0, _Format, _ArgList);
#endif
        }

        _Success_(return >= 0) inline int __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) achar_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ achar_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return vsprintf_s((char*)_Buffer, _BufferCount, (const char*)_Format, _ArgList);
#else
            return vsnprintf((char*)_Buffer, _BufferCount, (const char*)_Format, _ArgList);
#endif
        }

        _Success_(return >= 0) inline int __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) u8char_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ u8char_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return vsprintf_s((char*)_Buffer, _BufferCount, (const char*)_Format, _ArgList);
#else
            return vsnprintf((char*)_Buffer, _BufferCount, (const char*)_Format, _ArgList);
#endif
        }

        _Success_(return >= 0) inline int __YYAPI FormatStringV(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t* const _Buffer,
            _In_ size_t const _BufferCount,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            va_list _ArgList)
        {
#ifdef _MSC_VER
            return vswprintf_s(_Buffer, _BufferCount, _Format, _ArgList);
#else
            return vswprintf(_Buffer, _BufferCount, _Format, _ArgList);
#endif
        }

        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline int __YYAPI StringCompare(_In_opt_z_ const _CharT* _szLeft, _In_opt_z_ const _CharT* _szRight)
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
        inline int __YYAPI StringCompare(_In_opt_z_ const achar_t* _szLeft, _In_opt_z_ const achar_t* _szRight)
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

            return strcmp((const char*)_szLeft, (const char*)_szRight);
        }

        template<>
        inline int __YYAPI StringCompare(_In_opt_z_ const u8char_t* _szLeft, _In_opt_z_ const u8char_t* _szRight)
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

            return strcmp((const char*)_szLeft, (const char*)_szRight);
        }

        template<>
        inline int __YYAPI StringCompare(_In_opt_z_ const wchar_t* _szLeft, _In_opt_z_ const wchar_t* _szRight)
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
        /// �����ȵ��ַ����Ƚϡ�
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>��� return == 0����ô_szLeft ==_szRight
        /// ��� return ���� 0����ô _szLeft ���� _szRight
        /// ��� return С�� 0����ô _szLeft С��_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline int __YYAPI StringCompare(
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
        inline int __YYAPI StringCompare(
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
        inline int __YYAPI StringCompare(
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
        inline int __YYAPI StringCompare(
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
        /// ��ASCII������Դ�Сд�Ƚϡ�
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>��� return == 0����ô_szLeft ==_szRight
        /// ��� return ���� 0����ô _szLeft ���� _szRight
        /// ��� return С�� 0����ô _szLeft С��_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline int __YYAPI StringCompareIgnoreAsASCII(_In_opt_z_ const _CharT* _szLeft, _In_opt_z_ const _CharT* _szRight)
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
        /// ��ASCII������Դ�Сд�Ƚϡ�
        /// </summary>
        /// <param name="_szLeft"></param>
        /// <param name="_szRight"></param>
        /// <param name="_uCount"></param>
        /// <returns>��� return == 0����ô_szLeft ==_szRight
        /// ��� return ���� 0����ô _szLeft ���� _szRight
        /// ��� return С�� 0����ô _szLeft С��_szRight</returns>
        template<typename _CharT, typename = typename CharConfing<_CharT>::_CharT>
        inline int __YYAPI StringCompareIgnoreAsASCII(
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