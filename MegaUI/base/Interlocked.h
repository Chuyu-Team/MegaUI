#pragma once

#include <intrin.h>

#include "MegaUITypeInt.h"


namespace YY
{
	namespace MegaUI
	{
		namespace Interlocked
		{
			__forceinline uint32_t __fastcall Increment(uint32_t* _Addend)
			{
				return _InterlockedIncrement(reinterpret_cast<long volatile*>(_Addend));
			}

			__forceinline uint32_t __fastcall Decrement(uint32_t* _Addend)
			{
				return _InterlockedDecrement(reinterpret_cast<long volatile*>(_Addend));
			}

			__forceinline bool __fastcall BitTestAndSet(uint32_t* _Base, uint32_t _Offset)
			{
				return _interlockedbittestandset(reinterpret_cast<long volatile*>(_Base), _Offset);
			}

			__forceinline bool __fastcall BitTestAndReset(uint32_t* _Base, uint32_t _Offset)
			{
				return _interlockedbittestandreset(reinterpret_cast<long volatile*>(_Base), _Offset);
			}
		}
	}
}