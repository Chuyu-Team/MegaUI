#pragma once
#include <Base/YY.h>
#include <intrin.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
#if defined(_AMD64_)
            using ::_umul128;
#else
            inline constexpr uint64_t __YYAPI _umul128(uint64_t _Multiplier, uint64_t _Multiplicand, uint64_t* _HighProduct) noexcept
            {
                uint32_t _MultiplierLow = static_cast<uint32_t>(_Multiplier);
                uint32_t _MultiplierHigh = static_cast<uint32_t>(_Multiplier >> 32);
                uint32_t _MultiplicandLow = static_cast<uint32_t>(_Multiplicand);
                uint32_t _MultiplicandHigh = static_cast<uint32_t>(_Multiplicand >> 32);

                uint64_t _LowProduct = static_cast<uint64_t>(_MultiplierLow) * _MultiplicandLow;
                uint64_t _MidProduct1 = static_cast<uint64_t>(_MultiplierLow) * _MultiplicandHigh;
                uint64_t _MidProduct2 = static_cast<uint64_t>(_MultiplierHigh) * _MultiplicandLow;
                uint64_t _HighProductTemp = static_cast<uint64_t>(_MultiplierHigh) * _MultiplicandHigh;

                uint64_t _Carry = (_LowProduct >> 32) + static_cast<uint32_t>(_MidProduct1) + static_cast<uint32_t>(_MidProduct2);
                *_HighProduct = _HighProductTemp + (_MidProduct1 >> 32) + (_MidProduct2 >> 32) + (_Carry >> 32);

                return (_LowProduct & 0xFFFFFFFF) | (_Carry << 32);
            }
#endif

#if defined(_AMD64_)
            using ::_udiv128;
#else
            inline constexpr uint64_t __YYAPI _udiv128(uint64_t _HighDividend, uint64_t _LowDividend, uint64_t _Divisor, uint64_t* _Remainder) noexcept
            {
                uint32_t _Ret[4];

                uint64_t _uRemainder = _HighDividend >> 32;
                _Ret[3] = _uRemainder / _Divisor;
                _uRemainder %= _Divisor;

                _uRemainder = (_uRemainder << 32) | (_HighDividend & 0xFFFFFFFFull);
                _Ret[2] = _uRemainder / _Divisor;
                _uRemainder %= _Divisor;

                _uRemainder = (_uRemainder << 32) | (_LowDividend >> 32);
                _Ret[1] = _uRemainder / _Divisor;
                _uRemainder %= _Divisor;

                _uRemainder = (_uRemainder << 32) | (_LowDividend & 0xFFFFFFFFull);
                _Ret[0] = _uRemainder / _Divisor;
                _uRemainder %= _Divisor;

                *_Remainder = _uRemainder;
                return *(uint64_t*)_Ret;
            }
#endif

            inline constexpr uint64_t __YYAPI UMulDiv64(
                _In_ uint64_t _uNumber,
                _In_ uint64_t _uNumerator,
                _In_ uint64_t _uDenominator) noexcept
            {
                uint64_t _uHigh64Bit = 0;
                auto _uLow64Bit = _umul128(_uNumber, _uNumerator, &_uHigh64Bit);

                uint64_t _Remainder;
                auto _uRet = _udiv128(_uHigh64Bit, _uLow64Bit, _uDenominator, &_Remainder);
                if (_Remainder)
                {
                    if ((_Remainder * 2) >= _uDenominator)
                    {
                        ++_uRet;
                    }
                }

                return _uRet;
            }

            /// <summary>
            /// 快速的 64 位乘除法，适用于_uNumerator 与 _uDenominator整数倍的情况
            /// </summary>
            /// <param name="_uNumber"></param>
            /// <param name="_uNumerator"></param>
            /// <param name="_uDenominator"></param>
            /// <returns></returns>
            inline constexpr uint64_t __YYAPI UMulDiv64Fast(
                _In_ uint64_t _uNumber,
                _In_ uint64_t _uNumerator,
                _In_ uint64_t _uDenominator) noexcept
            {
                if (_uNumerator == 0)
                    return 0;

                if (_uNumerator > _uDenominator)
                {
                    _uNumerator /= _uDenominator;
                    return _uNumber * _uNumerator;
                }
                else if (_uNumerator < _uDenominator)
                {
                    _uDenominator /= _uNumerator;
                    auto _uRet = _uNumber / _uDenominator;
                    if ((_uNumber % _uDenominator) * 2 >= _uDenominator)
                    {
                        ++_uRet;
                    }
                    return _uRet;
                }
                else
                {
                    return _uNumber;
                }
            }
            
            /// <summary>
            /// 快速的 64 位乘除法，适用于_uNumerator 与 _uDenominator整数倍的情况
            /// </summary>
            /// <param name="_uNumber"></param>
            /// <param name="_uNumerator"></param>
            /// <param name="_uDenominator"></param>
            /// <returns></returns>
            inline constexpr int64_t __YYAPI MulDiv64Fast(
                _In_ int64_t _nNumber,
                _In_ int64_t _nNumerator,
                _In_ int64_t _nDenominator) noexcept
            {
                bool _bNegative = false;
                if (_nNumerator == 0)
                {
                    return 0;
                }
                else if (_nNumerator < 0)
                {
                    _nNumerator *= -1;
                    _bNegative = !_bNegative;
                }

                if (_nDenominator < 0)
                {
                    _nDenominator *= -1;
                    _bNegative = !_bNegative;
                }

                int64_t _nRet;
                if (_nNumerator > _nDenominator)
                {
                    _nNumerator /= _nDenominator;
                    _nRet = _nNumber * _nNumerator;
                }
                else if (_nNumerator < _nDenominator)
                {
                    if (_nNumber < 0)
                    {
                        _nNumber *= -1;
                        _bNegative = !_bNegative;
                    }

                    _nDenominator /= _nNumerator;
                    _nRet = _nNumber / _nDenominator;
                    if ((_nNumber % _nDenominator) * 2 >= _nDenominator)
                    {
                        ++_nRet;
                    }
                }
                else
                {
                    _nRet = _nNumber;
                }

                return _bNegative ? _nRet * -1 : _nRet;
            }
        }
    } // namespace Base

    using namespace Base::Utils;
} // namespace YY

#pragma pack(pop)
