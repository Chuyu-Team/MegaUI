#pragma once
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/Alloc.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
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
                _Ret_maybenull_ Entry* Pop() noexcept
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
                        HDelete(_pPendingDelete);
                    }

                    return nullptr;
                }

                void Push(_In_ Entry* _pEntry)
                {
                    if (!pLastWriteBlock)
                    {
                        pFirstReadBlock = pLastWriteBlock = HNew<Block>();
                    }

                    // 如果满了就尝试链接到下一块
                    if (pLastWriteBlock->IsFull())
                    {
                        auto _pNextBlock = HNew<Block>();
                        pLastWriteBlock->pNextBlock = _pNextBlock;
                        pLastWriteBlock = _pNextBlock;
                    }

                    pLastWriteBlock->arrLoopBuffer[pLastWriteBlock->uLastWriteIndex % uMaxBlockSize] = _pEntry;
                    pLastWriteBlock->uLastWriteIndex += 1;
                }
            };
        }
    }
} // namespace YY::Base::Sync

namespace YY
{
    using namespace YY::Base::Sync;
}

#pragma pack(pop)
