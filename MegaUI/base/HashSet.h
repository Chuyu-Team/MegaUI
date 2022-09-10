#pragma once

#include "MegaUITypeInt.h"
#include "alloc.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // 暂时仅适用于指针
        template<typename _Key, uint32_t _uBuckets = 128>
        class HashSet
        {
        private:
            struct HashSetEntry
            {
                HashSetEntry* pNext;
                _Key Data;
            };

            HashSetEntry* pBuckets[_uBuckets];

        public:
            HashSet()
                : pBuckets {}
            {
            }

            ~HashSet()
            {
                for (auto _pBucket : pBuckets)
                {
                    for (;;)
                    {
                        if (!_pBucket)
                            break;

                        auto _pTmp = _pBucket;
                        _pBucket = _pBucket->pNext;
                        HFree(_pTmp);
                    }
                }
            }

            _Key Pop()
            {
                for (auto& _pBucket : pBuckets)
                {
                    if (_pBucket)
                    {
                        auto _Tmp = _pBucket;
                        _pBucket = _Tmp->pNext;
                        return _Tmp->Data;
                    }
                }

                return nullptr;
            }

            HRESULT __MEGA_UI_API Insert(_Key _pKey)
            {
                const auto _uIndex = reinterpret_cast<uint_t>(_pKey) % _uBuckets;

                for (auto _pEntry = pBuckets[_uIndex]; _pEntry; _pEntry = _pEntry->pNext)
                {
                    if (_pEntry->Data == _pKey)
                        return S_FALSE;
                }

                auto pNewEntry = (HashSetEntry*) HAlloc(sizeof(HashSetEntry));
                if (!pNewEntry)
                    return E_OUTOFMEMORY;

                auto& _FirstEntry = pBuckets[_uIndex];

                pNewEntry->Data = _pKey;
                pNewEntry->pNext = _FirstEntry;
                _FirstEntry = pNewEntry;

                return S_OK;
            }

            HRESULT __MEGA_UI_API Remove(_Key _pKey)
            {
                const auto _uIndex = reinterpret_cast<uint_t>(_pKey) % _uBuckets;
                HashSetEntry* _pLastEntry = nullptr;

                for (auto _pEntry = pBuckets[_uIndex]; _pEntry; _pEntry = _pEntry->pNext)
                {
                    if (_pEntry->Data == _pKey)
                    {
                        auto _pNext = _pEntry->pNext;

                        if (_pLastEntry)
                        {
                            _pLastEntry->pNext = _pNext;
                        }
                        else
                        {
                            pBuckets[_uIndex] = _pNext;
                        }

                        HFree(_pEntry);

                        return S_OK;
                    }

                    _pLastEntry = _pEntry;
                }

                return S_FALSE;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
