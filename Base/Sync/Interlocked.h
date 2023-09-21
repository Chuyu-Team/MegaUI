#pragma once

#ifdef _WIN32
#include <intrin.h>
#include <Windows.h>
#include <winnt.h>
#endif

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
            enum class ProducerType
            {
                // 单生产者，不允许多个生产者同时调用
                Single,
                Multi,
            };

            enum class ConsumerType
            {
                // 单消费者，不允许多个消费者同时调用
                Single,
                Multi,
            };

            inline int32_t __YYAPI Increment(int32_t* _pAddend)
            {
#ifdef _MSC_VER
                return (int32_t)_InterlockedIncrement(reinterpret_cast<long volatile*>(_pAddend));
#else
                return __sync_add_and_fetch(_pAddend, 1);
#endif
            }

            inline uint32_t __YYAPI Increment(uint32_t* _pAddend)
            {
                return (uint32_t)Increment(reinterpret_cast<int32_t*>(_pAddend));
            }

            inline int64_t __YYAPI Increment(int64_t* _pAddend)
            {
#ifdef _MSC_VER
                return (int64_t)_interlockedincrement64(reinterpret_cast<long long volatile*>(_pAddend));
#else
                return __sync_add_and_fetch(_pAddend, 1);
#endif
            }

            inline uint64_t __YYAPI Increment(uint64_t* _pAddend)
            {
                return (uint64_t)Increment(reinterpret_cast<int64_t*>(_pAddend));
            }

            inline int32_t __YYAPI Decrement(int32_t* _pAddend)
            {
#ifdef _MSC_VER
                return (int32_t)_InterlockedDecrement(reinterpret_cast<long volatile*>(_pAddend));
#else
                return __sync_sub_and_fetch(_pAddend, 1);
#endif
            }

            inline uint32_t __YYAPI Decrement(uint32_t* _pAddend)
            {
                return (uint32_t)Decrement(reinterpret_cast<int32_t*>(_pAddend));
            }       

            inline int64_t __YYAPI Decrement(int64_t* _pAddend)
            {
#ifdef _MSC_VER
                return (int64_t)_interlockeddecrement64(reinterpret_cast<long long volatile*>(_pAddend));
#else
                return __sync_sub_and_fetch(_pAddend, 1);
#endif
            }

            inline uint64_t __YYAPI Decrement(uint64_t* _pAddend)
            {
                return (uint64_t)Decrement(reinterpret_cast<int64_t*>(_pAddend));
            }
            
            inline int32_t __YYAPI Add(int32_t* _pAddend, int32_t _iAdd)
            {
#ifdef _MSC_VER
                return InterlockedAdd((volatile LONG*)_pAddend, _iAdd);
#else
                return __sync_add_and_fetch(_pAddend, _iAdd);
#endif
            }

            inline uint32_t __YYAPI Add(uint32_t* _pAddend, uint32_t _uAdd)
            {
                return (uint32_t)Add((int32_t*)_pAddend, (int32_t)_uAdd);
            }

            inline int32_t __YYAPI Subtract(int32_t* _pAddend, int32_t _iAdd)
            {
                return Add(_pAddend, -_iAdd);
            }

            inline uint32_t __YYAPI Subtract(uint32_t* _pAddend, uint32_t _uAdd)
            {
                return (uint32_t)Add((int32_t*)_pAddend, -(int32_t)_uAdd);
            }

            /// <summary>
            /// 使用原子操作，将指定位Bit位设为 1
            /// </summary>
            /// <param name="_pBase"></param>
            /// <param name="_uOffset">指定bit</param>
            /// <returns>该 _uOffset bit位，原始值</returns>
            inline bool __YYAPI BitSet(uint32_t* _pBase, uint32_t _uOffset)
            {
#ifdef _MSC_VER
                return _interlockedbittestandset(reinterpret_cast<long volatile*>(_pBase), _uOffset);
#else
                const uint32_t _fAddMark = 1 << _uOffset;
                auto _uOld = __sync_fetch_and_or(_pBase, _fAddMark);
                return _uOld & _fAddMark;
#endif
            }

            /// <summary>
            /// 使用原子操作，将指定位Bit位设为 0
            /// </summary>
            /// <param name="_pBase"></param>
            /// <param name="_uOffset">指定bit</param>
            /// <returns>该 _uOffset bit位，原始值</returns>
            inline bool __YYAPI BitReset(uint32_t* _pBase, uint32_t _uOffset)
            {
#ifdef _MSC_VER
                return _interlockedbittestandreset(reinterpret_cast<long volatile*>(_pBase), _uOffset);
#else
                const uint32_t _fRemoveMark = 1 << _uOffset;
                auto _uOld = __sync_fetch_and_and(_pBase, ~_fRemoveMark);
                return _uOld & _fRemoveMark;
#endif
            }

            inline int32_t __YYAPI CompareExchange(int32_t* _pDestination, int32_t _iExchange, int32_t _iComparand)
            {
#ifdef _MSC_VER
                return (int32_t)_InterlockedCompareExchange(reinterpret_cast<long volatile*>(_pDestination), _iExchange, _iComparand);
#else
                return __sync_val_compare_and_swap(_pDestination, _iComparand, _iExchange);
#endif
            }

            inline uint32_t __YYAPI CompareExchange(uint32_t* _pDestination, uint32_t _iExchange, uint32_t _iComparand)
            {
                return (uint32_t)CompareExchange((int32_t*)_pDestination, (int32_t)_iExchange, (int32_t)_iComparand);
            }

            inline int64_t __YYAPI CompareExchange(int64_t* _pDestination, int64_t _iExchange, int64_t _iComparand)
            {
#ifdef _MSC_VER
                return (int64_t)_InterlockedCompareExchange64(reinterpret_cast<long long volatile*>(_pDestination), _iExchange, _iComparand);
#else
                return __sync_val_compare_and_swap(_pDestination, _iComparand, _iExchange);
#endif
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(_Type** _ppDestination, _Type* _pExchange, const _Type* _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(_Type** _ppDestination, _Type* _pExchange, std::nullptr_t _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(_Type** _ppDestination, std::nullptr_t _pExchange, const _Type* _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(_Type** _ppDestination, std::nullptr_t _pExchange, std::nullptr_t _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(volatile _Type** _ppDestination, _Type* _pExchange, _Type* _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(volatile _Type** _ppDestination, _Type* _pExchange, std::nullptr_t _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(volatile _Type** _ppDestination, std::nullptr_t _pExchange, _Type* _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            template<typename _Type>
            inline _Type* __YYAPI CompareExchangePoint(volatile _Type** _ppDestination, std::nullptr_t _pExchange, std::nullptr_t _pComparand)
            {
                return (_Type*)CompareExchange((intptr_t*)_ppDestination, (intptr_t)_pExchange, (intptr_t)_pComparand);
            }

            inline int32_t __YYAPI Exchange(int32_t* _pDestination, int32_t _iExchange)
            {
#ifdef _MSC_VER
                return (int32_t)_InterlockedExchange(reinterpret_cast<long volatile*>(_pDestination), _iExchange);
#else
                return __sync_lock_test_and_set(_pDestination, _iExchange);
#endif
            }

            inline uint32_t __YYAPI Exchange(uint32_t* _pDestination, uint32_t _iExchange)
            {
                return (uint32_t)Exchange((int32_t*)_pDestination, (int32_t)_iExchange);
            }

            inline int64_t __YYAPI Exchange(int64_t* _pDestination, int64_t _iExchange)
            {
#ifdef _MSC_VER
                return (int64_t)_interlockedexchange64(reinterpret_cast<long long volatile*>(_pDestination), _iExchange);
#else
                return __sync_lock_test_and_set(_pDestination, _iExchange);
#endif
            }

            template<typename _Type>
            inline _Type* __YYAPI ExchangePoint(_Type** _ppDestination, _Type* _pExchange)
            {
                return (_Type*)Exchange((intptr_t*)_ppDestination, (intptr_t)_pExchange);
            }

            template<typename _Type>
            inline _Type* __YYAPI ExchangePoint(_Type** _ppDestination, std::nullptr_t)
            {
                return (_Type*)Exchange((intptr_t*)_ppDestination, (intptr_t)0);
            }

            template<typename _Type>
            inline _Type* __YYAPI ExchangePoint(volatile _Type** _ppDestination, _Type* _pExchange)
            {
                return (_Type*)Exchange((intptr_t*)_ppDestination, (intptr_t)_pExchange);
            }

            template<typename _Type>
            inline _Type* __YYAPI ExchangePoint(volatile _Type** _ppDestination, std::nullptr_t)
            {
                return (_Type*)Exchange((intptr_t*)_ppDestination, (intptr_t)0);
            }

            inline int32_t __YYAPI BitOr(_In_ int32_t* _pDestination, int32_t _iValue)
            {
                auto _iLastVlaue = *_pDestination;

                for (;;)
                {
                    auto _iCurrentValue = CompareExchange(_pDestination, _iLastVlaue | _iValue, _iLastVlaue);
                    if (_iCurrentValue == _iLastVlaue)
                        break;

                    _iLastVlaue = _iCurrentValue;
                }

                return _iLastVlaue;
            }

            inline uint32_t __YYAPI BitOr(_In_ uint32_t* _pDestination, uint32_t _iValue)
            {
                return (uint32_t)BitOr((int32_t*)_pDestination, (int32_t)_iValue);
            }
            
            inline int32_t __YYAPI BitXor(_In_ int32_t* _pDestination, int32_t _iValue)
            {
                auto _iLastVlaue = *_pDestination;

                for (;;)
                {
                    auto _iCurrentValue = CompareExchange(_pDestination, _iLastVlaue ^ _iValue, _iLastVlaue);
                    if (_iCurrentValue == _iLastVlaue)
                        break;

                    _iLastVlaue = _iCurrentValue;
                }

                return _iLastVlaue;
            }

            inline uint32_t __YYAPI BitXor(_In_ uint32_t* _pDestination, uint32_t _iValue)
            {
                return (uint32_t)BitXor((int32_t*)_pDestination, (int32_t)_iValue);
            }
            
            inline int32_t __YYAPI BitAnd(_In_ int32_t* _pDestination, int32_t _iValue)
            {
                auto _iLastVlaue = *_pDestination;

                for (;;)
                {
                    auto _iCurrentValue = CompareExchange(_pDestination, _iLastVlaue & _iValue, _iLastVlaue);
                    if (_iCurrentValue == _iLastVlaue)
                        break;

                    _iLastVlaue = _iCurrentValue;
                }

                return _iLastVlaue;
            }

            inline uint32_t __YYAPI BitAnd(_In_ uint32_t* _pDestination, uint32_t _iValue)
            {
                return (uint32_t)BitAnd((int32_t*)_pDestination, (int32_t)_iValue);
            }
        } // namespace Sync
    } // namespace Base

    using namespace YY::Base;
} // namespace YY