﻿#pragma once
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>

#include "MegaUITypeInt.h"
#include "Encoding.h"

namespace YY
{
	namespace MegaUI
	{
		__forceinline constexpr uint_t __fastcall GetStringLength(const achar_t* szString)
		{
			return szString ? strlen(szString) : 0;
		}

		__forceinline constexpr uint_t __fastcall GetStringLength(const u8char_t* szString)
		{
			return szString ? strlen((const char*)szString) : 0;
		}

		__forceinline constexpr uint_t __fastcall GetStringLength(const u16char_t* szString)
		{
			return szString ? wcslen(szString) : 0;
		}

		__forceinline constexpr uint_t __fastcall GetStringLength(const u32char_t* szString)
		{
			if (!szString)
				return 0;
			auto pScan = szString;
			for (; *pScan; ++pScan);

			return pScan - szString;
		}

		__forceinline constexpr bool __fastcall IsEmptyString(const achar_t* szString)
		{
			return szString == nullptr || szString[0] == achar_t('\0');
		}

		__forceinline constexpr bool __fastcall IsEmptyString(const u8char_t* szString)
		{
			return szString == nullptr || szString[0] == u8char_t('\0');
		}

		__forceinline constexpr bool __fastcall IsEmptyString(const u16char_t* szString)
		{
			return szString == nullptr || szString[0] == u16char_t('\0');
		}

		__forceinline constexpr bool __fastcall IsEmptyString(const u32char_t* szString)
		{
			return szString == nullptr || szString[0] == u32char_t('\0');
		}

        __forceinline auto __cdecl GetStringFormatLength(
            _In_z_ _Printf_format_string_ achar_t const* const _Format,
            va_list              _ArgList
			)
        {
            return _vscprintf(_Format, _ArgList);
        }

		__forceinline auto __cdecl GetStringFormatLength(
			_In_z_ _Printf_format_string_ u8char_t const* const _Format,
			va_list               _ArgList
			)
		{
			return _vscprintf((const achar_t*)_Format, _ArgList);
		}

		__forceinline auto __cdecl GetStringFormatLength(
			_In_z_ _Printf_format_string_ wchar_t const* const _Format,
			va_list              _ArgList
			)
		{
			return _vscwprintf(_Format, _ArgList);
		}

		_Success_(return >= 0)
		__forceinline auto __cdecl FormatStringV(
				_Out_writes_(_BufferCount) _Always_(_Post_z_) achar_t* const _Buffer,
				_In_                                          size_t         const _BufferCount,
				_In_z_ _Printf_format_string_                 achar_t const* const _Format,
				va_list              _ArgList
			)
		{
			return vsprintf_s(_Buffer, _BufferCount, _Format, _ArgList);
		}

		_Success_(return >= 0)
		__forceinline auto __cdecl FormatStringV(
				_Out_writes_(_BufferCount) _Always_(_Post_z_) u8char_t* const _Buffer,
				_In_                                          size_t          const _BufferCount,
				_In_z_ _Printf_format_string_                 u8char_t const* const _Format,
				va_list               _ArgList
			)
		{
			return vsprintf_s((achar_t*)_Buffer, _BufferCount, (achar_t*)_Format, _ArgList);
		}

		_Success_(return >= 0)
		__forceinline auto __cdecl FormatStringV(
				_Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t* const _Buffer,
				_In_                                          size_t         const _BufferCount,
				_In_z_ _Printf_format_string_                 wchar_t const* const _Format,
				va_list              _ArgList
			)
		{
			return vswprintf_s(_Buffer, _BufferCount, _Format, _ArgList);
		}


		template<typename _char_t, Encoding _eEncoding>
		class StringView
		{
		public:
			using char_t = _char_t;
		private:
			constexpr static Encoding eEncoding = _eEncoding;

			_Field_z_ const char_t* _szString;
			uint_t _cchString;
		public:
			StringView()
				: _szString(nullptr)
				, _cchString(0)
			{
			}

			StringView(_In_reads_opt_(cchSrc) const char_t* szSrc, _In_ uint_t cchSrc)
				: _szString(szSrc)
				, _cchString(cchSrc)
			{
			}

			StringView(_In_opt_z_ const char_t* szSrc)
				: _szString(szSrc)
				, _cchString(GetStringLength(szSrc))
			{
			}

			template<uint_t ArrayCount>
			StringView(const char_t (&szSrc)[ArrayCount])
				: _szString(szSrc)
				, _cchString(ArrayCount - 1)
			{
			}

			constexpr static Encoding __fastcall GetEncoding()
			{
				return eEncoding;
			}

			__forceinline uint_t __fastcall GetSize() const
			{
				return _cchString;
			}

			__forceinline _Ret_maybenull_z_ const char_t* __fastcall GetConstString() const
			{
				return _szString;
			}

			__forceinline char_t __fastcall operator[](_In_ uint_t uIndex) const
			{
				_ASSERTE(uIndex < GetSize());

				return _szString[uIndex];
			}

			__forceinline _Ret_maybenull_z_ const char_t* __fastcall begin() const
			{
				return this->GetConstString();
			}

			__forceinline _Ret_maybenull_z_ const char_t* __fastcall end() const
			{
				return this->GetConstString() + this->GetSize();
			}
		};

		template<>
		class StringView<achar_t, Encoding::ANSI_DEFAULT>
		{
		public:
			using char_t = achar_t;
		private:
			_Field_z_ const char_t* _szString;
			uint_t _cchString;
			Encoding eEncoding;
		public:
			StringView()
				: _szString(nullptr)
				, _cchString(0)
				, eEncoding(Encoding::ANSI_DEFAULT)
			{
			}

			StringView(_In_reads_opt_(cchSrc) const char_t* szSrc, _In_ uint_t cchSrc, _In_ Encoding _eEncoding = Encoding::ANSI_DEFAULT)
				: _szString(szSrc)
				, _cchString(cchSrc)
				, eEncoding(_eEncoding)
			{
			}

			StringView(_In_opt_z_ const char_t* szSrc, _In_ Encoding _eEncoding = Encoding::ANSI_DEFAULT)
				: _szString(szSrc)
				, _cchString(GetStringLength(szSrc))
				, eEncoding(_eEncoding)
			{
			}

			template<uint_t ArrayCount>
			StringView(const char_t (&szSrc)[ArrayCount], _In_ Encoding _eEncoding = Encoding::ANSI_DEFAULT)
				: _szString(szSrc)
				, _cchString(ArrayCount - 1)
				, eEncoding(_eEncoding)
			{
			}

			__forceinline Encoding __fastcall GetEncoding() const
			{
				return eEncoding;
			}

			__forceinline uint_t __fastcall GetSize() const
			{
				return _cchString;
			}

			__forceinline _Ret_maybenull_z_ const char_t* __fastcall GetConstString() const
			{
				return _szString;
			}

			__forceinline char_t __fastcall operator[](_In_ uint_t uIndex) const
			{
				_ASSERTE(uIndex < GetSize());

				return _szString[uIndex];
			}

			__forceinline  _Ret_maybenull_z_ const char_t* __fastcall begin() const
			{
				return this->GetConstString();
			}

			__forceinline  _Ret_maybenull_z_ const char_t* __fastcall end() const
			{
				return this->GetConstString() + this->GetSize();
			}
		};


		typedef StringView<achar_t, Encoding::ANSI_DEFAULT> aStringView;
		typedef StringView<u8char_t, Encoding::UTF8> u8StringView;
		typedef StringView<u16char_t, Encoding::UTF16LE> u16StringLEView;
		typedef StringView<u16char_t, Encoding::UTF16BE> u16StringBEView;
		typedef StringView<u32char_t, Encoding::UTF32LE> u32StringLEView;
		typedef StringView<u32char_t, Encoding::UTF32BE> u32StringBEView;

		typedef StringView<u16char_t, Encoding::UTF16> u16StringView;
		typedef StringView<u32char_t, Encoding::UTF32> u32StringView;
	}
}