#pragma once

#include <intrin.h>

#include "MegaUITypeInt.h"

namespace YY
{
    namespace MegaUI
    {
        namespace Interlocked
        {
            __forceinline uint32_t __MEGA_UI_API Increment(uint32_t* _Addend)
            {
                return _InterlockedIncrement(reinterpret_cast<long volatile*>(_Addend));
            }

            __forceinline int32_t __MEGA_UI_API Increment(int32_t* _Addend)
            {
                return _InterlockedIncrement(reinterpret_cast<long volatile*>(_Addend));
            }

            __forceinline uint32_t __MEGA_UI_API Decrement(uint32_t* _Addend)
            {
                return _InterlockedDecrement(reinterpret_cast<long volatile*>(_Addend));
            }
            
            __forceinline int32_t __MEGA_UI_API Decrement(int32_t* _Addend)
            {
                return _InterlockedDecrement(reinterpret_cast<long volatile*>(_Addend));
            }

            __forceinline bool __MEGA_UI_API BitTestAndSet(uint32_t* _Base, uint32_t _Offset)
            {
                return _interlockedbittestandset(reinterpret_cast<long volatile*>(_Base), _Offset);
            }

            __forceinline bool __MEGA_UI_API BitTestAndReset(uint32_t* _Base, uint32_t _Offset)
            {
                return _interlockedbittestandreset(reinterpret_cast<long volatile*>(_Base), _Offset);
            }
        } // namespace Interlocked
    }     // namespace MegaUI
} // namespace YY