#pragma once

#include <intrin.h>
#include <Windows.h>
#include <winnt.h>

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
            __forceinline uint32_t __YYAPI Increment(uint32_t* _pAddend)
            {
                return (uint32_t)_InterlockedIncrement(reinterpret_cast<long volatile*>(_pAddend));
            }

            __forceinline int32_t __YYAPI Increment(int32_t* _pAddend)
            {
                return (int32_t)_InterlockedIncrement(reinterpret_cast<long volatile*>(_pAddend));
            }

            __forceinline uint64_t __YYAPI Increment(uint64_t* _pAddend)
            {
                return (uint64_t)_interlockedincrement64(reinterpret_cast<long long volatile*>(_pAddend));
            }

            __forceinline int64_t __YYAPI Increment(int64_t* _pAddend)
            {
                return (int64_t)_interlockedincrement64(reinterpret_cast<long long volatile*>(_pAddend));
            }

            __forceinline uint32_t __YYAPI Decrement(uint32_t* _pAddend)
            {
                return (uint32_t)_InterlockedDecrement(reinterpret_cast<long volatile*>(_pAddend));
            }
            
            __forceinline int32_t __YYAPI Decrement(int32_t* _pAddend)
            {
                return (int32_t)_InterlockedDecrement(reinterpret_cast<long volatile*>(_pAddend));
            }

            __forceinline uint64_t __YYAPI Decrement(uint64_t* _pAddend)
            {
                return (uint64_t)_interlockeddecrement64(reinterpret_cast<long long volatile*>(_pAddend));
            }

            __forceinline int64_t __YYAPI Decrement(int64_t* _pAddend)
            {
                return (int64_t)_interlockeddecrement64(reinterpret_cast<long long volatile*>(_pAddend));
            }

            /// <summary>
            /// 使用原子操作，将指定位Bit位设为 1
            /// </summary>
            /// <param name="_pBase"></param>
            /// <param name="_uOffset">指定bit</param>
            /// <returns>该 _uOffset bit位，原始值</returns>
            __forceinline bool __YYAPI BitSet(uint32_t* _pBase, uint32_t _uOffset)
            {
                return _interlockedbittestandset(reinterpret_cast<long volatile*>(_pBase), _uOffset);
            }

            /// <summary>
            /// 使用原子操作，将指定位Bit位设为 0
            /// </summary>
            /// <param name="_pBase"></param>
            /// <param name="_uOffset">指定bit</param>
            /// <returns>该 _uOffset bit位，原始值</returns>
            __forceinline bool __YYAPI BitReset(uint32_t* _pBase, uint32_t _uOffset)
            {
                return _interlockedbittestandreset(reinterpret_cast<long volatile*>(_pBase), _uOffset);
            }

            __forceinline int32_t __YYAPI CompareExchange(int32_t* _pDestination, int32_t _iExchange, int32_t _iComparand)
            {
                return (int32_t)_InterlockedCompareExchange(reinterpret_cast<long volatile*>(_pDestination), _iExchange, _iComparand);
            }

            __forceinline int64_t __YYAPI CompareExchange(int64_t* _pDestination, int64_t _iExchange, int64_t _iComparand)
            {
                return (int64_t)_InterlockedCompareExchange64(reinterpret_cast<long long volatile*>(_pDestination), _iExchange, _iComparand);
            }

            template<typename _Type>
            __forceinline _Type* __YYAPI CompareExchangePoint(_Type** _ppDestination, _Type* _pExchange, _Type* _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            __forceinline int32_t __YYAPI Exchange(int32_t* _pDestination, int32_t _iExchange)
            {
                return (int32_t)_InterlockedExchange(reinterpret_cast<long volatile*>(_pDestination), _iExchange);
            }

            __forceinline int64_t __YYAPI Exchange(int64_t* _pDestination, int64_t _iExchange)
            {
                return (int64_t)_interlockedexchange64(reinterpret_cast<long long volatile*>(_pDestination), _iExchange);
            }

            template<typename _Type>
            __forceinline _Type* __YYAPI ExchangePoint(_Type** _ppDestination, _Type* _pExchange)
            {
                return (_Type*)Exchange((intptr_t*)_ppDestination, (intptr_t)_pExchange);
            }

        } // namespace Sync
    } // namespace Base

    using namespace YY::Base;
} // namespace YY