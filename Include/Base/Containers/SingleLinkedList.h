#pragma once
#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            template<typename EntryType>
            struct SingleLinkedListEntryImpl
            {
                EntryType* pNext = nullptr;
            };

            /// <summary>
            /// 原始的单项链表。
            /// * 不支持多线程操作，需要自己加锁。
            /// * 不会自动释放内存，请自己释放内存。
            /// </summary>
            /// <typeparam name="EntryType"></typeparam>
            template<typename EntryType>
            class SingleLinkedList
            {
            private:
                EntryType* pFirst = nullptr;
                EntryType* pLast = nullptr;
                
            public:
                constexpr SingleLinkedList() = default;

                constexpr SingleLinkedList(SingleLinkedList&& _oList) noexcept
                {
                    pFirst = _oList.pFirst;
                    pLast = _oList.pLast;
                    _oList.pFirst = nullptr;
                    _oList.pLast = nullptr;
                }

                SingleLinkedList(const SingleLinkedList&) = delete;
                SingleLinkedList& operator=(const SingleLinkedList&) = delete;
                
                _Ret_maybenull_ constexpr EntryType* __YYAPI GetFirst() const noexcept
                {
                    return pFirst;
                }

                _Ret_maybenull_ constexpr EntryType* __YYAPI GetLast() const noexcept
                {
                    return pLast;
                }

                _Ret_maybenull_ EntryType* __YYAPI Pop() noexcept
                {
                    if (pFirst == nullptr)
                        return nullptr;

                    auto _pOldFirst = pFirst;
                    pFirst = _pOldFirst->pNext;

                    if (!pFirst)
                    {
                        pLast = nullptr;
                    }

                    return _pOldFirst;
                }

                void __YYAPI Push(_In_ EntryType* _pEntry) noexcept
                {
                    _pEntry->pNext = nullptr;

                    if (pLast)
                    {
                        pLast->pNext = _pEntry;
                    }
                    else
                    {
                        pFirst = _pEntry;
                    }

                    pLast = _pEntry;
                }

                void __YYAPI Push(SingleLinkedList&& _oList) noexcept
                {
                    if (this == &_oList)
                        return;

                    if (_oList.IsEmpty())
                        return;

                    if (pLast == nullptr)
                    {
                        pFirst = _oList.pFirst;
                        pLast = _oList.pLast;
                    }
                    else
                    {
                        pLast->pNext = _oList.pFirst;
                        pLast = _oList.pLast;
                    }
                    _oList.pFirst = nullptr;
                    _oList.pLast = nullptr;
                }

                constexpr bool __YYAPI IsEmpty() const noexcept
                {
                    return pFirst == nullptr;
                }
            };

        }
    } // namespace Base

    using namespace YY::Base::Containers;
} // namespace YY

#pragma pack(pop)
