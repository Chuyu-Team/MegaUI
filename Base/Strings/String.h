
/*

此文件提供一种带 写拷贝（copy-on-write） 的String类。

此String类内部提供了简易的线程同步机制，但是并非完全，线程安全。



√  多线程读取，多个线程读取同一个String对象。
√  多线程读取，多个线程读取各自的String对象，但是它们是同一个引用。

╳  多线程写入，多个线程写入同一个String对象。
√  多线程写入，多个线程写入各自的String对象，但是他们是同一个引用。


LockBuffer 与 UnlockBuffer 必须成对出现。


*/

#pragma once

#include <stdlib.h>
#include <algorithm>
#include <limits>

#include <Base/Strings/StringView.h>
#include <Base/Encoding.h>
#include <Base/Exception.h>
#include <Base/YY.h>
#include <Base/ErrorCode.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/Alloc.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Strings
{
    class NString;
    class EndianHelper;

    template<class T, typename char_t, Encoding _eEncoding>
    class StringFunctionImp
    {
    public:
        HRESULT AppendFormatV(
            _In_z_ _Printf_format_string_ const char_t* _szFormat,
            _In_ va_list _args)
        {
            if (!_szFormat)
                return E_INVALIDARG;

            auto _pThis = static_cast<T*>(this);

            auto _uSize = _pThis->GetSize();
            auto _nAppendLength = GetStringFormatLength(_szFormat, _args);
            if (_nAppendLength < 0)
                return E_INVALIDARG;

            auto _szDstBuffer = _pThis->LockBuffer(_uSize + _nAppendLength);
            if (!_szDstBuffer)
                return E_OUTOFMEMORY;

            _nAppendLength = FormatStringV(_szDstBuffer + _uSize, _nAppendLength + 1, _szFormat, _args);
            if (_nAppendLength < 0)
            {
                _pThis->UnlockBuffer(_uSize);
                return E_INVALIDARG;
            }
            else
            {
                _pThis->UnlockBuffer(_uSize + _nAppendLength);
                return S_OK;
            }
        }

        HRESULT AppendFormat(
            _In_z_ _Printf_format_string_ const char_t* _szFormat,
            ...)
        {
            if (!_szFormat)
                return E_INVALIDARG;

            va_list _argList;
            va_start(_argList, _szFormat);

            auto _hr = AppendFormatV(_szFormat, _argList);

            va_end(_argList);

            return _hr;
        }

        HRESULT FormatV(
            _In_z_ _Printf_format_string_ const char_t* _szFormat,
            _In_ va_list _args)
        {
            if (!_szFormat)
                return E_INVALIDARG;

            auto _pThis = static_cast<T*>(this);

            _pThis->Clear();

            return AppendFormatV(_szFormat, _args);
        }

        HRESULT Format(
            _In_z_ _Printf_format_string_ const char_t* _szFormat,
            ...)
        {
            if (!_szFormat)
                return E_INVALIDARG;

            auto _pThis = static_cast<T*>(this);

            _pThis->Clear();

            va_list _argList;
            va_start(_argList, _szFormat);

            auto _hr = AppendFormatV(_szFormat, _argList);

            va_end(_argList);

            return _hr;
        }
    };

    template<class T>
    class StringFunctionImp<T,  u16char_t, Encoding::UTF16BE>
    {
    };

    template<class T>
    class StringFunctionImp<T,  u32char_t, Encoding::UTF32LE>
    {
    };

    template<class T>
    class StringFunctionImp<T,  u32char_t, Encoding::UTF32BE>
    {
    };

    template<typename _char_t, Encoding _eEncoding>
    class StringBase : public StringFunctionImp<StringBase<_char_t, _eEncoding>, _char_t, _eEncoding>
    {
    public:
        using char_t = _char_t;
        using StringView_t = StringView<char_t, _eEncoding>;

    private:
        constexpr static Encoding eEncoding = _eEncoding;

        friend NString;
        friend EndianHelper;

        _Field_z_ char_t* szString;

    public:
        explicit StringBase() noexcept
            : szString(StringData::GetEmtpyStringData()->GetStringBuffer())
        {
        }

        explicit StringBase(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
            : szString(StringData::GetEmtpyStringData()->GetStringBuffer())
        {
            auto _hr = SetString(_szSrc, _cchSrc);
            if (FAILED(_hr))
                throw Exception(_S("StringBase构造失败。"), _hr);
        }

        StringBase(_In_opt_z_ const char_t* _szSrc)
            : szString(StringData::GetEmtpyStringData()->GetStringBuffer())
        {
            if (!IsEmptyString(_szSrc))
            {
                auto _hr = SetString(_szSrc);
                if (FAILED(_hr))
                    throw Exception(_S("StringBase构造失败。"), _hr);
            }
        }

        template<size_t _uArrayCount>
        StringBase(const char_t (&_szSrc)[_uArrayCount])
            : szString(StringData::GetEmtpyStringData()->GetStringBuffer())
        {
            auto _hr = SetString(_szSrc, _uArrayCount - 1);
            if (FAILED(_hr))
                throw Exception(_S("StringBase构造失败。"), _hr);
        }

        StringBase(const StringView_t& _szSrc)
            : szString(StringData::GetEmtpyStringData()->GetStringBuffer())
        {
            auto _hr = SetString(_szSrc.GetConstString(), _szSrc.GetSize());
            if (FAILED(_hr))
                throw Exception(_S("StringBase构造失败。"), _hr);
        }

        StringBase(const StringBase& _szSrc)
        {
            auto _pStringDataOld = const_cast<StringBase&>(_szSrc).GetInternalStringData();

            if (_pStringDataOld->IsLocked())
            {
                szString = StringData::GetEmtpyStringData()->GetStringBuffer();
                auto _hr = SetString(_pStringDataOld->GetStringBuffer(), _pStringDataOld->uSize);
                if (FAILED(_hr))
                    throw Exception(_S("StringBase构造失败。"), _hr);
            }
            else
            {
                _pStringDataOld->AddRef();
                szString = _pStringDataOld->GetStringBuffer();
            }
        }

        StringBase(StringBase&& _szSrc) noexcept
            : szString(_szSrc.szString)
        {
            _szSrc.szString = StringData::GetEmtpyStringData()->GetStringBuffer();
        }

        ~StringBase()
        {
            GetInternalStringData()->Release();
        }

        uint_t __YYAPI GetSize() const
        {
            return GetInternalStringData()->uSize;
        }

        uint_t __YYAPI GetCapacity() const
        {
            return GetInternalStringData()->uCapacity;
        }

        _Ret_z_ const char_t* __YYAPI GetConstString() const
        {
            return szString;
        }

        _Ret_writes_maybenull_(_uCapacity) char_t* __YYAPI LockBuffer(_In_ size_t _uCapacity = 0)
        {
            auto _pInternalStringData = GetInternalStringData();

            if (_uCapacity < _pInternalStringData->uSize)
                _uCapacity = _pInternalStringData->uSize;

            if (_uCapacity == 0 && _pInternalStringData->uCapacity == 0)
            {
                // 因为Capacity 是0，其实它就是 EmtpyStringData。
                // 所以，我们什么也不做，直接返回即可。
                return szString;
            }

            if (_pInternalStringData->IsShared())
            {
                // 因为是共享缓冲区，所以我们需要复制
                auto _pNewStringData = _pInternalStringData->CloneStringData(_uCapacity);
                if (!_pNewStringData)
                    return nullptr;

                szString = _pNewStringData->GetStringBuffer();
                _pInternalStringData->Release();
                _pInternalStringData = _pNewStringData;
            }
            else if (_uCapacity > _pInternalStringData->uCapacity)
            {
                if (_pInternalStringData->IsLocked())
                {
                    throw Exception(_S("缓冲区已经锁定，无法调用 ReallocStringData。"));
                    return nullptr;
                }

                //当前缓冲区独享，并且需要扩容
                _pInternalStringData = StringData::ReallocStringData(_pInternalStringData, _uCapacity);
                if (!_pInternalStringData)
                    return nullptr;

                szString = _pInternalStringData->GetStringBuffer();
            }

            if (!_pInternalStringData->IsReadOnly())
                _pInternalStringData->Lock();

            return szString;
        }

        void __YYAPI UnlockBuffer(_In_ size_t _uNewSize)
        {
            auto _pInternalStringData = GetInternalStringData();

            if (!_pInternalStringData->IsReadOnly())
            {
                if (!_pInternalStringData->IsLocked())
                {
                    throw Exception(_S("缓冲区并未锁定，无法 UnlockBuffer。"));
                    return;
                }

                assert(_uNewSize <= _pInternalStringData->uCapacity);

                if (_pInternalStringData->uCapacity < _uNewSize)
                    _uNewSize = _pInternalStringData->uCapacity;

                _pInternalStringData->uSize = _uNewSize;
                szString[_uNewSize] = char_t('\0');
                _pInternalStringData->Unlock();
            }
        }

        void __YYAPI UnlockBuffer()
        {
            auto _pInternalStringData = GetInternalStringData();

            if (!_pInternalStringData->IsReadOnly())
            {
                _pInternalStringData->Unlock();
            }
        }

        void __YYAPI Clear()
        {
            auto _pInternalStringData = GetInternalStringData();

            if (_pInternalStringData->IsShared())
            {
                szString = StringData::GetEmtpyStringData()->GetStringBuffer();
                _pInternalStringData->Release();
            }
            else
            {
                szString[0] = char_t('\0');
                _pInternalStringData->uSize = 0;
            }
        }

        HRESULT __YYAPI SetString(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
        {
            if (_szSrc == nullptr && _cchSrc)
            {
                return E_INVALIDARG;
            }

            Clear();

            if (_cchSrc == 0)
            {
                return S_OK;
            }

            auto _szBuffer = LockBuffer(_cchSrc);
            if (!_szBuffer)
                return E_OUTOFMEMORY;

            memcpy(_szBuffer, _szSrc, _cchSrc * sizeof(char_t));
            UnlockBuffer(_cchSrc);

            return S_OK;
        }

        HRESULT __YYAPI SetString(_In_opt_z_ const char_t* _szSrc)
        {
            return SetString(_szSrc, GetStringLength(_szSrc));
        }

        template<size_t _uArrayCount>
        HRESULT __YYAPI SetString(const char_t (&_szSrc)[_uArrayCount])
        {
            return SetString(_szSrc, _uArrayCount - 1);
        }

        HRESULT __YYAPI SetString(const StringBase& _szSrc)
        {
            if (szString != _szSrc.szString)
            {
                auto _pStringDataOld = const_cast<StringBase&>(_szSrc).GetInternalStringData();

                if (_pStringDataOld->IsLocked())
                {
                    return SetString(_pStringDataOld->GetStringBuffer(), _pStringDataOld->uSize);
                }
                else
                {
                    _pStringDataOld->AddRef();
                    Attach(_pStringDataOld);
                }
            }

            return S_OK;
        }

        HRESULT __YYAPI SetString(StringBase&& _szSrc)
        {
            if (szString != _szSrc.szString)
            {
                if (_szSrc.GetInternalStringData()->IsLocked())
                {
                    throw Exception(_S("StringBase处于锁定状态，无法进行移动语义。"));
                    return E_UNEXPECTED;
                }

                Attach(_szSrc.Detach());
            }

            return S_OK;
        }

        HRESULT __YYAPI SetItem(_In_ size_t _uIndex, _In_ char_t _ch)
        {
            const auto _nSize = GetSize();

            if (_uIndex >= _nSize)
                return E_BOUNDS;

            auto _szBuffer = LockBuffer();
            if (!_szBuffer)
                return E_OUTOFMEMORY;

            _szBuffer[_uIndex] = _ch;

            UnlockBuffer();

            return S_OK;
        }

        HRESULT __YYAPI AppendString(_In_opt_z_ const char_t* _szSrc)
        {
            return AppendString(_szSrc, GetStringLength(_szSrc));
        }

        HRESULT __YYAPI AppendString(_In_reads_opt_(_cchSrc) const char_t* _szSrc, _In_ size_t _cchSrc)
        {
            if (_cchSrc == 0)
                return S_OK;

            if (_szSrc == nullptr)
                return E_INVALIDARG;

            auto _cchOldString = GetSize();
            const auto _cchNewString = _cchOldString + _cchSrc;

            auto _szBuffer = LockBuffer(_cchNewString);
            if (!_szBuffer)
                return E_OUTOFMEMORY;
            memcpy(_szBuffer + _cchOldString, _szSrc, _cchSrc * sizeof(char_t));
            UnlockBuffer(_cchNewString);

            return S_OK;
        }

        HRESULT __YYAPI AppendString(const StringView_t& _szSrc)
        {
            return AppendString(_szSrc.GetConstString(), _szSrc.GetSize());
        }

        HRESULT __YYAPI AppendString(const StringBase& _szSrc)
        {
            if (_szSrc.GetSize() == 0)
                return S_OK;

            if (GetSize() == 0)
            {
                return SetString(_szSrc);
            }
            else
            {
                return AppendString(_szSrc.szString, _szSrc.GetSize());
            }
        }

        HRESULT __YYAPI AppendChar(_In_ char_t ch)
        {
            auto _cchOldString = GetSize();
            const auto _cchNewString = _cchOldString + 1;

            auto _szBuffer = LockBuffer(_cchNewString);
            if (!_szBuffer)
                return E_OUTOFMEMORY;

            _szBuffer[_cchOldString] = ch;

            UnlockBuffer(_cchNewString);

            return S_OK;
        }

        Encoding __YYAPI GetEncoding() const
        {
            return eEncoding != Encoding::ANSI ? eEncoding : Encoding(GetInternalStringData()->eEncoding);
        }

        HRESULT __YYAPI SetANSIEncoding(_In_ Encoding _eNewEncoding)
        {
            if (GetEncoding() == _eNewEncoding)
                return S_OK;

            if (eEncoding != Encoding::ANSI)
            {
                // Unicode系列无法设置 ANSI代码页
                return E_NOINTERFACE;
            }

            if (_eNewEncoding == Encoding::UTF8 || _eNewEncoding == Encoding::UTF16LE || _eNewEncoding == Encoding::UTF16BE || _eNewEncoding == Encoding::UTF32LE || _eNewEncoding == Encoding::UTF32BE)
            {
                // 既然是个ANSI代码页，那么无法支持Unciode的编码
                return E_INVALIDARG;
            }

            // 防止优化到静态只读区，随便定个
            auto _szBuffer = LockBuffer(1u);
            if (!_szBuffer)
                return E_OUTOFMEMORY;

            GetInternalStringData()->eEncoding = uint16_t(_eNewEncoding);

            UnlockBuffer();

            return S_OK;
        }

        _Ret_z_ __YYAPI operator const char_t*() const
        {
            return szString;
        }

        __YYAPI operator StringView_t() const
        {
            return StringView_t(szString, GetSize());
        }

        char_t __YYAPI operator[](_In_ size_t _uIndex) const
        {
            assert(_uIndex < GetSize());

            return szString[_uIndex];
        }

        StringBase& __YYAPI operator=(_In_opt_z_ const char_t* _szSrc)
        {
            auto _hr = SetString(_szSrc);
            if (FAILED(_hr))
                throw Exception(_S("SetString失败！"), _hr);

            return *this;
        }

        StringBase& __YYAPI operator=(const StringBase& _szSrc)
        {
            auto _hr = SetString(_szSrc);
            if (FAILED(_hr))
                throw Exception(_S("SetString失败！"), _hr);

            return *this;
        }

        StringBase& __YYAPI operator=(StringBase&& _szSrc) noexcept
        {
            auto _hr = SetString(std::move(_szSrc));
            if (FAILED(_hr))
            {
                // move 失败
                abort();
            }
            return *this;
        }

        StringBase __YYAPI operator+(_In_opt_z_ const char_t* _szSrc)
        {
            auto _szTmp = *this;
            _szTmp += _szSrc;

            return _szTmp;
        }

        StringBase& __YYAPI operator+=(_In_opt_z_ const char_t* _szSrc)
        {
            auto _hr = AppendString(_szSrc);
            if (FAILED(_hr))
                throw Exception(_S("AppendString失败！"), _hr);

            return *this;
        }

        StringBase& __YYAPI operator+=(const StringBase& _szSrc)
        {
            auto _hr = AppendString(_szSrc);
            if (FAILED(_hr))
                throw Exception(_S("AppendString失败！"), _hr);

            return *this;
        }

        StringBase& __YYAPI operator+=(_In_ char_t _ch)
        {
            auto _hr = AppendChar(_ch);
            if (FAILED(_hr))
                throw Exception(_S("AppendChar失败！"), _hr);

            return *this;
        }
                
        bool __YYAPI operator==(const StringBase& _szSrc) const
        {
            if (this->GetSize() != _szSrc.GetSize())
                return false;

            return memcmp(this->GetConstString(), _szSrc.GetConstString(), this->GetSize() * sizeof(char_t)) == 0;
        }

        bool __YYAPI operator==(const StringView_t& _szSrc) const
        {
            if (this->GetSize() != _szSrc.GetSize())
                return false;

            return memcmp(this->GetConstString(), _szSrc.GetConstString(), this->GetSize() * sizeof(char_t)) == 0;
        }

        _Ret_z_ const char_t* __YYAPI begin() const
        {
            return this->GetConstString();
        }

        _Ret_z_ const char_t* __YYAPI end() const
        {
            return this->GetConstString() + this->GetSize();
        }

        int32_t __YYAPI CompareI(_In_ StringView<char_t, eEncoding> _Other) const
        {
            auto _iLeftSize = GetSize();
            auto _iRightSize = _Other.GetSize();
            auto _iMinSize = (std::min)(_iLeftSize, _iRightSize);

            uint_t _uIndex = 0;
            for (; _uIndex != _iMinSize; ++_uIndex)
            {
                int32_t _result = CharUpperAsASCII(szString[_uIndex]) - CharUpperAsASCII(_Other[_uIndex]);
                if (_result != 0)
                    return _result;
            }

            return (int32_t)int_t(_iLeftSize - _iRightSize);
        }

        struct StringData
        {
            union
            {
                struct
                {
                    // 暂不使用
                    uint16_t fMarks;
                    uint16_t eEncoding;
                    // 如果 >= 0，那么表示这块内存的引用次数
                    // 如果  < 0，那么表示这块内存的锁定次数，内存锁定时，内存引用计数隐式为 1。
                    int32_t iRef;
                };
                uint32_t fFlags;
                uint64_t uRawData;
            };

            // 内存的申请长度
            size_t uCapacity;
            // 字符串的实际长度，此长度不包含 0 终止。
            size_t uSize;

            // char_t szString[0];

            _Ret_maybenull_ StringData* __YYAPI CloneStringData(_In_ size_t _uAllocLength)
            {
                if (_uAllocLength < uSize)
                    _uAllocLength = uSize;

                if (_uAllocLength == 0)
                    return GetEmtpyStringData();

                auto _pNewStringData = AllocStringData(_uAllocLength);
                if (!_pNewStringData)
                    return nullptr;

                const auto _cbBuffer = uSize * sizeof(char_t);
                auto _szBuffer = _pNewStringData->GetStringBuffer();

                memcpy(_szBuffer, GetStringBuffer(), _cbBuffer);
                _szBuffer[uSize] = char_t('\0');

                return _pNewStringData;
            }

            static _Ret_maybenull_ StringData* __YYAPI ReallocStringData(_In_ StringData* _pOldStringData, _In_ uint_t _uAllocLength)
            {
                if (_pOldStringData->uCapacity >= _uAllocLength)
                    return _pOldStringData;

                // 因为字符串最后已 '0' 截断，所以我们有必要多申请一个
                ++_uAllocLength;

                // 默认尝试以1.5倍速度扩充
                auto _uNewCapacity = _pOldStringData->uCapacity + _pOldStringData->uCapacity / 2;
                if (_uNewCapacity < _uAllocLength)
                    _uNewCapacity = _uAllocLength;

                _uNewCapacity = (_uNewCapacity + 15) & ~15;

                const auto _cbStringDataBuffer = sizeof(StringData) + _uNewCapacity * sizeof(char_t);

                auto _pNewStringData = (StringData*)HReAlloc(_pOldStringData, _cbStringDataBuffer);
                if (!_pNewStringData)
                    return nullptr;

                _pNewStringData->uCapacity = _uNewCapacity - 1;

                return _pNewStringData;
            }

            static _Ret_maybenull_ StringData* __YYAPI AllocStringData(_In_ uint_t _uAllocLength)
            {
                if (_uAllocLength == 0)
                    return GetEmtpyStringData();

                ++_uAllocLength;

                _uAllocLength = (_uAllocLength + 15) & ~15;

                const auto _cbNewStringData = sizeof(StringData) + _uAllocLength * sizeof(char_t);

                auto _pNewStringData = (StringData*)HAlloc(_cbNewStringData);
                if (!_pNewStringData)
                    return nullptr;

                _pNewStringData->fMarks = 0;
                _pNewStringData->eEncoding = uint16_t(_eEncoding);
                _pNewStringData->iRef = 1;
                _pNewStringData->uCapacity = _uAllocLength - 1;
                _pNewStringData->uSize = 0u;

                auto _szBuffer = _pNewStringData->GetStringBuffer();
                _szBuffer[0] = char_t('\0');
                return _pNewStringData;
            }

            static _Ret_notnull_ StringData* __YYAPI GetEmtpyStringData()
            {
                struct StaticStringData
                {
                    StringData Base;
                    char_t szString[1];
                };

                const static StaticStringData g_EmptyStringData =
                {
                    {{0, uint16_t(_eEncoding), (std::numeric_limits<decltype(iRef)>::max)()}, 0, 0}
                };
                return const_cast<StringData*>(&g_EmptyStringData.Base);
            }

            _Ret_z_ char_t* __YYAPI GetStringBuffer()
            {
                return reinterpret_cast<char_t*>(this + 1);
            }

            uint32_t __YYAPI AddRef()
            {
                if (IsReadOnly())
                {
                    return (std::numeric_limits<decltype(iRef)>::max)();
                }

                if (iRef < 0)
                {
                    throw Exception(_S("缓冲区锁定时无法共享。"));
                    return 1;
                }

                return (uint32_t)Sync::Increment(&iRef);
            }

            uint32_t __YYAPI Release()
            {
                if (IsReadOnly())
                {
                    return (std::numeric_limits<decltype(iRef)>::max)();
                }

                if (iRef < 0)
                {
                    // 锁定时 隐含 内容引用计数 为 1，所以 Release 后将释放。
                    free(this);
                    return 0;
                }

                const auto uRefNew = Sync::Decrement(&iRef);
                if (uRefNew == 0)
                {
                    free(this);
                }

                return (uint32_t)uRefNew;
            }

            bool __YYAPI IsReadOnly()
            {
                return iRef == (std::numeric_limits<decltype(iRef)>::max)();
            }

            bool __YYAPI IsShared()
            {
                return iRef > 1;
            }

            void __YYAPI Lock()
            {
                if (iRef > 1 || iRef == 0)
                {
                    throw  Exception(_S("仅在非共享状态才允许锁定缓冲区。"));
                    return;
                }

                if (iRef == 1)
                {
                    iRef = -1;
                }
                else
                {
                    --iRef;
                }
            }

            void __YYAPI Unlock()
            {
                if (iRef >= 0)
                {
                    throw Exception(_S("缓冲区并未锁定，无法 Unlock。"));
                    return;
                }

                if (iRef == -1)
                {
                    iRef = 1;
                }
                else
                {
                    ++iRef;
                }
            }

            bool __YYAPI IsLocked()
            {
                return iRef < 0;
            }

            uint_t __YYAPI GetLockedCount()
            {
                if (iRef >= 0)
                    return 0;

                return iRef * -1;
            }
        };

    private:
        _Ret_notnull_ StringData* __YYAPI GetInternalStringData() const
        {
            return reinterpret_cast<StringData*>(szString) - 1;
        }

        /// <summary>
        /// 把内部指针挂接，注意此函数不会增加引用计数。
        /// </summary>
        /// <param name="_pNewStringData"></param>
        /// <returns></returns>
        void __YYAPI Attach(_In_ StringData* _pNewStringData)
        {
            auto _pOldStringData = GetInternalStringData();
            szString = _pNewStringData->GetStringBuffer();
            _pOldStringData->Release();
        }

        /// <summary>
        /// 注意把返回的指针释放。
        /// </summary>
        /// <typeparam name="_char_t"></typeparam>
        _Ret_notnull_ StringData* __YYAPI Detach()
        {
            auto _pStringData = GetInternalStringData();
            szString = StringData::GetEmtpyStringData()->GetStringBuffer();
            return _pStringData;
        }
    };

    typedef StringBase<achar_t, Encoding::ANSI> aString;
    typedef StringBase<u8char_t, Encoding::UTF8> u8String;
    typedef StringBase<u16char_t, Encoding::UTF16LE> u16StringLE;
    typedef StringBase<u16char_t, Encoding::UTF16BE> u16StringBE;
    typedef StringBase<u32char_t, Encoding::UTF32LE> u32StringLE;
    typedef StringBase<u32char_t, Encoding::UTF32BE> u32StringBE;

    typedef StringBase<u16char_t, Encoding::UTF16> u16String;
    typedef StringBase<u32char_t, Encoding::UTF32> u32String;

    typedef StringBase<wchar_t, Encoding::UTFW> wString;

    // 默认最佳的Unicode编码字符串
    typedef StringBase<uchar_t,  DetaultEncoding<uchar_t>::eEncoding> uString;
} // namespace YY::Base::Strings;

#pragma pack(pop)
