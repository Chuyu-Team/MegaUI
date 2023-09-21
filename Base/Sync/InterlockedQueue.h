#pragma once
#include <Base/Sync/Interlocked.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Sync
        {


            //constexpr auto uMaxBlockSize = 16;
            // using Entry = int;

            template<class Entry, size_t uMaxBlockSize = 512, ProducerType eProducer = ProducerType::Single, ConsumerType eConsumer = ConsumerType::Single>
            class InterlockedQueue;

            template<class Entry, size_t uMaxBlockSize>
            class InterlockedQueue<Entry, uMaxBlockSize, ProducerType::Single, ConsumerType::Single>
            {
            private:
                struct Block
                {
                    size_t uLastReadIndex = 0;
                    size_t uLastWriteIndex = 0;
                    Block* pNextBlock = nullptr;
                    Entry* arrLoopBuffer[uMaxBlockSize];

                    bool IsEmpty()
                    {
                        return uLastReadIndex == uLastWriteIndex;
                    }

                    bool IsFull()
                    {
                        return uLastReadIndex + uMaxBlockSize == uLastWriteIndex;
                    }
                };

                Block* pFirstReadBlock = nullptr;
                Block* pLastWriteBlock = nullptr;

            public:
                Entry* Pop()
                {
                    if (!pFirstReadBlock)
                        return nullptr;

                    for (;;)
                    {
                        // 当前块任然有元素？
                        if (!pFirstReadBlock->IsEmpty())
                        {
                            auto _pTmp = pFirstReadBlock->arrLoopBuffer[pFirstReadBlock->uLastReadIndex % uMaxBlockSize];
                            pFirstReadBlock->uLastReadIndex += 1;
                            return _pTmp;
                        }

                        // 尝试流转到下一块
                        if (!pFirstReadBlock->pNextBlock)
                            return nullptr;

                        auto _pPendingDelete = pFirstReadBlock;
                        pFirstReadBlock = pFirstReadBlock->pNextBlock;
                        delete _pPendingDelete;
                    }

                    return nullptr;
                }

                void Push(_In_ Entry* _pEntry)
                {
                    if (!pLastWriteBlock)
                    {
                        pFirstReadBlock = pLastWriteBlock = new Block;
                    }

                    // 如果满了就尝试链接到下一块
                    if (pLastWriteBlock->IsFull())
                    {
                        auto _pNextBlock = new Block;
                        pLastWriteBlock->pNextBlock = _pNextBlock;
                        pLastWriteBlock = _pNextBlock;
                    }

                    pLastWriteBlock->arrLoopBuffer[pLastWriteBlock->uLastWriteIndex % uMaxBlockSize] = _pEntry;
                    pLastWriteBlock->uLastWriteIndex += 1;
                }
            };
        }
    } // namespace Base

    using namespace YY::Base::Sync;
} // namespace YY

#pragma pack(pop)
