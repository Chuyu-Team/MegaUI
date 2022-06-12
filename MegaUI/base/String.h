#pragma once


#include "MegaUITypeInt.h"
#include "alloc.h"
#include "Interlocked.h"
#include "ErrorCode.h"
#include "Encoding.h"
#include "StringView.h"

/*

此文件提供一种带 写拷贝（copy-on-write） 的String类。

此String类内部提供了简易的线程同步机制，但是并非完全，线程安全。



√  多线程读取，多个线程读取同一个String对象。
√  多线程读取，多个线程读取各自的String对象，但是它们是同一个引用。

╳  多线程写入，多个线程写入同一个String对象。
√  多线程写入，多个线程写入各自的String对象，但是他们是同一个引用。


LockBuffer 与 UnlockBuffer 必须成对出现，不能重入


*/


namespace YY
{
	namespace MegaUI
	{

		template<class T, typename char_t, Encoding _eEncoding>
		class StringFunctionImp
		{
		public:
			HRESULT AppendFormatV(
				_In_z_ _Printf_format_string_ const char_t* pszFormat,
				_In_ va_list args)
			{
				if (!pszFormat)
					return E_INVALIDARG;

				auto pThis = static_cast<T*>(this);

				auto uSize = pThis->GetSize();
				auto nAppendLength = GetStringFormatLength(pszFormat, args);
				if (nAppendLength < 0)
					return E_INVALIDARG;

				auto pszDstBuffer = pThis->LockBuffer(uSize + nAppendLength);
				if (!pszDstBuffer)
					return E_OUTOFMEMORY;

				nAppendLength = FormatStringV(pszDstBuffer + uSize, nAppendLength + 1, pszFormat, args);
				if (nAppendLength < 0)
				{
					pThis->UnlockBuffer(uSize);
					return E_INVALIDARG;
				}
				else
				{
					pThis->UnlockBuffer(uSize + nAppendLength);
					return S_OK;
				}
			}

			HRESULT __cdecl AppendFormat(
				_In_z_ _Printf_format_string_ const char_t* pszFormat,
				...)
			{
				if (!pszFormat)
					return E_INVALIDARG;

				va_list argList;
				va_start(argList, pszFormat);

				auto hr = AppendFormatV(pszFormat, argList);

				va_end(argList);

				return hr;
			}

			HRESULT FormatV(
				_In_z_ _Printf_format_string_ const char_t* pszFormat,
				_In_ va_list args)
			{
				if (!pszFormat)
					return E_INVALIDARG;

				auto pThis = static_cast<T*>(this);

				pThis->Clear();

				return AppendFormatV(pszFormat, args);
			}

			HRESULT __cdecl Format(
				_In_z_ _Printf_format_string_ const char_t* pszFormat,
				...)
			{
				if (!pszFormat)
					return E_INVALIDARG;

				auto pThis = static_cast<T*>(this);

				pThis->Clear();

				va_list argList;
				va_start(argList, pszFormat);

				auto hr = AppendFormatV(pszFormat, argList);

				va_end(argList);

				return hr;
			}
		};

		template<class T>
		class StringFunctionImp<T, u16char_t, Encoding::UTF16BE>
		{
		};

		template<class T>
		class StringFunctionImp<T, u32char_t, Encoding::UTF32LE>
		{
		};

		template<class T>
		class StringFunctionImp<T, u32char_t, Encoding::UTF32BE>
		{
		};

		class String;
		class EndianHelper;

		template<typename _char_t, Encoding _eEncoding>
		class StringBase : public StringFunctionImp<StringBase<_char_t, _eEncoding>, _char_t, _eEncoding>
		{
		public:
			using char_t = _char_t;
			using StringView_t = StringView<char_t, _eEncoding>;
		private:
			constexpr static Encoding eEncoding = _eEncoding;

			friend String;
			friend EndianHelper;

			_Field_z_ char_t* _szString;
		public:
			StringBase() noexcept
				: _szString(StringData::GetEmtpyStringData()->GetStringBuffer())
			{
				
			}

			StringBase(_In_reads_opt_(cchSrc) const char_t* szSrc, _In_ uint_t cchSrc)
				: _szString(StringData::GetEmtpyStringData()->GetStringBuffer())
			{
				SetString(szSrc, cchSrc);
			}

			StringBase(_In_opt_z_ const char_t* szSrc)
				: _szString(StringData::GetEmtpyStringData()->GetStringBuffer())
			{
				if (!IsEmptyString(szSrc))
				{
					SetString(szSrc);
				}
			}

			template<uint_t uArrayCount>
			StringBase(const char_t (&szSrc)[uArrayCount])
				: _szString(StringData::GetEmtpyStringData()->GetStringBuffer())
			{
				SetString(szSrc, uArrayCount - 1);
			}
			
			StringBase(const StringView_t& szSrc)
				: _szString(StringData::GetEmtpyStringData()->GetStringBuffer())
			{
				SetString(szSrc.GetConstString(), szSrc.GetSize());
			}

			StringBase(const StringBase& OldString)
			{
				auto& _OldString = const_cast<StringBase&>(OldString);

				auto pStringDataOld = _OldString._GetInternalStringData();

				if ((pStringDataOld->fFlags & StringDataFlags::Exclusive))
				{
					_szString = StringData::GetEmtpyStringData()->GetStringBuffer();
					SetString(pStringDataOld->GetStringBuffer(), pStringDataOld->uSize);
				}
				else
				{
					pStringDataOld->AddRef();
					_szString = pStringDataOld->GetStringBuffer();
				}
			}

			StringBase(StringBase&& OldString) noexcept
				: _szString(OldString._szString)
			{
				OldString._szString = StringData::GetEmtpyStringData()->GetStringBuffer();
			}


			~StringBase()
			{
				_GetInternalStringData()->Release();
			}

			uint_t __fastcall GetSize() const
			{
				return _GetInternalStringData()->uSize;
			}

			uint_t __fastcall GetCapacity() const
			{
				return _GetInternalStringData()->uCapacity;
			}

			_Ret_z_ const char_t* __fastcall GetConstString() const
			{
				return _szString;
			}

			_Ret_writes_maybenull_(uCapacity) char_t* __fastcall LockBuffer(_In_ uint_t uCapacity = 0)
			{
				auto pInternalStringData = _GetInternalStringData();

				// 锁定只能增加一次
				_ASSERTE((pInternalStringData->fFlags & StringDataFlags::Exclusive) == 0);

				if (pInternalStringData->fFlags & StringDataFlags::Exclusive)
					return nullptr;

				if (uCapacity < pInternalStringData->uSize)
					uCapacity = pInternalStringData->uSize;

				if (uCapacity == 0 && pInternalStringData->uCapacity == 0)
				{
					// 因为Capacity 是0，其实它就是 EmtpyStringData。
					// 所以，我们什么也不做，直接返回即可。
					return _szString;
				}

				if (pInternalStringData->IsShared())
				{
					// 因为是共享缓冲区，所以我们需要复制
					auto pNewStringData = pInternalStringData->CloneStringData(uCapacity);
					if (!pNewStringData)
						return nullptr;

					_szString = pNewStringData->GetStringBuffer();
					pInternalStringData->Release();
					pInternalStringData = pNewStringData;
				}
				else if (uCapacity > pInternalStringData->uCapacity)
				{
					//当前缓冲区独享，并且需要扩容
					pInternalStringData = StringData::ReallocStringData(pInternalStringData, uCapacity);
					if (!pInternalStringData)
						return nullptr;

					_szString = pInternalStringData->GetStringBuffer();
				}

				if (!pInternalStringData->IsReadOnly())
					pInternalStringData->fFlags |= StringDataFlags::Exclusive;

				return _szString;
			}

			void __fastcall UnlockBuffer(_In_ uint_t uNewSize)
			{
				auto pInternalStringData = _GetInternalStringData();

				if (!pInternalStringData->IsReadOnly())
				{
					_ASSERTE((pInternalStringData->fFlags & StringDataFlags::Exclusive) != 0);
					_ASSERTE(uNewSize <= pInternalStringData->uCapacity);

					if (pInternalStringData->uCapacity < uNewSize)
						uNewSize = pInternalStringData->uCapacity;

					pInternalStringData->fFlags &= ~StringDataFlags::Exclusive;
					pInternalStringData->uSize = uNewSize;
					_szString[uNewSize] = char_t('\0');
				}
			}


			void __fastcall UnlockBuffer()
			{
				auto pInternalStringData = _GetInternalStringData();

				if (!pInternalStringData->IsReadOnly())
				{
					_ASSERTE((pInternalStringData->fFlags & StringDataFlags::Exclusive) != 0);
					pInternalStringData->fFlags &= ~StringDataFlags::Exclusive;
				}
			}

			void __fastcall Clear()
			{
				auto pInternalStringData = _GetInternalStringData();

				if (pInternalStringData->IsShared())
				{
					_szString = StringData::GetEmtpyStringData()->GetStringBuffer();
					pInternalStringData->Release();
				}
				else
				{
					_szString[0] = char_t('\0');
					pInternalStringData->uSize = 0;
				}
			}

			HRESULT __fastcall SetString(_In_reads_opt_(cchSrc) const char_t* szSrc, _In_ uint_t cchSrc)
			{
				if (szSrc == nullptr && cchSrc)
				{
					return E_INVALIDARG;
				}

				Clear();

				if (cchSrc == 0)
				{
					return S_OK;
				}

				auto pszStringBuffer = LockBuffer(cchSrc);
				if (!pszStringBuffer)
					return E_OUTOFMEMORY;

				memcpy(pszStringBuffer, szSrc, cchSrc * sizeof(char_t));
				UnlockBuffer(cchSrc);

				return S_OK;
			}

			HRESULT __fastcall SetString(_In_opt_z_ const char_t* szSrc)
			{
				return SetString(szSrc, GetStringLength(szSrc));
			}

			template<uint_t uArrayCount>
			HRESULT __fastcall SetString(const char_t (&szSrc)[uArrayCount])
			{
				return SetString(szSrc, uArrayCount - 1);
			}

			HRESULT __fastcall SetString(const StringBase& szSrc)
			{
				if (_szString != szSrc._szString)
				{
					auto& _OldString = const_cast<StringBase&>(szSrc);

					auto pStringDataOld = _OldString._GetInternalStringData();

					if ((pStringDataOld->fFlags & StringDataFlags::Exclusive))
					{
						return SetString(pStringDataOld->GetStringBuffer(), pStringDataOld->uSize);
					}
					else
					{
						Attach(pStringDataOld);
					}
				}

				return S_OK;
			}

			HRESULT __fastcall SetString(StringBase&& szSrc)
			{
				if (_szString != szSrc._szString)
				{
					auto pNewData = szSrc.Detach();

					Attach(pNewData);

					pNewData->Release();
				}

				return S_OK;
			}


			HRESULT __fastcall SetItem(_In_ uint_t uIndex, _In_ char_t ch)
			{
				const auto nSize = GetSize();

				if (uIndex >= nSize)
					return E_BOUNDS;

				auto pszBuffer = LockBuffer();
				if (!pszBuffer)
					return E_OUTOFMEMORY;

				pszBuffer[uIndex] = ch;

				UnlockBuffer();

				return S_OK;
			}


			HRESULT __fastcall AppendString(_In_opt_z_ const char_t* szSrc)
			{
				return AppendString(szSrc, GetStringLength(szSrc));
			}

			HRESULT __fastcall AppendString(_In_reads_opt_(cchSrc) const char_t* szSrc, _In_ uint_t cchSrc)
			{
				if (cchSrc == 0)
					return S_OK;

				if (szSrc == nullptr)
					return E_INVALIDARG;

				auto cchOldString = GetSize();
				const auto cchNewString = cchOldString + cchSrc;

				auto pszStringBuffer = LockBuffer(cchNewString);
				if (!pszStringBuffer)
					return E_OUTOFMEMORY;
				memcpy(pszStringBuffer + cchOldString, szSrc, cchSrc * sizeof(char_t));
				UnlockBuffer(cchNewString);

				return S_OK;
			}

			HRESULT __fastcall AppendString(const StringView_t& szSrc)
			{
				return AppendString(szSrc.GetConstString(), szSrc.GetSize());
			}

			HRESULT __fastcall AppendString(const StringBase& szSrc)
			{
				if (szSrc.GetSize() == 0)
					return S_OK;

				if (GetSize() == 0)
				{
					return SetString(szSrc);
				}
				else
				{
					return AppendString(szSrc._szString, szSrc.GetSize());
				}
			}

			HRESULT __fastcall AppendChar(_In_ char_t ch)
			{
				auto cchOldString = GetSize();
				const auto cchNewString = cchOldString + 1;

				auto pszStringBuffer = LockBuffer(cchNewString);
				if (!pszStringBuffer)
					return E_OUTOFMEMORY;
				
				pszStringBuffer[cchOldString] = ch;

				UnlockBuffer(cchNewString);

				return S_OK;
			}

			Encoding __fastcall GetEncoding() const
			{
				return eEncoding != Encoding::ANSI_DEFAULT ? eEncoding : Encoding(_GetInternalStringData()->eEncoding);
			}

			HRESULT __fastcall SetANSIEncoding(_In_ Encoding eNewEncoding)
			{
				if (GetEncoding() == eNewEncoding)
					return S_OK;

				if (eEncoding != Encoding::ANSI_DEFAULT)
				{
					// Unicode系列无法设置 ANSI代码页
					return E_NOINTERFACE;
				}

				if (eNewEncoding == Encoding::UTF8
					|| eNewEncoding == Encoding::UTF16LE
					|| eNewEncoding == Encoding::UTF16BE
					|| eNewEncoding == Encoding::UTF32LE
					|| eNewEncoding == Encoding::UTF32BE)
				{
					// 既然是个ANSI代码页，那么无法支持Unciode的编码
					return E_INVALIDARG;
				}

				// 防止优化到静态只读区，随便定个
				auto pszBuffer = LockBuffer(1u);
				if (!pszBuffer)
					return E_OUTOFMEMORY;

				_GetInternalStringData()->eEncoding = uint16_t(eNewEncoding);

				UnlockBuffer();

				return S_OK;
			}

			_Ret_z_ __fastcall operator const char_t*() const
			{
				return _szString;
			}

			__fastcall operator StringView_t() const
			{
				return StringView_t(_szString, GetSize());
			}

			char_t __fastcall operator[](_In_ uint_t uIndex) const
			{
				_ASSERTE(uIndex < GetSize());

				return _szString[uIndex];
			}

			StringBase& __fastcall operator=(_In_opt_z_ const char_t* szSrc)
			{
				SetString(szSrc);
				return *this;
			}

			StringBase& __fastcall operator=(const StringBase& szSrc)
			{
				SetString(szSrc);
				return *this;
			}

			StringBase& __fastcall operator=(StringBase&& szSrc) noexcept
			{
				SetString(std::move(szSrc));
				return *this;
			}

			StringBase& __fastcall operator+=(_In_opt_z_ const char_t* szSrc) noexcept
			{
				AppendString(szSrc);

				return *this;
			}

			StringBase& __fastcall operator+=(const StringBase& szSrc) noexcept
			{
				AppendString(szSrc);

				return *this;
			}

			StringBase& __fastcall operator+=(_In_ char_t ch) noexcept
			{
				AppendChar(ch);

				return *this;
			}

			_Ret_z_ const char_t* __fastcall begin() const
			{
				return this->GetConstString();
			}

			_Ret_z_ const char_t* __fastcall end() const
			{
				return this->GetConstString() + this->GetSize();
			}

		private:
			enum StringDataFlags
			{
				// 缓冲区是独占的，阻止共享
				Exclusive              = 0x00000001,
			};

			struct StringData
			{
				union
				{					
					struct
					{
						uint16_t fMarks;
						uint16_t eEncoding;
						// 这块内存的引用次数
						uint32_t uRef;
					};
					uint32_t fFlags;
					uint64_t uRawData;
				};


				// 内存的申请长度
				uint_t uCapacity;
				// 字符串的实际长度，此长度不包含 0 终止。
				uint_t uSize;

				// char_t szString[0];


				_Ret_maybenull_ StringData* __fastcall CloneStringData(_In_ uint_t uAllocLength)
				{
					if (uAllocLength < uSize)
						uAllocLength = uSize;

					if (uAllocLength == 0)
						return GetEmtpyStringData();

					auto pNewStringData = AllocStringData(uAllocLength);
					if (!pNewStringData)
						return nullptr;

					const auto cbString = uSize * sizeof(char_t);
					auto pszString = pNewStringData->GetStringBuffer();

					memcpy(pszString, GetStringBuffer(), cbString);
					pszString[uSize] = char_t('\0');

					return pNewStringData;
				}

				static _Ret_maybenull_ StringData* __fastcall ReallocStringData(_In_ StringData* pOldStringData, _In_ uint_t uAllocLength)
				{
					if (pOldStringData->uCapacity >= uAllocLength)
						return pOldStringData;

					// 因为字符串最后已 '0' 截断，所以我们有必要多申请一个
					++uAllocLength;

					// 默认尝试以1.5倍速度扩充
					auto uNewCapacity = pOldStringData->uCapacity + pOldStringData->uCapacity / 2;
					if (uNewCapacity < uAllocLength)
						uNewCapacity = uAllocLength;

					uNewCapacity = (uNewCapacity + 15) & ~15;


					const auto cbStringDataBuffer = sizeof(StringData) + uNewCapacity * sizeof(char_t);

					auto pNewStringData = (StringData*)HReAlloc(pOldStringData, cbStringDataBuffer);
					if (!pNewStringData)
						return nullptr;
					
					pNewStringData->uCapacity = uNewCapacity - 1;

					return pNewStringData;
				}


				static _Ret_maybenull_ StringData* __fastcall AllocStringData(_In_ uint_t uAllocLength)
				{
					if (uAllocLength == 0)
						return GetEmtpyStringData();

					++uAllocLength;

					uAllocLength = (uAllocLength + 15) & ~15;

					const auto cbNewStringData = sizeof(StringData) + uAllocLength * sizeof(char_t);

					auto pNewStringData = (StringData*)HAlloc(cbNewStringData);
					if (!pNewStringData)
						return nullptr;


					pNewStringData->fMarks = 0;
					pNewStringData->eEncoding = uint16_t(_eEncoding);
					pNewStringData->uRef = 1u;
					pNewStringData->uCapacity = uAllocLength - 1;
					pNewStringData->uSize = 0u;

					auto pszString = pNewStringData->GetStringBuffer();
					pszString[0] = char_t('\0');
					return pNewStringData;
				}

				static _Ret_notnull_ StringData* __fastcall GetEmtpyStringData()
				{
					struct StaticStringData
					{
						StringData Base;
						char_t szString[1];
					};


					const static StaticStringData g_EmptyStringData = { { { 0, uint16_t(_eEncoding), uint32_max }, 0, 0 } };
					return const_cast<StringData*>(&g_EmptyStringData.Base);
				}


				_Ret_z_ char_t* __fastcall GetStringBuffer()
				{
					return reinterpret_cast<char_t*>(this + 1);
				}

				uint32_t __fastcall AddRef()
				{
					if (IsReadOnly())
					{
						return uint32_max;
					}

					return YY::MegaUI::Interlocked::Increment(&uRef);
				}

				uint32_t __fastcall Release()
				{
					if (IsReadOnly())
					{
						return uint32_max;
					}
					
					const auto uRefNew = YY::MegaUI::Interlocked::Decrement(&uRef);
					if (uRefNew == 0)
					{
						HFree(this);
					}

					return uRefNew;
				}

				bool __fastcall IsReadOnly()
				{
					return uRef == uint32_max;
				}

				bool __fastcall IsShared()
				{
					return uRef > 1;
				}
			};


			_Ret_notnull_ StringData* __fastcall _GetInternalStringData() const
			{
				return reinterpret_cast<StringData*>(_szString) -1;
			}

			void __fastcall Attach(_In_ StringData* pNewStringData)
			{
				auto Old = _GetInternalStringData();

				if (Old != pNewStringData)
				{
					pNewStringData->AddRef();
					_szString = pNewStringData->GetStringBuffer();
					Old->Release();
				}
			}
			
			_Ret_notnull_ StringData* __fastcall Detach()
			{
				auto pData = _GetInternalStringData();
				_szString = StringData::GetEmtpyStringData()->GetStringBuffer();
				return pData;
			}
		};
		



		typedef StringBase<achar_t, Encoding::ANSI_DEFAULT> aString;
		typedef StringBase<u8char_t, Encoding::UTF8> u8String;
		typedef StringBase<u16char_t, Encoding::UTF16LE> u16StringLE;
		typedef StringBase<u16char_t, Encoding::UTF16BE> u16StringBE;
		typedef StringBase<u32char_t, Encoding::UTF32LE> u32StringLE;
		typedef StringBase<u32char_t, Encoding::UTF32BE> u32StringBE;

		typedef StringBase<u16char_t, Encoding::UTF16> u16String;
		typedef StringBase<u32char_t, Encoding::UTF32> u32String;

		typedef StringBase<wchar_t, Encoding::UTFW> wString;


		// To ANSI
		HRESULT __fastcall Transform(const aStringView& szSrc, aString* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, aString* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, aString* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, aString* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, aString* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, aString* pszDst);

		HRESULT __fastcall Transform(const aString& szSrc, aString* pszDst);

		// To UTF8
		HRESULT __fastcall Transform(const aStringView& szSrc, u8String* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, u8String* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, u8String* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, u8String* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, u8String* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, u8String* pszDst);

		HRESULT __fastcall Transform(const u8String& szSrc, u8String* pszDst);

		// To UTF16LE
		HRESULT __fastcall Transform(const aStringView& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, u16StringLE* pszDst);

		HRESULT __fastcall Transform(const u16StringLE& szSrc, u16StringLE* pszDst);
		HRESULT __fastcall Transform(u16StringBE&& szSrc, u16StringLE* pszDst);

		// To UTF16BE
		HRESULT __fastcall Transform(const aStringView& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, u16StringBE* pszDst);

		HRESULT __fastcall Transform(u16StringLE&& szSrc, u16StringBE* pszDst);
		HRESULT __fastcall Transform(const u16StringBE& szSrc, u16StringBE* pszDst);

		//To UTF32LE
		HRESULT __fastcall Transform(const aStringView& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, u32StringLE* pszDst);

		HRESULT __fastcall Transform(const u32StringLE& szSrc, u32StringLE* pszDst);
		HRESULT __fastcall Transform(u32StringBE&& szSrc, u32StringLE* pszDst);

		//To UFT32BE
		HRESULT __fastcall Transform(const aStringView& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u8StringView& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u16StringLEView& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u16StringBEView& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u32StringLEView& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u32StringBEView& szSrc, u32StringBE* pszDst);

		HRESULT __fastcall Transform(u32StringLE&& szSrc, u32StringBE* pszDst);
		HRESULT __fastcall Transform(const u32StringBE& szSrc, u32StringBE* pszDst);

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
				u32String   szUTF32;
				u32StringLE szUTF32LE;
				u32StringBE szUTF32BE;
				wString szUTFW;
			};
		public:
			String()
				: szANSI()
			{
			}

			String(const aString& szSrc)
				: szANSI(szSrc)
			{
			}

			String(aString&& szSrc)
				: szANSI(std::move(szSrc))
			{
			}

			String(const u8String& szSrc)
				: szUTF8(szSrc)
			{
			}

			String(u8String&& szSrc)
				: szUTF8(std::move(szSrc))
			{

			}

			String(_In_reads_opt_(cchSrc) const wchar_t* szSrc, _In_ uint_t cchSrc)
				: szUTFW(szSrc, cchSrc)
			{
			}

			String(_In_opt_z_ const wchar_t* szSrc)
				: szUTFW(szSrc)
			{
			}

			template<uint_t uArrayCount>
			String(const wchar_t (&szSrc)[uArrayCount])
				: szUTFW(szSrc, uArrayCount -1)
			{
			}

			String(_In_reads_opt_(cchSrc) const char16_t* szSrc, _In_ uint_t cchSrc)
				: szUTF16((u16char_t*)szSrc, cchSrc)
			{
			}

			String(_In_opt_z_ const char16_t* szSrc)
				: szUTF16((u16char_t*)szSrc)
			{
			}

			template<uint_t uArrayCount>
			String(const char16_t (&szSrc)[uArrayCount])
				: szUTF16((u16char_t*)szSrc, uArrayCount - 1)
			{
			}

			String(_In_reads_opt_(cchSrc) const char32_t* szSrc, _In_ uint_t cchSrc)
				: szUTF32((u32char_t*)szSrc, cchSrc)
			{
			}

			String(_In_opt_z_ const char32_t* szSrc)
				: szUTF32((u32char_t*)szSrc)
			{
			}

			template<uint_t uArrayCount>
			String(const char32_t(&szSrc)[uArrayCount])
				: szUTF32((u32char_t*)szSrc, uArrayCount - 1)
			{
			}

			String(const u16StringLE& szSrc)
				: szUTF16LE(szSrc)
			{
			}

			String(u16StringLE&& szSrc)
				: szUTF16LE(std::move(szSrc))
			{
			}

			String(const u16StringBE& szSrc)
				: szUTF16BE(szSrc)
			{
			}

			String(u16StringBE&& szSrc)
				: szUTF16BE(std::move(szSrc))
			{
			}

			String(const u32StringView& szSrc)
				: szUTF32(szSrc.GetConstString(), szSrc.GetSize())
			{
			}

			String(const u32StringLE& szSrc)
				: szUTF32LE(szSrc)
			{
			}

			String(u32StringLE&& szSrc)
				: szUTF32LE(std::move(szSrc))
			{
			}

			String(const u32StringBE& szSrc)
				: szUTF32BE(szSrc)
			{
			}

			String(u32StringBE&& szSrc)
				: szUTF32BE(std::move(szSrc))
			{
			}

			~String()
			{
				szANSI.Detach()->Release();
			}

			Encoding __fastcall GetEncoding()
			{
				return Encoding(_GetInternalStringData()->eEncoding);
			}

			HRESULT __fastcall SetEncoding(Encoding eEncoding)
			{
				auto pStringData = _GetInternalStringData();
				const auto eOldEncoding = Encoding(pStringData->eEncoding);

				if (eOldEncoding == eEncoding)
					return S_OK;
				
				
				if (pStringData->uSize == 0)
				{
					_ClearAndUpdateEncoding(eEncoding);

					if (pStringData->IsShared() == false)
					{
						// 当数据独占时，我们直接更新 eEncoding 值即可。
						const auto OldCharSize = _GetCharSize(eOldEncoding);
						const auto NewCharSize = _GetCharSize(eEncoding);

						if (OldCharSize != NewCharSize)
						{
							pStringData->uCapacity = (pStringData->uCapacity + 1) * OldCharSize / NewCharSize - 1;
						}

						pStringData->eEncoding = (uint16_t)eEncoding;
						return S_OK;
					}
					else
					{
						// 尝试重置到默认值
						switch (eEncoding)
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
							return szANSI.SetANSIEncoding(eEncoding);
						}
					}
				}

				switch (eOldEncoding)
				{
				case YY::MegaUI::Encoding::UTF8:
				{
					auto szSrc(std::move(szUTF8));

					if (eEncoding == Encoding::UTF16LE)
					{
						return Transform(std::move(szSrc), &szUTF16LE);
					}
					else if (eEncoding == Encoding::UTF16BE)
					{
						return Transform(std::move(szSrc), &szUTF16BE);
					}
					else if (eEncoding == Encoding::UTF32LE)
					{
						return Transform(std::move(szSrc), &szUTF32LE);
					}
					else if (eEncoding == Encoding::UTF32BE)
					{
						return Transform(std::move(szSrc), &szUTF32BE);
					}
					else
					{
						// ANSI
						szANSI.Clear();
						if (!szANSI.LockBuffer(szSrc.GetSize() * 2))
							return E_OUTOFMEMORY;
						szANSI.UnlockBuffer(0);
						szANSI.SetANSIEncoding(eEncoding);

						return Transform(std::move(szSrc), &szANSI);
					}
				}
				case YY::MegaUI::Encoding::UTF16LE:
				{
					auto szSrc(std::move(szUTF16LE));

					if (eEncoding == Encoding::UTF8)
					{
						return Transform(std::move(szSrc), &szUTF8);
					}
					else if (eEncoding == Encoding::UTF16BE)
					{
						return Transform(std::move(szSrc), &szUTF16BE);
					}
					else if (eEncoding == Encoding::UTF32LE)
					{
						return Transform(std::move(szSrc), &szUTF32LE);
					}
					else if (eEncoding == Encoding::UTF32BE)
					{
						return Transform(std::move(szSrc), &szUTF32BE);
					}
					else
					{
						// ANSI
						szANSI.Clear();
						if (!szANSI.LockBuffer(szSrc.GetSize() * 2))
							return E_OUTOFMEMORY;
						szANSI.UnlockBuffer(0);
						szANSI.SetANSIEncoding(eEncoding);

						return Transform(std::move(szSrc), &szANSI);
					}
				}
				case YY::MegaUI::Encoding::UTF16BE:
				{
					auto szSrc(std::move(szUTF16BE));

					if (eEncoding == Encoding::UTF8)
					{
						return Transform(std::move(szSrc), &szUTF8);
					}
					else if (eEncoding == Encoding::UTF16LE)
					{
						return Transform(std::move(szSrc), &szUTF16LE);
					}
					else if (eEncoding == Encoding::UTF32LE)
					{
						return Transform(std::move(szSrc), &szUTF32LE);
					}
					else if (eEncoding == Encoding::UTF32BE)
					{
						return Transform(std::move(szSrc), &szUTF32BE);
					}
					else
					{
						// ANSI
						szANSI.Clear();
						if (!szANSI.LockBuffer(szSrc.GetSize() * 2))
							return E_OUTOFMEMORY;
						szANSI.UnlockBuffer(0);
						szANSI.SetANSIEncoding(eEncoding);
						return Transform(std::move(szSrc), &szANSI);
					}
				}
				case YY::MegaUI::Encoding::UTF32LE:
				{
					auto szSrc(std::move(szUTF32LE));

					if (eEncoding == Encoding::UTF8)
					{
						return Transform(std::move(szSrc), &szUTF8);
					}
					else if (eEncoding == Encoding::UTF16LE)
					{
						return Transform(std::move(szSrc), &szUTF16LE);
					}
					else if (eEncoding == Encoding::UTF16BE)
					{
						return Transform(std::move(szSrc), &szUTF16BE);
					}
					else if (eEncoding == Encoding::UTF32BE)
					{
						return Transform(std::move(szSrc), &szUTF32BE);
					}
					else
					{
						// ANSI
						szANSI.Clear();
						if (!szANSI.LockBuffer(szSrc.GetSize() * 2))
							return E_OUTOFMEMORY;
						szANSI.UnlockBuffer(0);
						szANSI.SetANSIEncoding(eEncoding);
						return Transform(std::move(szSrc), &szANSI);
					}
				}
				case YY::MegaUI::Encoding::UTF32BE:
				{
					auto szSrc(std::move(szUTF32BE));

					if (eEncoding == Encoding::UTF8)
					{
						return Transform(std::move(szSrc), &szUTF8);
					}
					else if (eEncoding == Encoding::UTF16LE)
					{
						return Transform(std::move(szSrc), &szUTF16LE);
					}
					else if (eEncoding == Encoding::UTF16BE)
					{
						return Transform(std::move(szSrc), &szUTF16BE);
					}
					else if (eEncoding == Encoding::UTF32LE)
					{
						return Transform(std::move(szSrc), &szUTF32LE);
					}
					else
					{
						// ANSI
						szANSI.Clear();
						if (!szANSI.LockBuffer(szSrc.GetSize() * 2))
							return E_OUTOFMEMORY;
						szANSI.UnlockBuffer(0);
						szANSI.SetANSIEncoding(eEncoding);
						return Transform(std::move(szSrc), &szANSI);
					}
				}
				default:
				{
					// ANSI
					auto szSrc(std::move(szANSI));

					if (eEncoding == Encoding::UTF8)
					{
						return Transform(std::move(szSrc), &szUTF8);
					}
					else if (eEncoding == Encoding::UTF16LE)
					{
						return Transform(std::move(szSrc), &szUTF16LE);
					}
					else if (eEncoding == Encoding::UTF16BE)
					{
						return Transform(std::move(szSrc), &szUTF16BE);
					}
					else if (eEncoding == Encoding::UTF32LE)
					{
						return Transform(std::move(szSrc), &szUTF32LE);
					}
					else if (eEncoding == Encoding::UTF32BE)
					{
						return Transform(std::move(szSrc), &szUTF32BE);
					}
					else
					{
						// ANSI
						auto hr = szSrc.SetANSIEncoding(eEncoding);
						if (FAILED(hr))
							return hr;

						return szANSI.SetString(std::move(szSrc));
					}
				}
				}
			}

			HRESULT __fastcall SetString(const aStringView& szSrc)
			{
				if (!IsANSI())
					_ClearAndUpdateEncoding(Encoding::ANSI_DEFAULT);

				auto hr = szANSI.SetString(szSrc);
				if (FAILED(hr))
					return hr;

				return szANSI.SetANSIEncoding(szSrc.GetEncoding());
			}
			HRESULT __fastcall SetString(const aString& szSrc)
			{
				if (!IsANSI())
					_ClearAndUpdateEncoding(Encoding::ANSI_DEFAULT);

				return szANSI.SetString(szSrc);
			}

			HRESULT __fastcall SetString(aString&& szSrc)
			{
				return szANSI.SetString(std::move(szSrc));
			}

			HRESULT __fastcall SetString(const u8StringView& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF8);

				return szUTF8.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u8String& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF8);

				return szUTF8.SetString(szSrc);
			}

			HRESULT __fastcall SetString(u8String&& szSrc)
			{
				return szUTF8.SetString(std::move(szSrc));
			}
			
			HRESULT __fastcall SetString(_In_reads_opt_(cchSrc) const char16_t* szSrc, _In_ uint_t cchSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16LE);

				return szUTF16.SetString((u16char_t*)szSrc, cchSrc);
			}

			HRESULT __fastcall SetString(_In_opt_z_ const char16_t* szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16LE);

				return szUTF16.SetString((u16char_t*)szSrc);
			}

			HRESULT __fastcall SetString(const u16StringLEView& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16LE);

				return szUTF16LE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u16StringBEView& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16BE);

				return szUTF16BE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u16StringLE& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16LE);

				return szUTF16LE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(u16StringLE&& szSrc)
			{
				return szUTF16LE.SetString(std::move(szSrc));
			}

			HRESULT __fastcall SetString(const u16StringBE& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF16BE);

				return szUTF16BE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(u16StringBE&& szSrc)
			{
				return szUTF16BE.SetString(std::move(szSrc));
			}

			HRESULT __fastcall SetString(_In_reads_opt_(cchSrc) const char32_t* szSrc, _In_ uint_t cchSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32LE);

				return szUTF32.SetString(szSrc, cchSrc);
			}

			HRESULT __fastcall SetString(_In_opt_z_ const char32_t* szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32LE);

				return szUTF32.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u32StringLEView& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32LE);

				return szUTF32LE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u32StringBEView& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32BE);

				return szUTF32BE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(const u32StringLE& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32LE);

				return szUTF32LE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(u32StringLE&& szSrc)
			{
				return szUTF32LE.SetString(std::move(szSrc));
			}

			HRESULT __fastcall SetString(const u32StringBE& szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTF32BE);

				return szUTF32BE.SetString(szSrc);
			}

			HRESULT __fastcall SetString(u32StringBE&& szSrc)
			{
				return szUTF32BE.SetString(std::move(szSrc));
			}

			HRESULT __fastcall SetString(_In_reads_opt_(cchSrc) const wchar_t* szSrc, _In_ uint_t cchSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTFW);

				return szUTFW.SetString(szSrc, cchSrc);
			}

			HRESULT __fastcall SetString(_In_opt_z_ const wchar_t* szSrc)
			{
				_ClearAndUpdateEncoding(Encoding::UTFW);

				return szUTFW.SetString(szSrc);
			}

			HRESULT __fastcall AppendString(const aStringView& szSrc)
			{
				return _AppendStringT(szSrc);
			}

			HRESULT __fastcall AppendString(const u8StringView& szSrc)
			{
				return _AppendStringT(szSrc);
			}

			HRESULT __fastcall AppendString(const u16StringLEView& szSrc)
			{
				return _AppendStringT(szSrc);
			}

			HRESULT __fastcall AppendString(const u16StringBEView& szSrc)
			{
				return _AppendStringT(szSrc);
			}


			HRESULT __fastcall AppendString(const u32StringLEView& szSrc)
			{
				return _AppendStringT(szSrc);
			}

			HRESULT __fastcall AppendString(const u32StringBEView& szSrc)
			{
				return _AppendStringT(szSrc);
			}

			HRESULT __fastcall GetString(aString* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				auto eDstEncoding = pszDst->GetEncoding();
				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					pszDst->SetANSIEncoding(eDstEncoding);
					return Transform(szUTF8, pszDst);
				case Encoding::UTF16LE:
					pszDst->SetANSIEncoding(eDstEncoding);
					return Transform(szUTF16LE, pszDst);
				case Encoding::UTF16BE:
					pszDst->SetANSIEncoding(eDstEncoding);
					return Transform(szUTF16BE, pszDst);
				case Encoding::UTF32LE:
					pszDst->SetANSIEncoding(eDstEncoding);
					return Transform(szUTF32LE, pszDst);
				case Encoding::UTF32BE:
					pszDst->SetANSIEncoding(eDstEncoding);
					return Transform(szUTF32BE, pszDst);
				default:
					return pszDst->SetString(szANSI);
				}
			}

			HRESULT __fastcall GetString(u8String* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					return pszDst->SetString(szUTF8);
				case Encoding::UTF16LE:
					return Transform(szUTF16LE, pszDst);
				case Encoding::UTF16BE:
					return Transform(szUTF16BE, pszDst);
				case Encoding::UTF32LE:
					return Transform(szUTF32LE, pszDst);
				case Encoding::UTF32BE:
					return Transform(szUTF32BE, pszDst);
				default:
					return Transform(szANSI, pszDst);
				}
			}

			HRESULT __fastcall GetString(u16StringLE* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					return Transform(szUTF8, pszDst);
				case Encoding::UTF16LE:
					return pszDst->SetString(szUTF16LE);
				case Encoding::UTF16BE:
					return Transform(szUTF16BE, pszDst);
				case Encoding::UTF32LE:
					return Transform(szUTF32LE, pszDst);
				case Encoding::UTF32BE:
					return Transform(szUTF32BE, pszDst);
				default:
					return Transform(szANSI, pszDst);
				}
			}

			HRESULT __fastcall GetString(u16StringBE* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					return Transform(szUTF8, pszDst);
				case Encoding::UTF16LE:
					return Transform(szUTF16LE, pszDst);
				case Encoding::UTF16BE:
					return pszDst->SetString(szUTF16BE);
				case Encoding::UTF32LE:
					return Transform(szUTF32LE, pszDst);
				case Encoding::UTF32BE:
					return Transform(szUTF32BE, pszDst);
				default:
					return Transform(szANSI, pszDst);
				}
			}

			HRESULT __fastcall GetString(u32StringLE* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					return Transform(szUTF8, pszDst);
				case Encoding::UTF16LE:
					return Transform(szUTF16LE, pszDst);
				case Encoding::UTF16BE:
					return Transform(szUTF16BE, pszDst);
				case Encoding::UTF32LE:
					return pszDst->SetString(szUTF32LE);
				case Encoding::UTF32BE:
					return Transform(szUTF32BE, pszDst);
				default:
					return Transform(szANSI, pszDst);
				}
			}

			HRESULT __fastcall GetString(u32StringBE* pszDst)
			{
				if (pszDst == nullptr)
					return E_POINTER;

				pszDst->Clear();

				switch (GetEncoding())
				{
				case Encoding::UTF8:
					return Transform(szUTF8, pszDst);
				case Encoding::UTF16LE:
					return Transform(szUTF16LE, pszDst);
				case Encoding::UTF16BE:
					return Transform(szUTF16BE, pszDst);
				case Encoding::UTF32LE:
					return Transform(szUTF32LE, pszDst);
				case Encoding::UTF32BE:
					return pszDst->SetString(szUTF32BE);
				default:
					return Transform(szANSI, pszDst);
				}
			}

			uint_t __fastcall GetSize()
			{
				// 长度大家都是一样的，所以随便返回一个即可
				return _GetInternalStringData()->uSize;
			}

			void __fastcall Clear()
			{
				auto pStringData = szANSI._GetInternalStringData();
				if (pStringData->uSize == 0)
					return;

				const auto OldEncoding = Encoding(pStringData->eEncoding);

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
				case YY::MegaUI::Encoding::ANSI_DEFAULT:
					szANSI.Clear();
					break;
				default:
					szANSI.Clear();
					szANSI.SetANSIEncoding(OldEncoding);
					break;
				}
			}

		private:
			aString::StringData* __fastcall _GetInternalStringData()
			{
				// 因为所有编码的字符串长度 以及 缓冲区大小格式都是一样的，
				// 所以随便取一个 _GetInternalStringData 就可以了。
				return szANSI._GetInternalStringData();
			}

			static constexpr uint32_t __fastcall _GetCharSize(Encoding eEncoding)
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

			bool __fastcall IsANSI()
			{
				return YY::MegaUI::IsANSI(GetEncoding());
			}

			HRESULT __fastcall _ClearAndUpdateEncoding(Encoding eEncoding)
			{
				auto pStringData = _GetInternalStringData();
				auto eOldEncoding = Encoding(pStringData->eEncoding);
				if (eOldEncoding == eEncoding)
					return S_OK;

				if (pStringData->IsShared())
				{
					switch (eEncoding)
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
						return szANSI.SetANSIEncoding(eEncoding);
					}
				}
				else
				{
					const auto OldCharSize = _GetCharSize(eOldEncoding);
					const auto NewCharSize = _GetCharSize(eEncoding);

					if (OldCharSize != NewCharSize)
					{
						pStringData->uCapacity = (pStringData->uCapacity + 1) * OldCharSize / NewCharSize - 1;
					}

					pStringData->eEncoding = uint16_t(eEncoding);
					pStringData->uSize = 0;
					memset(pStringData->GetStringBuffer(), 0, NewCharSize);
					return S_OK;
				}
			}


			template<class string_t>
			__forceinline HRESULT __fastcall _AppendStringT(const string_t& szSrc)
			{
				if (szSrc.GetSize() == 0)
					return S_OK;

				const auto eDstEncoding = GetEncoding();

				switch (eDstEncoding)
				{
				case Encoding::UTF8:
					return Transform(szSrc, &szUTF8);
				case Encoding::UTF16LE:
					return Transform(szSrc, &szUTF16LE);
				case Encoding::UTF16BE:
					return Transform(szSrc, &szUTF16BE);
				case Encoding::UTF32LE:
					return Transform(szSrc, &szUTF32LE);
				case Encoding::UTF32BE:
					return Transform(szSrc, &szUTF32BE);
				default:
					// ANSI
					return Transform(szSrc, &szANSI);
				}
			}

		};
	}
}
