#pragma once
#include <vector>

#include "alloc.h"

namespace YY
{
	namespace MegaUI
	{
		template <typename T>
		class DynamicArray : protected std::vector<T, YY::MegaUI::allocator<T>>
		{
		public:
			uint_t __fastcall GetSize()
			{
				return this->size();
			}
			
			HRESULT __fastcall Reserve(uint_t uNewCapacity)
			{
				try
				{
					this->reserve(uNewCapacity);

					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}
			
			HRESULT __fastcall Resize(uint_t NewSize)
			{
				try
				{
					this->resize(NewSize);
					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}

			T* __fastcall GetItemPtr(uint_t Index)
			{
				if (this->size() <= Index)
					return nullptr;

				return this->data() + Index;
			}

			HRESULT __fastcall SetItem(uint_t Index, const T& NewItem)
			{
				auto pItem = GetItemPtr(Index);
				if (!pItem)
					return E_INVALIDARG;

				*pItem = NewItem;
				return S_OK;
			}

			HRESULT __fastcall SetItem(uint_t Index, T&& NewItem)
			{
				auto pItem = GetItemPtr(Index);
				if (!pItem)
					return E_INVALIDARG;

				*pItem = std::move(NewItem);
				return S_OK;
			}

			HRESULT __fastcall Add(const T& NewItem)
			{
				try
				{
					this->push_back(NewItem);
					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}

			HRESULT __fastcall Add(T&& NewItem)
			{
				try
				{
					this->push_back(std::move(NewItem));
					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}

			HRESULT __fastcall Insert(uint_t Index, const T& NewItem)
			{
				const auto Size = this->size();

				if (Index > Size)
					return E_INVALIDARG;

				if (Size == Index)
					return Add(NewItem);

				try
				{
					this->insert(this->begin() + Index, NewItem);
					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}

			HRESULT __fastcall Insert(uint_t Index, T&& NewItem)
			{
				const auto Size = this->size();

				if (Index > Size)
					return E_INVALIDARG;

				if (Size == Index)
					return Add(std::move(NewItem));

				try
				{
					this->insert(this->begin() + Index, std::move(NewItem));
					return S_OK;
				}
				catch (const std::exception&)
				{
					return E_OUTOFMEMORY;
				}
			}

			auto begin() noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::begin();
			}

			auto end() noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::end();
			}

			auto _Unchecked_begin() noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::_Unchecked_begin();
			}

			auto _Unchecked_begin() const noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::_Unchecked_begin();
			}

			auto _Unchecked_end() noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::_Unchecked_end();
			}

			auto _Unchecked_end() const noexcept
			{
				return std::vector<T, YY::MegaUI::allocator<T>>::_Unchecked_end();
			}
		};

	}
}