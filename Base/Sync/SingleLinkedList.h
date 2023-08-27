#pragma once

#include <Base/Sync/Interlocked.h>

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
            template<class Entry>
            struct SingleLinkedEntryBase
            {
                Entry* pNext = nullptr;
            };

            template<class Entry>
            class SingleLinkedList
            {
            public:
                // 一般不应该直接操作此成员
                Entry volatile* pHead;

                SingleLinkedList()
                    : pHead(nullptr)
                {
                }

                ~SingleLinkedList()
                {
                    for (auto _pEntry = Flush(); _pEntry;)
                    {
                        auto _pNext = _pEntry->pNext;
                        delete _pEntry;
                        _pEntry = _pNext;
                    }
                }

                SingleLinkedList(const SingleLinkedList&) = delete;
                SingleLinkedList& operator=(const SingleLinkedList&) = delete;

                /// <summary>
                /// 向单向链表插入一段链表。
                /// <para/>注意：_pEntryBegin -> ... -> _pEntryEnd之间的元素必须首尾相连，不能产生环路，否则将死循环。
                /// <para/>注意：无锁单向链表为了实现无锁，其插入是倒序的。
                /// <para/>插入前：Head -> ABC
                /// <para/>插入后：Head -> _pEntryBegin -> ... -> _pEntryEnd -> ABC
                /// </summary>
                /// <param name="_pEntryBegin"></param>
                /// <param name="_pEntryEnd"></param>
                void Push(_In_ Entry* _pEntryBegin, _In_ Entry* _pEntryEnd)
                {
                    auto _pEntry = (Entry*)pHead;
                    for (;;)
                    {
                        _pEntryEnd->pNext = _pEntry;

                        auto _pLast = CompareExchangePoint(&pHead, _pEntryBegin, _pEntry);
                        if (_pLast != _pEntry)
                        {
                            _pEntry = _pLast;
                            // 锁定失败，换个姿势再来一次。
                            continue;
                        }
                        break;
                    }
                }

                /// <summary>
                /// 向单向链表插入一个元素。
                /// <para/>注意：无锁单向链表为了实现无锁，其插入是倒序的。
                /// <para/>插入前：Head -> ABC
                /// <para/>插入后：Head -> _pEntry -> ABC
                /// </summary>
                /// <param name="_pEntry">需要插入的元素。</param>
                void Push(_In_ Entry* _pEntry)
                {
                    return Push(_pEntry, _pEntry);
                }
                
                /// <summary>
                /// 从链表弹出一个元素。
                /// <para/>警告：PopUnsafe不能并行调用，这可能导致内存非法访问。
                /// <para/>警告：PopUnsafe不能与Flush并行调用，这可能导致内存非法访问。
                /// </summary>
                /// <returns>返回当前List头部元素。</returns>
                Entry* PopUnsafe()
                {
                    auto _pEntry = (Entry*)pHead;
                    for (;;)
                    {
                        if (_pEntry == nullptr)
                            break;

                        // 注意，如果多线程调用此函数，`_pEntry->pNext` 可能非法访问。
                        auto _pLast = CompareExchangePoint((Entry**)&pHead, _pEntry->pNext, _pEntry);
                        if (_pLast == _pEntry)
                        {
                            _pEntry->pNext = nullptr;
                            break;
                        }

                        // 交换失败，继续尝试
                        _pEntry = _pLast;
                    }

                    return _pEntry;
                }

                /// <summary>
                /// 清空当前列表，并返回之前链表的首个元素。
                /// <para/>警告：Flush不能与PopUnsafe并行调用，这可能导致内存非法访问。
                /// </summary>
                /// <returns>链表的首个元素地址。</returns>
                Entry* Flush()
                {
                    return ExchangePoint((Entry**)&pHead, (Entry*)nullptr);
                }
            };
        }

        using namespace YY::Base::Sync;
    } // namespace Base
} // namespace YY
