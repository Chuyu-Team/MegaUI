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
			size_t __fastcall GetSize()
			{
				return this->size();
			}

			HRESULT __fastcall Resize(size_t NewSize)
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

			T* __fastcall GetItemPtr(size_t Index)
			{
				if (this->size() <= Index)
					return nullptr;

				return this->data() + Index;
			}

			HRESULT __fastcall SetItem(size_t Index, const T& NewItem)
			{
				auto pItem = GetItemPtr(Index);
				if (!pItem)
					return E_INVALIDARG;

				*pItem = NewItem;
				return S_OK;
			}

			HRESULT __fastcall SetItem(size_t Index, T&& NewItem)
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

			HRESULT __fastcall Insert(size_t Index, const T& NewItem)
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

			HRESULT __fastcall Insert(size_t Index, T&& NewItem)
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
		};

	}
}