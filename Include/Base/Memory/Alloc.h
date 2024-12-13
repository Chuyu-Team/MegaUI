#pragma once
#include <stdlib.h>
#include <memory>
#include <string.h>

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Memory
        {
            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_cbSize)
            _CRTALLOCATOR
            inline void* Alloc(_In_ size_t _cbSize)
            {
                return malloc(_cbSize);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_cbSize)
            _CRTALLOCATOR
            inline void* AllocAndZero(_In_ size_t _cbSize)
            {
                return calloc(1, _cbSize);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_cbSize)
            _CRTALLOCATOR
            inline void* ReAlloc(_Pre_maybenull_ _Post_invalid_ void* _pBlock, _In_ size_t _cbSize)
            {
                return realloc(_pBlock, _cbSize);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_cbSize)
            _CRTALLOCATOR
            inline void* ReAllocAndZero(_Pre_maybenull_ _Post_invalid_ void* _pBlock, _In_ size_t _cbSize)
            {
#ifdef _MSC_VER
                return _recalloc(_pBlock, 1, _cbSize);
#else
                auto _pNewBlock = realloc(_pBlock, _cbSize);
                if (_pNewBlock)
                {
                    ::memset(_pNewBlock, 0, _cbSize);
                }
                return _pNewBlock;
#endif
            }

            inline void Free(_Pre_maybenull_ _Post_invalid_ void* _pBlock)
            {
                free(_pBlock);
            }

            template<typename T, typename... Args>
            _Success_(return != NULL) _Check_return_ _Ret_maybenull_
            _CRTALLOCATOR
            inline T* New(Args&&... args)
            {
                T* _p = (T*)Alloc(sizeof(T));
                if (_p)
                    new (_p) T(std::forward<Args>(args)...);

                return _p;
            }

            template<typename T, typename... Args>
            _Success_(return != NULL) _Check_return_ _Ret_maybenull_
            _CRTALLOCATOR
            inline T* NewAndZero(Args&&... args)
            {
                T* _p = (T*)AllocAndZero(sizeof(T));
                if (_p)
                    new (_p) T(std::forward<Args>(args)...);

                return _p;
            }

            template<typename T>
            inline void Delete(_Pre_maybenull_ _Post_invalid_ T* _pObject)
            {
                if (_pObject)
                {
                    _pObject->~T();
                    Free(_pObject);
                }
            }
        } // namespace Memory
    } // namespace Base
    using namespace YY::Base::Memory;
} // namespace YY
