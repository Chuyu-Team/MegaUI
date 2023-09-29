#pragma once
#include <stdlib.h>
#include <memory>

#include <Base/YY.h>

namespace YY
{
    namespace Base
    {
        namespace Memory
        {

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
                _CRTALLOCATOR
                inline void* HAlloc(_In_ size_t _Size)
            {
                return malloc(_Size);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
                _CRTALLOCATOR
                inline void* HAllocAndZero(_In_ size_t _Size)
            {
                return calloc(1, _Size);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
                _CRTALLOCATOR
                inline void* HReAlloc(_Pre_maybenull_ _Post_invalid_ void* _Block, _In_ size_t _Size)
            {
                return realloc(_Block, _Size);
            }

            _Success_(return != NULL) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
                _CRTALLOCATOR
                inline void* HReAllocAndZero(_Pre_maybenull_ _Post_invalid_ void* _Block, _In_ size_t _Size)
            {
#ifdef _MSC_VER
                return _recalloc(_Block, 1, _Size);
#else
                auto _pNewBlock = realloc(_Block, _Size);
                if (_pNewBlock)
                {
                    memset(_pNewBlock, 0, _Size);
                }
                return _pNewBlock;

#endif
            }

            inline void HFree(_Pre_maybenull_ _Post_invalid_ void* _Block)
            {
                free(_Block);
            }

            template<typename T, typename... Args>
            _Success_(return != NULL) _Check_return_ _Ret_maybenull_
                _CRTALLOCATOR
                inline T* HNew(Args&&... args)
            {
                T* _p = (T*)HAlloc(sizeof(T));
                if (_p)
                    new (_p) T(std::forward<Args>(args)...);

                return _p;
            }

            template<typename T, typename... Args>
            _Success_(return != NULL) _Check_return_ _Ret_maybenull_
                _CRTALLOCATOR
                inline T* HNewAndZero(Args&&... args)
            {
                T* _p = (T*)HAllocAndZero(sizeof(T));
                if (_p)
                    new (_p) T(std::forward<Args>(args)...);

                return _p;
            }

            template<typename T>
            inline void HDelete(_Pre_maybenull_ _Post_invalid_ T* _p)
            {
                if (_p)
                {
                    _p->~T();
                    HFree(_p);
                }
            }
        } // namespace Memory
    } // namespace Base
    using namespace YY::Base::Memory;
} // namespace YY