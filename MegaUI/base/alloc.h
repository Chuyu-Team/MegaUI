#pragma once
#include <stdlib.h>
#include <xmemory>

namespace YY
{
	namespace MegaUI
	{
		inline void* __cdecl HAlloc(size_t s)
		{
			return malloc(s);
		}

		inline void* __cdecl HAllocAndZero(size_t s)
		{
			return calloc(1, s);
		}

		inline void* __cdecl HReAlloc(void* p, size_t s)
		{
			return realloc(p, s);
		}

		inline void* __cdecl HReAllocAndZero(void* p, size_t s)
		{
			return _recalloc(p, 1, s);
		}

		inline void __cdecl HFree(void* p)
		{
			free(p);
		}

		template <typename T> inline T* HNew()
		{
			T* p = (T*)HAlloc(sizeof(T));
			if (p)
				new(p) T;

			return p;
		}

		template <typename T> inline T* HNewAndZero()
		{
			T* p = (T*)HAllocAndZero(sizeof(T));
			if (p)
				new(p) T;

			return p;
		}

		template <typename T> inline void HDelete(T* p)
		{
			p->~T();
			HFree(p);
		}

		template <class _Ty>
		class allocator : public std::allocator<_Ty>
		{
		public:
			_Ty* allocate(const size_t _Count)
			{
				return HAlloc(_Count * sizeof(_Ty));
			}

			void deallocate(_Ty* const _Ptr, const size_t _Count)
			{
				HFree(_Ptr);
			}
		};

	}
}