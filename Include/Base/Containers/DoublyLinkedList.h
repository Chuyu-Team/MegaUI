#pragma once
#include <Base/YY.h>
#include <type_traits>
#include <assert.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            template<typename EntryType>
            struct DoublyLinkedListEntryImpl
            {
                EntryType* pPrior = nullptr;
                EntryType* pNext = nullptr;
            };

            /// <summary>
            /// 原始形式的双向链表。
            /// * 不支持多线程操作，需要自己加锁。
            /// * 不会自动释放内存，请自己释放内存。
            /// </summary>
            /// <typeparam name="EntryType"></typeparam>
            template<typename EntryType>
            class DoublyLinkedList
            {
            private:
                EntryType* pFirst = nullptr;
                EntryType* pLast = nullptr;

            public:
                constexpr DoublyLinkedList() = default;

                constexpr DoublyLinkedList(DoublyLinkedList&& _oList) noexcept
                {
                    pFirst = _oList.pFirst;
                    pLast = _oList.pLast;
                    _oList.pFirst = nullptr;
                    _oList.pLast = nullptr;
                }

                DoublyLinkedList(const DoublyLinkedList&) = delete;
                DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
                
                DoublyLinkedList __YYAPI Flush() noexcept
                {
                    DoublyLinkedList _oList;
                    _oList.pFirst = pFirst;
                    _oList.pLast = pLast;
                    pFirst = nullptr;
                    pLast = nullptr;
                    return _oList;
                }

                void __YYAPI PushBack(_In_ EntryType* _pEntry) noexcept
                {
                    _pEntry->pPrior = pLast;
                    _pEntry->pNext = nullptr;

                    if (pLast)
                    {
                        pLast->pNext = _pEntry;
                        pLast = _pEntry;
                    }
                    else
                    {
                        pFirst = pLast = _pEntry;
                    }
                }

                void __YYAPI PushBack(DoublyLinkedList&& _oList) noexcept
                {
                    if (this == &_oList || _oList.pFirst == nullptr)
                        return;

                    if (pLast)
                    {
                        pLast->pNext = _oList.pFirst;
                        _oList.pFirst->pPrior = pLast;
                        pLast = _oList.pLast;
                    }
                    else
                    {
                        pFirst = _oList.pFirst;
                        pLast = _oList.pLast;
                    }

                    _oList.pFirst = nullptr;
                    _oList.pLast = nullptr;
                }

                _Ret_maybenull_ EntryType* __YYAPI PopBack() noexcept
                {
                    auto _pRet = pLast;
                    if (_pRet)
                    {
                        pLast = pLast->pPrior;
                        if (pLast)
                        {
                            pLast->pNext = nullptr;
                        }
                        else
                        {
                            pFirst = nullptr;
                        }

                        _pRet->pPrior = nullptr;
                    }

                    return _pRet;
                }

                _Ret_maybenull_ EntryType* __YYAPI PopFront() noexcept
                {
                    auto _pRet = pFirst;
                    if (_pRet)
                    {
                        pFirst = pFirst->pNext;
                        if (pFirst)
                        {
                            pFirst->pPrior = nullptr;
                        }
                        else
                        {
                            pLast = nullptr;
                        }
                        _pRet->pNext = nullptr;
                    }

                    return _pRet;
                }

                void __YYAPI Insert(_In_ EntryType* _pWhere, _In_ EntryType* _pEntry) noexcept
                {
                    _pEntry->pPrior = _pWhere->pPrior;
                    _pEntry->pNext = _pWhere;

                    _pWhere->pPrior = _pEntry;

                    if (pFirst == _pWhere)
                    {
                        pFirst = _pEntry;
                    }
                }

                void __YYAPI Insert(_In_ EntryType* _pEntry) noexcept
                {
                    _pEntry->pPrior = nullptr;
                    _pEntry->pNext = pFirst;
                    if (pFirst)
                    {
                        pFirst->pPrior = _pEntry;
                        pFirst = _pEntry;
                    }
                    else
                    {
                        pFirst = pLast = _pEntry;
                    }
                }

                _Ret_maybenull_ constexpr EntryType* __YYAPI GetFirst() const noexcept
                {
                    return pFirst;
                }

                _Ret_maybenull_ constexpr EntryType* __YYAPI GetLast() const noexcept
                {
                    return pLast;
                }

                void __YYAPI Remove(_In_ EntryType* _pEntry) noexcept
                {
                    auto _pPrior = _pEntry->pPrior;
                    _pEntry->pPrior = nullptr;
                    auto _pNext = _pEntry->pNext;
                    _pEntry->pNext = nullptr;

                    if (_pPrior)
                    {
                        _pPrior->pNext = _pNext;
                    }
                    else
                    {
                        assert(pFirst == _pEntry);
                        pFirst = _pNext;
                    }

                    if (_pNext)
                    {
                        _pNext->pPrior = _pPrior;
                    }
                    else
                    {
                        assert(pLast == _pEntry);
                        pLast = _pPrior;
                    }
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
