#pragma once
#include <assert.h>
#include <utility>

#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>

#pragma pack(push, __YY_PACKING)

namespace UnitTest
{
    class BitMapUnitTest;
}

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            inline
            bool
            __YYAPI
            BitGet(
                _In_reads_bytes_((_uOffset / 8) + 1) const int32_t* _pBase,
                _In_ uint32_t _uOffset) noexcept
            {
#if defined(_WIN32)
                return ::_bittest(reinterpret_cast<const LONG*>(_pBase), _uOffset);
#else
                uint32_t _uMod = _uOffset % YY_bitsizeof(int32_t);

                return (_pBase[_uOffset / YY_bitsizeof(int32_t)] >> _uMod) & 1;
#endif
            }

            inline
            bool
            __YYAPI
            BitGet(
                _In_reads_bytes_((_uOffset / 8) + 1) const int64_t* _pBase,
                _In_ uint32_t _uOffset) noexcept
            {
#if defined(_WIN32) && (defined(_AMD64_) || defined(_ARM64_))
                return ::_bittest64(reinterpret_cast<const LONG64*>(_pBase), _uOffset);
#else
                return BitGet(reinterpret_cast<const int32_t*>(_pBase), _uOffset);
#endif
            }

            inline
            bool
            __YYAPI
            BitGet(
                _In_reads_bytes_((_uOffset / 8) + 1) const uint32_t* _pBase,
                _In_ uint32_t _uOffset)
            {
                return BitGet(reinterpret_cast<const int32_t*>(_pBase), _uOffset);
            }

            inline
            bool
            __YYAPI
            BitGet(
                _In_reads_bytes_((_uOffset / 8) + 1) const uint64_t* _pBase,
                _In_ uint32_t _uOffset) noexcept
            {
                return BitGet(reinterpret_cast<const int64_t*>(_pBase), _uOffset);
            }

            inline
            int32_t
            __YYAPI
            BitFind(
                _In_ const uint32_t* _puMask,
                _In_range_(< , 32) uint32_t _uIndex
                ) noexcept
            {
#if defined(_WIN32)
                DWORD _uNext;
                if (::_BitScanForward(&_uNext, *_puMask >> _uIndex))
                {
                    return (int32_t)_uIndex + _uNext;
                }
                else
                {
                    return -1;
                }
#else
                auto _uMask = *_puMask >> _uIndex;
                if (_uMask == 0)
                    return -1;

                return __builtin_ctzl(_uMask) + _uIndex;
#endif
            }

            inline
            int32_t
            __YYAPI
            BitFind(
                _In_ const uint64_t* _puMask,
                _In_range_(< , 64) uint32_t _uIndex
                )
            {
#if defined(_WIN32) && (defined(_AMD64_) || defined(_ARM64_))
                DWORD _uNext;
                if (::_BitScanForward64(&_uNext, *_puMask >> _uIndex))
                {
                    return (int32_t)_uIndex + _uNext;
                }
                else
                {
                    return -1;
                }
#else
                // 搜索低32位
                if (_uIndex < YY_bitsizeof(uint32_t))
                {
                    auto _nIndex = BitFind((uint32_t*)(_puMask), _uIndex);
                    if (_nIndex >= 0)
                    {
                        return _nIndex;
                    }
                }

                // 没有找到再搜索高 32位
                auto _nIndex = BitFind((uint32_t*)(_puMask)+1, _uIndex - (uint32_t)YY_bitsizeof(uint32_t));
                if (_nIndex >= 0)
                {
                    return _nIndex + YY_bitsizeof(uint32_t);
                }

                return false;
#endif
            }

            template<uint32_t uBits>
            class BitMap
            {
                friend UnitTest::BitMapUnitTest;

            protected:
                uintptr_t arrBits[(uBits + YY_bitsizeof(uintptr_t) - 1) / YY_bitsizeof(uintptr_t)];
                uint32_t uCount;

            public:
                static constexpr uint32_t MaxBitCount = uBits;

                BitMap()
                    : arrBits{}
                    , uCount(0u)
                {
                }

                constexpr uint32_t __YYAPI GetSize() const noexcept
                {
                    return uCount;
                }

                constexpr bool __YYAPI IsEmpty() const noexcept
                {
                    return uCount == 0u;
                }

                constexpr bool __YYAPI SetItem(_In_range_(< , uBits) uint32_t _uBitIndex, _In_ bool _bValue) noexcept
                {
                    assert(uBits > _uBitIndex);

                    if (_bValue)
                    {
                        const auto _bRet = Sync::BitSet(arrBits, _uBitIndex);
                        if (!_bRet)
                        {
                            Sync::Increment(&uCount);
                        }
                        return _bRet;
                    }
                    else
                    {
                        const auto _bRet = Sync::BitReset(arrBits, _uBitIndex);
                        if (_bRet)
                        {
                            Sync::Decrement(&uCount);
                        }
                        return _bRet;
                    }
                }

                constexpr bool __YYAPI GetItem(_In_range_(< , uBits) uint32_t _uBitIndex) const noexcept
                {
                    assert(uBits > _uBitIndex);

                    return BitGet(arrBits, _uBitIndex);
                }

                constexpr bool __YYAPI operator[](_In_range_(< , uBits) uint32_t _uBitIndex) const noexcept
                {
                    return GetItem(_uBitIndex);
                }

                constexpr int32_t __YYAPI Find(_In_range_(< , uBits) uint32_t _uBitIndex) const noexcept
                {
                    if (uBits <= _uBitIndex)
                    {
                        return -1;
                    }

                    auto _uBolckIndex = _uBitIndex / YY_bitsizeof(uintptr_t);
                    auto _uIndex = BitFind(&arrBits[_uBolckIndex], _uBitIndex % YY_bitsizeof(uintptr_t));
                    if (_uIndex >= 0)
                        return static_cast<int32_t>(_uIndex + _uBolckIndex * YY_bitsizeof(uintptr_t));

                    for (++_uBolckIndex; _uBolckIndex < std::size(arrBits); ++_uBolckIndex)
                    {
                        _uIndex = BitFind(&arrBits[_uBolckIndex], 0);
                        if (_uIndex >= 0)
                        {
                            return static_cast<int32_t>(_uIndex + _uBolckIndex * YY_bitsizeof(uintptr_t));
                        }
                    }

                    return -1;
                }
            };
        }
    }
}

namespace YY
{
    using namespace YY::Base::Containers;
}

#pragma pack(pop)
