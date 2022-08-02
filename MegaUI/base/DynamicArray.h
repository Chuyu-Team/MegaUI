#pragma once

#include "MegaUITypeInt.h"
#include "alloc.h"
#include "Exception.h"
#include "Interlocked.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        constexpr auto szReducedSizeIsNotSupportedBecauseBufferLocked = _S("Buffer处于锁定状态，无法减少 uSize。");
        constexpr auto szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked = _S("Buffer并未锁定，无法调用UnlockBuffer。");

        template<typename T, bool _bUsingWriteOnCopy>
        struct DynamicArraySharedPoint
        {
            using SharedPoint = const T*;
        };

        template<typename T>
        struct DynamicArraySharedPoint<T, false>
        {
            using SharedPoint = T*;
        };

        template<typename T, bool _bUsingClassConstructor = true>
        struct DynamicArrayConstructor
        {
            __forceinline static void __fastcall Init(_Out_ T* _pData)
            {
                new (_pData) T {};
            }

            __forceinline static void __fastcall Init(_Out_writes_(_uCount) T* _pData, _In_ uint_t _uCount)
            {
                const auto _pDataEnd = _pData + _uCount;
                for (; _pData != _pDataEnd; ++_pData)
                {
                    new (_pData) T {};
                }
            }
            
            __forceinline static void __fastcall Init(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) const T* _pDataSrc, _In_ uint_t _uCount)
            {
                const auto _pDataSrcEnd = _pDataSrc + _uCount;
                for (; _pDataSrc != _pDataSrcEnd; ++_pDataSrc, ++_pDataDst)
                {
                    new (_pDataDst) T(*_pDataSrc);
                }
            }

            __forceinline static void __fastcall Uninit(_In_ T* _pData)
            {
                _pData->~T();
            }

            __forceinline static void __fastcall Uninit(_In_ T* _pData, _In_ uint_t _uCount)
            {
                for (const auto _pDataEnd = _pData + _uCount; _pData != _pDataEnd; ++_pData)
                {
                    _pData->~T();
                }
            }

            __forceinline static void __fastcall InitAndMove(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) T* _pDataSrc, _In_ uint_t _uCount)
            {
                const auto _pDataSrcEnd = _pDataSrc + _uCount;
                for (; _pDataSrc != _pDataSrcEnd; ++_pDataSrc, ++_pDataDst)
                {
                    new (_pDataDst) T(std::move(*_pDataSrc));
                }
            }

            __forceinline static void __fastcall Copy(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) const T* _pDataSrc, _In_ uint_t _uCount)
            {
                const auto _pDataSrcEnd = _pDataSrc + _uCount;
                for (; _pDataSrc != _pDataSrcEnd; ++_pDataSrc, ++_pDataDst)
                {
                    *_pDataDst = *_pDataSrc;
                }
            }
            
            __forceinline static void __fastcall CopyAndMove(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) T* _pDataSrc, _In_ uint_t _uCount)
            {
                if (_pDataDst == _pDataSrc || _uCount == 0)
                    return;

                if (_pDataDst < _pDataSrc)
                {
                    const auto _pDataSrcEnd = _pDataSrc + _uCount;
                    for (; _pDataSrc != _pDataSrcEnd; ++_pDataSrc, ++_pDataDst)
                    {
                        *_pDataDst = std::move(*_pDataSrc);
                    }
                }
                else
                {
                    auto _pDataDstLast = _pDataDst + _uCount;
                    auto _pDataSrcLast = _pDataSrc + _uCount;

                    for (; _pDataSrc != _pDataSrcLast;)
                    {
                        --_pDataSrcLast;
                        --_pDataDstLast;

                        *_pDataDstLast = std::move(*_pDataSrcLast);
                    }
                }
            }
        };

        template<typename T>
        struct DynamicArrayConstructor<T, false>
        {
            __forceinline static void __fastcall Init(_In_ T* _pData, _In_ uint_t _uCount = 1)
            {
                memset(_pData, 0, _uCount * sizeof(T));
            }

            __forceinline static void __fastcall Init(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) const T* _pDataSrc, _In_ uint_t _uCount)
            {
                memcpy(_pDataDst, _pDataSrc, _uCount * sizeof(T));
            }

            __forceinline static void __fastcall Uninit(_In_ T* _pData, _In_ uint_t _uCount = 1)
            {
            }
            
            __forceinline static void __fastcall InitAndMove(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) T* _pDataSrc, _In_ uint_t _uCount)
            {
                memcpy(_pDataDst, _pDataSrc, _uCount * sizeof(T));
            }
            
            __forceinline static void __fastcall Copy(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) const T* _pDataSrc, _In_ uint_t _uCount)
            {
                memcpy(_pDataDst, _pDataSrc, _uCount * sizeof(T));
            }

            __forceinline static void __fastcall CopyAndMove(_Out_writes_(_uCount) T* _pDataDst, _In_reads_(_uCount) const T* _pDataSrc, _In_ uint_t _uCount)
            {
                if (_uCount == 0)
                    return;

                memmove(_pDataDst, _pDataSrc, _uCount * sizeof(T));
            }
        };


        template<typename T, bool _bUsingWriteOnCopy = false, bool _bUsingClassConstructor = true>
        class DynamicArray
        {
        private:
            using SharedPoint = typename DynamicArraySharedPoint<T, _bUsingWriteOnCopy>::SharedPoint;
            using Constructor = DynamicArrayConstructor<T, _bUsingClassConstructor>;

            T* pData;
        public:
            DynamicArray()
                : pData(nullptr)
            {
            }

            DynamicArray(const DynamicArray& _Src)
                : pData(nullptr)
            {
                auto _hr = SetArray(_Src);

                if (FAILED(_hr))
                    throw Exception(_S("DynamicArray构造失败！"), _hr);
            }

            DynamicArray(DynamicArray&& _Src)
                : pData(nullptr)
            {
                auto _hr = SetArray(std::move(_Src));
                if (FAILED(_hr))
                    throw Exception(_S("DynamicArray构造失败！"), _hr);
            }

            DynamicArray(std::initializer_list<T> _List)
                : pData(nullptr)
            {
                auto _hr = SetArray(_List.begin(), _List.size());

                if (FAILED(_hr))
                    throw Exception(_S("DynamicArray构造失败！"), _hr);
            }

            ~DynamicArray()
            {
                if (auto _pSharedData = GetSharedData())
                    _pSharedData->Release();
            }


            uint_t __fastcall GetSize() const
            {
                auto _pSharedData = GetSharedData();
                return _pSharedData ? _pSharedData->uSize : 0;
            }
            
            HRESULT __fastcall Reserve(uint_t _uCapacity)
            {
                auto _pLockData = LockSharedData(_uCapacity);
                if (!_pLockData)
                    return E_OUTOFMEMORY;

                _pLockData->Unlock();
                return S_OK;
            }

            uint_t __fastcall GetCapacity()
            {
                auto _pSharedData = GetSharedData();
                return _pSharedData ? _pSharedData->uCapacity : 0;
            }

            _Ret_maybenull_ T* __fastcall LockBufferAndSetSize(uint_t _uNewSize)
            {
                auto _pLockData = LockSharedData(_uNewSize);
                if (!_pLockData)
                    return nullptr;
                
                auto _uSize = _pLockData->uSize;
                auto _pBuffer = _pLockData->GetData();


                if (_uSize < _uNewSize)
                {
                    Constructor::Init(_pBuffer + _uSize, _uNewSize - _uSize);
                }
                else
                {
                    if (_pLockData->GetLockedCount() > 1)
                    {
                        _pLockData->Unlock();
                        throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                        return nullptr;
                    }

                    Constructor::Uninit(_pBuffer + _uNewSize, _uSize - _uNewSize);
                }

                _pLockData->uSize = _uNewSize;
                return _pBuffer;
            }

            void __fastcall UnlockBuffer()
            {
                auto _pSharedData = GetSharedData();
                if (_pSharedData == nullptr || _pSharedData->IsLocked() == false)
                {
                    throw Exception(szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked);
                    return;
                }

                _pSharedData->Unlock();
            }

            void __fastcall Clear()
            {
                auto _pSharedData = GetSharedData();
                if (!_pSharedData)
                    return;

                if (_pSharedData->IsShared())
                {
                    pData = nullptr;
                    _pSharedData->Release();
                }
                else if (_pSharedData->uSize != 0)
                {
                    if (_pSharedData->IsLocked())
                    {
                        throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                        return;
                    }
                    _pSharedData->uSize = 0;
                }
            }

            HRESULT __fastcall Resize(uint_t _uNewSize)
            {
                if (_uNewSize == 0)
                {
                    Clear();
                    return S_OK;
                }

                auto _pBuffer = LockBufferAndSetSize(_uNewSize);
                if (!_pBuffer)
                    return E_OUTOFMEMORY;

                UnlockBuffer();
                return S_OK;
            }

            _Ret_maybenull_ SharedPoint __fastcall GetItemPtr(uint_t _uIndex)
            {
                if (GetSize() <= _uIndex)
                    return nullptr;

                return pData + _uIndex;
            }

            _Ret_maybenull_ const T* __fastcall GetItemPtr(uint_t _uIndex) const
            {
                if (GetSize() <= _uIndex)
                    return nullptr;

                return pData + _uIndex;
            }

            HRESULT __fastcall SetItem(uint_t _uIndex, const T& _NewItem)
            {
                auto _pSharedData = GetSharedData();
                if (_pSharedData == nullptr || _pSharedData->uSize <= _uIndex)
                    return E_BOUNDS;

                _pSharedData = LockSharedData(0);
                if (!_pSharedData)
                    return E_OUTOFMEMORY;

                Constructor::Copy(_pSharedData->GetData() + _uIndex, &_NewItem, 1);
                _pSharedData->Unlock();
                return S_OK;
            }

            HRESULT __fastcall SetArray(_In_reads_(_uCount) const T* _pSrc, _In_ uint_t _uCount)
            {
                if (_uCount == 0)
                {
                    Clear();
                    return S_OK;
                }

                if (!_pSrc)
                    return E_POINTER;

                auto _pSharedData = LockSharedData(_uCount);
                if (!_pSharedData)
                    return E_OUTOFMEMORY;

                auto _pData = _pSharedData->GetData();
                auto _uSize = _pSharedData->uSize;

                if (_uCount <= _uSize)
                {
                    Constructor::Copy(_pData, _pSrc, _uCount);
                    Constructor::Uninit(_pData + _uCount, _uSize - _uCount);
                }
                else
                {
                    Constructor::Copy(_pData, _pSrc, _uSize);
                    Constructor::Init(_pData + _uSize, _pSrc, _uCount - _uSize);
                }
                
                _pSharedData->uSize = _uCount;
                _pSharedData->Unlock();
                return S_OK;
            }

            HRESULT __fastcall SetArray(const DynamicArray& _Src)
            {
                auto& __Src = const_cast<DynamicArray&>(_Src);
                auto _pSharedData = __Src.GetSharedData();
                if (!_pSharedData)
                {
                    Clear();
                    return S_OK;
                }

                if (_bUsingWriteOnCopy && _pSharedData->IsLocked() == false)
                {
                    Attach(_pSharedData);
                    return S_OK;
                }
                else
                {
                    return SetArray(_pSharedData->GetData(), _pSharedData->uSize);
                }
            }

            HRESULT __fastcall SetArray(DynamicArray&& _Src)
            {
                auto _pSharedData = _Src.Detach();
                if (!_pSharedData)
                {
                    Clear();
                    return S_OK;
                }

                Attach(_pSharedData);
                _pSharedData->Release();
                return S_OK;
            }

            HRESULT __fastcall Add(const T& _NewItem)
            {
                auto _uIndex = GetSize();
                auto _uNewSize = _uIndex + 1;

                auto _pSharedData = LockSharedData(_uNewSize);
                if (!_pSharedData)
                    return E_OUTOFMEMORY;

                Constructor::Init(_pSharedData->GetData() + _uIndex, &_NewItem, 1);
                _pSharedData->uSize = _uNewSize;
                _pSharedData->Unlock();
                return S_OK;
            }

            T* __fastcall AddAndGetPtr(uint_t* _puIndex = nullptr)
            {
                auto _uIndex = GetSize();
                auto _uNewSize = _uIndex + 1;

                auto _pSharedData = LockSharedData(_uNewSize);
                if (!_pSharedData)
                    return nullptr;
                if (_puIndex)
                    *_puIndex = _uIndex;
                auto _pItem = _pSharedData->GetData() + _uIndex;

                Constructor::Init(_pItem, 1);
                _pSharedData->uSize = _uNewSize;
                _pSharedData->Unlock();
                return _pItem;
            }

            HRESULT __fastcall Insert(uint_t _uIndex, const T& _NewItem)
            {
                const auto _uSize = GetSize();

                if (_uIndex > _uSize)
                    return E_BOUNDS;
                
                auto _uNewSize = _uSize + 1;
                auto _pSharedData = LockSharedData(_uNewSize);
                if (!_pSharedData)
                    return E_OUTOFMEMORY;

                auto _pData = _pSharedData->GetData();

                auto _pDst = _pData + _uIndex + 1;
                auto _pSrc = _pData + _uIndex;
                auto _uMoveCount = _uSize - _uIndex;

                if (_uSize == _uIndex)
                {
                    Constructor::Init(_pData + _uIndex, &_NewItem, 1);
                }
                else
                {
                    Constructor::Init(_pData + _uSize);
                    Constructor::CopyAndMove(_pDst, _pSrc, _uMoveCount);
                    Constructor::Copy(_pData + _uIndex, &_NewItem, 1);
                }
                _pSharedData->uSize = _uNewSize;
                _pSharedData->Unlock();

                return S_OK;
            }

            HRESULT __fastcall Remove(_In_ uint_t _uIndex, _In_ uint_t _uCount = 1)
            {
                if (_uCount == 0)
                    return S_OK;

                const auto _uSize = GetSize();
                if (_uSize <= _uIndex)
                    return S_OK;

                auto _uRemoveCount = _uSize - _uIndex;
                if (_uRemoveCount > _uCount)
                    _uRemoveCount = _uCount;

                if (_uRemoveCount == _uSize)
                {
                    Clear();
                    return S_OK;
                }

                auto _pSharedData = LockSharedData(0);
                if (!_pSharedData)
                    return E_OUTOFMEMORY;

                auto _pData = _pSharedData->GetData();

                Constructor::CopyAndMove(_pData + _uIndex, _pData + _uIndex + _uRemoveCount, _uSize - (_uIndex + _uRemoveCount));
                Constructor::Uninit(_pData + _uSize - (_uIndex + _uRemoveCount), _uRemoveCount);
                _pSharedData->uSize = _uSize - _uRemoveCount;
                _pSharedData->Unlock();

                return S_OK;
            }
            
            SharedPoint __fastcall GetData() noexcept
            {
                return pData;
            }

            const T* __fastcall GetData() const noexcept
            {
                return pData;
            }

            SharedPoint __fastcall begin() noexcept
            {
                return pData;
            }

            SharedPoint __fastcall end() noexcept
            {
                return pData + GetSize();
            }

            const T* __fastcall begin() const noexcept
            {
                return pData;
            }

            const T* __fastcall end() const noexcept
            {
                return pData + GetSize();
            }
            
            SharedPoint __fastcall _Unchecked_begin() noexcept
            {
                return pData;
            }

            SharedPoint __fastcall _Unchecked_end() noexcept
            {
                return pData + GetSize();
            }

            const T* __fastcall _Unchecked_begin() const noexcept
            {
                return pData;
            }

            const T* __fastcall _Unchecked_end() const noexcept
            {
                return pData + GetSize();
            }
            
            bool __fastcall operator==(const DynamicArray& _Array) const
            {
                auto _Size = GetSize();

                if (_Size != _Array.GetSize())
                    return false;

                for (uint_t i = 0; i != _Size;++i)
                {
                    if (pData[i] != _Array.pData[i])
                        return false;
                }

                return true;
            }

            struct SharedData
            {
                // 暂不使用
                uint32_t fMarks;
                // 如果 >= 0，那么表示这块内存的引用次数。
                // 如果 <  0，那么表示内存锁定次数。
                int32_t iRef;

                // 内存的申请长度
                uint_t uCapacity;
                // 元素的实际使用个数
                uint_t uSize;

                void __fastcall AddRef()
                {
                    if (iRef < 1)
                    {
                        throw Exception(_S("iRef 应该为 >= 1。"));
                        return;
                    }

                    Interlocked::Increment(&iRef);
                }

                void __fastcall Release()
                {
                    if (iRef > 0)
                    {
                        auto iNewRef = Interlocked::Decrement(&iRef);
                        if (iNewRef != 0)
                            return;
                    }
                    // 锁定时，虽然是负数，但是其实内存引用次数其实是 1。
                    // 所以直接释放内存即可。
   
                    Constructor::Uninit(GetData(), uSize);
                    HFree(this);
                }

                bool __fastcall IsShared()
                {
                    return iRef > 1;
                }

                uint_t __fastcall GetLockedCount()
                {
                    if (iRef >= 0)
                        return 0;

                    return iRef * -1;
                }

                bool __fastcall IsLocked()
                {
                    return iRef < 0;
                }

                /// <summary>
                /// 将 SharedData 锁定，注意不支持多线程安全。
                /// </summary>
                /// <returns></returns>
                void __fastcall Lock()
                {
                    if (iRef > 1)
                    {
                        throw Exception(_S("共享状态时无法锁定。"));
                        return;
                    }

                    if (iRef == 0)
                    {
                        throw Exception(_S("iRef 不应该为 0。"));
                        return;
                    }

                    if (iRef ==1)
                    {
                        iRef = -1;
                    }
                    else
                    {
                        --iRef;
                    }
                }

                /// <summary>
                /// 将 SharedData 解除锁定，注意不支持多线程安全。
                /// </summary>
                /// <returns></returns>
                void __fastcall Unlock()
                {
                    if (iRef > 0)
                    {
                        throw Exception(szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked);
                        return;
                    }

                    if (iRef == -1)
                    {
                        iRef = 1;
                    }
                    else
                    {
                        ++iRef;
                    }
                }

                T* __fastcall GetData()
                {
                    return reinterpret_cast<T*>(this + 1);
                }

                static _Ret_maybenull_ SharedData* __fastcall ReAlloc(_In_opt_ SharedData* _pOldSharedData,_In_  uint_t _uNewCapacity)
                {
                    if (!_pOldSharedData)
                    {
                        return Alloc(_uNewCapacity);
                    }

                    if (_pOldSharedData->uCapacity >= _uNewCapacity)
                        return _pOldSharedData;

                    if (_pOldSharedData->IsLocked())
                    {
                        throw Exception(_S("Buffer已经锁定，无法 ReAlloc。"));
                        return nullptr;
                    }

                    // 向上对齐到 16
                    ++_uNewCapacity;
                    _uNewCapacity = (_uNewCapacity + 15) & ~15;

                    if (_bUsingClassConstructor)
                    {
                        auto pNewSharedData = Alloc(_uNewCapacity);
                        if (!pNewSharedData)
                            return nullptr;

                        pNewSharedData->fMarks = _pOldSharedData->fMarks;
                        pNewSharedData->iRef = _pOldSharedData->iRef;
                        Constructor::InitAndMove(pNewSharedData->GetData(), _pOldSharedData->GetData(), _pOldSharedData->uSize);
                        pNewSharedData->uSize = _pOldSharedData->uSize;
                        _pOldSharedData->Release();
                        return pNewSharedData;
                    }
                    else
                    {
                        auto pNewSharedData = (SharedData*)HReAlloc(_pOldSharedData, sizeof(SharedData) + sizeof(T) * _uNewCapacity);
                        if (!pNewSharedData)
                            return nullptr;

                        pNewSharedData->uCapacity = _uNewCapacity;
                        return pNewSharedData;
                    }
                }

                static _Ret_maybenull_ SharedData* __fastcall Alloc(uint_t _uNewCapacity)
                {
                    // 向上对齐到 16
                    ++_uNewCapacity;
                    _uNewCapacity = (_uNewCapacity + 15) & ~15;

                    auto pNewSharedData = (SharedData*)HAlloc(sizeof(SharedData) + sizeof(T) * _uNewCapacity);
                    if (!pNewSharedData)
                        return nullptr;
                    pNewSharedData->fMarks = 0;
                    pNewSharedData->iRef = 1;
                    pNewSharedData->uSize = 0;
                    pNewSharedData->uCapacity = _uNewCapacity;
                    return pNewSharedData;
                }

                _Ret_maybenull_ SharedData* __fastcall Clone(_In_ uint_t _uNewCapacity = 0)
                {
                    const auto _uSize = uSize;
                    if (_uNewCapacity < _uSize)
                        _uNewCapacity = _uSize;

                    auto _pNewSharedData = Alloc(_uNewCapacity);
                    if (!_pNewSharedData)
                        return nullptr;

                    Constructor::Init(_pNewSharedData->GetData(), GetData(), _uSize);
                    _pNewSharedData->uSize = _uSize;
                    return _pNewSharedData;
                }
            };
        private:
            _Ret_maybenull_ SharedData* __fastcall GetSharedData() const
            {
                if (!pData)
                    return nullptr;

                return reinterpret_cast<SharedData*>(pData) - 1;
            }

            void __fastcall Attach(_In_opt_ SharedData* _pNewSharedData)
            {
                auto _pOldSharedData = GetSharedData();

                if (_pOldSharedData != _pNewSharedData)
                {
                    if (_pNewSharedData)
                    {
                        _pNewSharedData->AddRef();
                        pData = _pNewSharedData->GetData();
                    }
                    else
                    {
                        pData = nullptr;
                    }

                    if (_pOldSharedData)
                        _pOldSharedData->Release();
                }
            }

            _Ret_maybenull_ SharedData* __fastcall Detach()
            {
                auto _pOldSharedData = GetSharedData();
                if (_pOldSharedData && _pOldSharedData->IsLocked())
                {
                    throw Exception(_S("Buffer已经锁定。"));
                    return nullptr;
                }

                pData = nullptr;
                return _pOldSharedData;
            }

            /// <summary>
            /// 锁定SharedData，如果处于独占，那么将发生复制。
            /// </summary>
            /// <param name="_uNewCapacity">期望申请的最小容量。</param>
            /// <returns></returns>
            _Ret_maybenull_ SharedData* __fastcall LockSharedData(_In_ uint_t _uNewCapacity)
            {
                auto _pSharedData = GetSharedData();

                if (_pSharedData)
                {
                    if (_pSharedData->IsShared())
                    {
                        if (_pSharedData->uSize < _uNewCapacity)
                            _uNewCapacity = GetNewCapacity(_pSharedData->uSize, _uNewCapacity);

                        _pSharedData = _pSharedData->Clone(_uNewCapacity);
                        if (!_pSharedData)
                            return nullptr;

                        Attach(_pSharedData);
                        _pSharedData->Release();
                    }
                    else
                    {
                        if (_pSharedData->uCapacity < _uNewCapacity)
                        {
                            _uNewCapacity = GetNewCapacity(_pSharedData->uCapacity, _uNewCapacity);

                            _pSharedData = SharedData::ReAlloc(_pSharedData, _uNewCapacity);
                            if (!_pSharedData)
                                return nullptr;

                            pData = _pSharedData->GetData();
                        }
                    }
                }
                else
                {
                    _uNewCapacity += _uNewCapacity / 2;
                    _pSharedData = SharedData::Alloc(_uNewCapacity);
                    if (!_pSharedData)
                        return nullptr;
                    Attach(_pSharedData);
                    _pSharedData->Release();
                }

                _pSharedData->Lock();

                return _pSharedData;
            }

            /// <summary>
            /// 返回新的建议的 容量。为了提高效率，内部以1.5倍速度增长。
            /// </summary>
            /// <param name="_uCapacity">现有容量</param>
            /// <param name="_uCapacityNeed">实际请求的容量</param>
            /// <returns>返回需要开辟的容量</returns>
            __forceinline static uint_t __fastcall GetNewCapacity(uint_t _uCapacity, uint_t _uCapacityNeed)
            {
                if (_uCapacity >= _uCapacityNeed)
                    return _uCapacity;

                _uCapacity += _uCapacity / 2;

                if (_uCapacity >= _uCapacityNeed)
                    return _uCapacity;
                else
                    return _uCapacityNeed;
            }
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
