#pragma once
#include <stdlib.h>
#include <xmemory>

#include "MegaUITypeInt.h"

namespace YY
{
    namespace MegaUI
    {
        _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
        _CRTALLOCATOR
        inline void* __cdecl HAlloc(_In_ uint_t _Size)
        {
            return malloc(_Size);
        }

        _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
        _CRTALLOCATOR
        inline void* __cdecl HAllocAndZero(_In_ uint_t _Size)
        {
            return calloc(1, _Size);
        }

        _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
        _CRTALLOCATOR
        inline void* __cdecl HReAlloc(_Pre_maybenull_ _Post_invalid_ void* _Block, _In_ uint_t _Size)
        {
            return realloc(_Block, _Size);
        }
        
        _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
        _CRTALLOCATOR
        inline void* __cdecl HReAllocAndZero(_Pre_maybenull_ _Post_invalid_ void* _Block, _In_ uint_t _Size)
        {
            return _recalloc(_Block, 1, _Size);
        }

        inline void __cdecl HFree(_Pre_maybenull_ _Post_invalid_ void* _Block)
        {
            free(_Block);
        }

        template<typename T, typename ...Args>
        _Success_(return != NULL) _Check_return_ _Ret_maybenull_
        _CRTALLOCATOR
        inline T* HNew(Args... args)
        {
            T* _p = (T*)HAlloc(sizeof(T));
            if (_p)
                new (_p) T(args...);

            return _p;
        }

        template<typename T, typename... Args>
        _Success_(return != NULL) _Check_return_ _Ret_maybenull_
        _CRTALLOCATOR
        inline T* __cdecl HNewAndZero(Args... args)
        {
            T* _p = (T*)HAllocAndZero(sizeof(T));
            if (_p)
                new (_p) T(args...);

            return _p;
        }

        template<typename T>
        inline void __cdecl HDelete(_Pre_maybenull_ _Post_invalid_ T* _p)
        {
            if (_p)
            {
                _p->~T();
                HFree(_p);
            }
        }

        template<class _Ty>
        class allocator : public std::allocator<_Ty>
        {
        public:
            _Ty* __cdecl allocate(const uint_t _Count)
            {
                return HAlloc(_Count * sizeof(_Ty));
            }

            void __cdecl deallocate(_Ty* const _Ptr, const uint_t _Count)
            {
                HFree(_Ptr);
            }
        };

    } // namespace MegaUI
} // namespace YY