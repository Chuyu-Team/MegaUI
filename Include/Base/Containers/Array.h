#pragma once

/*
Array 是一组动态扩展数组（类似于vector），不同的是它支持2种模式。

1. Copy On Wirte模式 —— 拥有引用计数能力，复制时只增加引用

Array<int, AllocPolicy::COW> _Array;

此模式类内部提供了简易的线程同步机制，但是并非完全，线程安全。


√  多线程读取，多个线程读取同一个SharedArray对象。
√  多线程读取，多个线程读取各自的SharedArray对象，但是它们是同一个引用。

╳  多线程写入，多个线程写入同一个SharedArray对象。
√  多线程写入，多个线程写入各自的SharedArray对象，但是他们是同一个引用。


LockBuffer 与 UnlockBuffer 必须成对出现！

2. Small Object Optimization模式 —— 小实际转移到内置缓冲区，减少Heap申请开销

Array<int, AllocPolicy::SOO> _Array;

LockBuffer 与 UnlockBuffer 必须成对出现！

*/

#include <initializer_list>
#include <stdlib.h>
#include <limits>

#include <Base/YY.h>
#include <Base/Exception.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Containers/ConstructorPolicy.h>
#include <Base/ErrorCode.h>
#include <Base/Memory/Alloc.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            constexpr auto szReducedSizeIsNotSupportedBecauseBufferLocked = _S("Buffer处于锁定状态，无法减少 uSize。");
            constexpr auto szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked = _S("Buffer并未锁定，无法调用UnlockBuffer。");
    
            enum class AllocPolicy
            {
                // Copy On Wirte
                COW,
                // Small Object Optimization
                SOO,
            };

            template<typename _Type, AllocPolicy _eAllocPolicy, uint32_t _uInsideBufferSize>
            class AllocPolicyArray;

            // Copy On Wirte优化的数组策略
            template<typename _Type>
            class AllocPolicyArray<_Type, AllocPolicy::COW, 0>
            {
            protected:
                _Type* pData;

            public:
                typedef const _Type* _ReadPoint;
                typedef const _Type _ReadType;

                constexpr AllocPolicyArray()
                    : pData(SharedData::GetEmptySharedData()->GetData())
                {
                }

                ~AllocPolicyArray()
                {
                    auto _pInternalData = GetInternalData();
                    _pInternalData->Release();
                }

                bool __YYAPI IsEmpty() const
                {
                    auto _pInternalData = GetInternalData();
                    return _pInternalData->GetSize() == 0;
                }

                size_t __YYAPI GetSize() const
                {
                    auto _pInternalData = GetInternalData();
                    return _pInternalData->GetSize();
                }
        
                size_t __YYAPI GetCapacity()
                {
                    auto _pInternalData = GetInternalData();
                    return _pInternalData->GetCapacity();
                }
        
                void __YYAPI Clear()
                {
                    auto _pInternalData = GetInternalData();
                    if (_pInternalData->IsShared())
                    {
                        pData = SharedData::GetEmptySharedData()->GetData();
                        _pInternalData->Release();
                    }
                    else if (_pInternalData->GetSize() != 0)
                    {
                        if (_pInternalData->IsLocked())
                        {
                            throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                            return;
                        }

                        ConstructorPolicy<_Type>::Destructor(_pInternalData->GetData(), _pInternalData->GetSize());
                        _pInternalData->SetSize(0);
                    }
                }
        
                HRESULT __YYAPI SetArray(_In_reads_(_uCount) const _Type* _pSrc, _In_ size_t _uCount)
                {
                    if (_uCount && _pSrc == nullptr)
                    {
                        return E_POINTER;
                    }

                    auto _pInternalData = GetInternalData();
                    if (_pInternalData->IsShared())
                    {
                        // 共享模式中放弃原有内存，重新申请内存即可。
                        auto _pNewSharedData = SharedData::Alloc(_uCount);
                        if (!_pNewSharedData)
                            return E_OUTOFMEMORY;

                        pData = _pNewSharedData->GetData();
                        _pInternalData->Release();
                        _pInternalData = _pNewSharedData;
                    }
                    else if (_pInternalData->IsLocked())
                    {
                        if (_pInternalData->GetSize() > _uCount)
                        {
                            throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                            return E_INVALIDARG;
                        }

                        if (_pInternalData->GetCapacity() < _uCount)
                        {
                            // 内存处于锁定状态，无法 ReAlloc 扩大。
                            throw Exception();
                            return E_INVALIDARG;
                        }
                    }
                    else
                    {
                        if (_pInternalData->GetCapacity() < _uCount)
                        {
                            _pInternalData = SharedData::ReAlloc(_pInternalData, _uCount);
                            if (!_pInternalData)
                                return E_OUTOFMEMORY;
                            pData = _pInternalData->GetData();
                        }
                    }

                    _pInternalData->Lock();

                    auto _pData = _pInternalData->GetData();
                    if (_uCount <= _pInternalData->GetSize())
                    {
                        ConstructorPolicy<_Type>::Copy(_pData, _pSrc, _uCount);
                        ConstructorPolicy<_Type>::Destructor(_pData + _uCount, _pData + _pInternalData->GetSize());
                    }
                    else
                    {
                        ConstructorPolicy<_Type>::Copy(_pData, _pSrc, _pSrc + _pInternalData->GetSize());
                        ConstructorPolicy<_Type>::CopyConstructor(_pData + _pInternalData->GetSize(), _pSrc + _pInternalData->GetSize(), _pSrc + _uCount);
                    }

                    _pInternalData->SetSize(_uCount);
                    _pInternalData->Unlock();
                    return S_OK;
                }

                HRESULT __YYAPI SetArray(const AllocPolicyArray& _Src)
                {
                    if (GetData() == _Src.GetData())
                        return S_OK;

                    auto _pInternalData = _Src.GetInternalData();
                    if (_pInternalData->IsLocked() == false)
                    {
                        _pInternalData->AddRef();
                        Attach(_pInternalData);
                        return S_OK;
                    }
                    else
                    {
                        return SetArray(_pInternalData->GetData(), _pInternalData->GetSize());
                    }
                }

                HRESULT __YYAPI SetArray(AllocPolicyArray&& _Src)
                {
                    if (GetData() == _Src.GetData())
                        return S_OK;

                    Attach(_Src.Detach());
                    return S_OK;
                }

                _Ret_notnull_ const _Type* __YYAPI GetData() const noexcept
                {
                    return pData;
                }

                // WirteOnCopy机制内部的内存块
                struct SharedData
                {
                    // 暂不使用
                    uint_t fMarks;
                    // 如果 >= 0，那么表示这块内存的引用次数。
                    // 如果 <  0，那么表示内存锁定次数。
                    int_t iRef;
                    // 缓冲区上限
                    size_t uCapacity;
                    // 实际使用的元素个数
                    size_t uSize;

                    ~SharedData()
                    {
                        ConstructorPolicy<_Type>::Destructor(GetData(), uSize);
                    }

                    static _Ret_notnull_ SharedData* __YYAPI _GetEmptySharedData()
                    {
                        static const SharedData s_Empty = {0, (std::numeric_limits<decltype(SharedData::iRef)>::max)()};
                        return const_cast<SharedData*>(&s_Empty);
                    }

                    static _Ret_notnull_ SharedData* __YYAPI GetEmptySharedData()
                    {
                        // 为了将其他特化后的类型使用同一份数据，所以再次强制转换一次。
                        return reinterpret_cast<SharedData*>(AllocPolicyArray<int, AllocPolicy::COW, 0>::SharedData::_GetEmptySharedData());
                    }

                    bool __YYAPI IsReadOnly() const
                    {
                        return iRef == (std::numeric_limits<decltype(SharedData::iRef)>::max)();
                    }

                    void __YYAPI AddRef()
                    {
                        if (IsReadOnly())
                            return;

                        if (iRef < 1)
                        {
                            throw Exception(_S("iRef 应该为 >= 1。"));
                            return;
                        }

                        Sync::Increment(&iRef);
                    }

                    void __YYAPI Release()
                    {
                        if (IsReadOnly())
                            return;

                        if (iRef > 0)
                        {
                            auto iNewRef = Sync::Decrement(&iRef);
                            if (iNewRef != 0)
                                return;
                        }
                        // 锁定时，虽然是负数，但是其实内存引用次数其实是 1。
                        // 所以直接释放内存即可。
                        Free(this);
                    }

                    bool __YYAPI IsShared()
                    {
                        return iRef > 1;
                    }

                    size_t __YYAPI GetLockedCount()
                    {
                        if (iRef >= 0)
                            return 0;

                        return size_t(-iRef);
                    }

                    bool __YYAPI IsLocked()
                    {
                        return iRef < 0;
                    }

                    /// <summary>
                    /// 将 SharedData 锁定，注意不支持多线程安全。
                    /// </summary>
                    /// <returns></returns>
                    void __YYAPI Lock()
                    {
                        // ReadOnly时不需要锁定，因为它的长度总是 0。
                        if (IsReadOnly())
                            return;

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

                        if (iRef == 1)
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
                    void __YYAPI Unlock()
                    {
                        // ReadOnly时不需要锁定，因为它的长度总是 0。
                        if (IsReadOnly())
                            return;

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

                    constexpr _Ret_notnull_ _Type* __YYAPI GetData()
                    {
                        return reinterpret_cast<_Type*>(this + 1);
                    }

                    size_t __YYAPI GetSize()
                    {
                        return uSize;
                    }

                    void __YYAPI SetSize(size_t _uSize)
                    {
                        if (_uSize == uSize)
                            return;

                        uSize = _uSize;
                    }
            
                    size_t __YYAPI GetCapacity()
                    {
                        return uCapacity;
                    }

                    _Ret_notnull_ _Type* __YYAPI GetLast()
                    {
                        return GetData() + uSize;
                    }

                    static _Ret_maybenull_ SharedData* __YYAPI ReAlloc(_In_opt_ SharedData* _pOldSharedData, _In_ size_t _uNewCapacity)
                    {
                        if (_pOldSharedData == nullptr || _pOldSharedData->IsReadOnly())
                        {
                            return Alloc(_uNewCapacity);
                        }

                        if (_pOldSharedData->GetCapacity() >= _uNewCapacity)
                            return _pOldSharedData;

                        if (_pOldSharedData->IsLocked())
                        {
                            throw Exception(_S("Buffer已经锁定，无法 ReAlloc。"));
                            return nullptr;
                        }

                        // 向上对齐到 16
                        _uNewCapacity = (_uNewCapacity + 15) & ~15;

                        auto _uSize = _pOldSharedData->GetSize();
                        SharedData* _pNewSharedData;

                        if constexpr (std::is_trivially_copyable<_Type>::value)
                        {
                            _pNewSharedData = (SharedData*)YY::Base::Memory::ReAlloc(_pOldSharedData, sizeof(SharedData) + sizeof(_Type) * _uNewCapacity);
                            if (!_pNewSharedData)
                                return nullptr;
                        }
                        else
                        {
                            _pNewSharedData = (SharedData*)Alloc(sizeof(SharedData) + sizeof(_Type) * _uNewCapacity);
                            if (!_pNewSharedData)
                                return nullptr;

                            _pNewSharedData->fMarks = _pOldSharedData->fMarks;
                            _pNewSharedData->iRef = _pOldSharedData->iRef;

                            ConstructorPolicy<_Type>::MoveConstructor(_pNewSharedData->GetData(), _pOldSharedData->GetData(), _pOldSharedData->GetSize());
                            Free(_pOldSharedData);
                        }

                        _pNewSharedData->uCapacity = _uNewCapacity;
                        _pNewSharedData->uSize = _uSize;
                        return _pNewSharedData;
                    }

                    static _Ret_maybenull_ SharedData* __YYAPI Alloc(size_t _uNewCapacity)
                    {
                        // 向上对齐到 16
                        _uNewCapacity = (_uNewCapacity + 15) & ~15;

                        if (_uNewCapacity == 0)
                            return SharedData::GetEmptySharedData();

                        auto _pNewSharedData = (SharedData*)YY::Base::Memory::Alloc(sizeof(SharedData) + sizeof(_Type) * _uNewCapacity);
                        if (!_pNewSharedData)
                            return nullptr;
                        _pNewSharedData->fMarks = 0;
                        _pNewSharedData->iRef = 1;
                        _pNewSharedData->uCapacity = _uNewCapacity;
                        _pNewSharedData->uSize = 0;

                        return _pNewSharedData;
                    }

                    _Ret_maybenull_ SharedData* __YYAPI Clone(_In_ size_t _uNewCapacity = 0)
                    {
                        const auto _uSize = GetSize();
                        if (_uNewCapacity < _uSize)
                            _uNewCapacity = _uSize;

                        auto _pNewSharedData = Alloc(_uNewCapacity);
                        if (!_pNewSharedData)
                            return nullptr;

                        ConstructorPolicy<_Type>::CopyConstructor(_pNewSharedData->GetData(), GetData(), uSize);
                        _pNewSharedData->SetSize(_uSize);
                        return _pNewSharedData;
                    }

                    static void __YYAPI Free(_In_opt_ SharedData* _pSharedData)
                    {
                        if (_pSharedData == nullptr || _pSharedData->IsReadOnly())
                            return;

                        _pSharedData->~SharedData();
                        YY::Base::Memory::Free(_pSharedData);
                    }
                };

            protected:
                _Ret_notnull_ SharedData* __YYAPI GetInternalData() const noexcept
                {
                    return reinterpret_cast<SharedData*>(pData) - 1;
                }

                void __YYAPI Attach(_In_ SharedData* _pNewSharedData)
                {
                    auto _pOldSharedData = GetInternalData();

                    if (_pOldSharedData != _pNewSharedData)
                    {
                        if (_pOldSharedData->IsLocked())
                        {
                            throw Exception(_S("Buffer已经锁定。"));
                        }
                        pData = _pNewSharedData->GetData();
                    }
                    
                    _pOldSharedData->Release();
                }

                _Ret_notnull_ SharedData* __YYAPI Detach()
                {
                    auto _pOldSharedData = GetInternalData();
                    if (_pOldSharedData->IsLocked())
                    {
                        throw Exception(_S("Buffer已经锁定。"));
                    }

                    pData = SharedData::GetEmptySharedData()->GetData();
                    return _pOldSharedData;
                }

                /// <summary>
                /// 锁定SharedData，如果处于独占，那么将发生复制。
                /// </summary>
                /// <param name="_uNewCapacity">期望申请的最小容量。</param>
                /// <returns></returns>
                _Ret_maybenull_ SharedData* __YYAPI LockInternalData(_In_ size_t _uNewCapacity)
                {
                    auto _pInternalData = GetInternalData();
                    
                    if (_pInternalData->IsShared())
                    {
                        _pInternalData = _pInternalData->Clone(_uNewCapacity);
                        if (!_pInternalData)
                            return nullptr;

                        Attach(_pInternalData);
                    }
                    else
                    {
                        auto _uCapacity = _pInternalData->GetCapacity();

                        if (_uCapacity < _uNewCapacity)
                        {
                            _pInternalData = SharedData::ReAlloc(_pInternalData, _uNewCapacity);
                            if (!_pInternalData)
                                return nullptr;

                            pData = _pInternalData->GetData();
                        }
                    }

                    _pInternalData->Lock();
                    return _pInternalData;
                }

                _Ret_maybenull_ SharedData* __YYAPI LockInternalDataIncrement(_In_ size_t _uIncrementCount)
                {
                    auto _pInternalData = GetInternalData();
                    if (_pInternalData->IsShared())
                    {
                        auto _pNewInternalData = _pInternalData->Clone(_pInternalData->GetSize() + _uIncrementCount);
                        if (!_pNewInternalData)
                            return nullptr;
                        _pInternalData->Release();
                        _pInternalData = _pNewInternalData;

                        pData = _pInternalData->GetData();
                        _pInternalData->Lock();
                        return _pInternalData;
                    }
                    else
                    {
                        auto _uCapacity = _pInternalData->GetCapacity();
                        auto _uNewCapacity = GetNewCapacity(_uCapacity, _pInternalData->GetSize() + _uIncrementCount);

                        if (_uCapacity < _uNewCapacity)
                        {
                            _pInternalData = SharedData::ReAlloc(_pInternalData, _uNewCapacity);
                            if (!_pInternalData)
                                return nullptr;

                            pData = _pInternalData->GetData();
                        }

                        _pInternalData->Lock();
                        return _pInternalData;
                    }
                }

                /// <summary>
                /// 返回新的建议的 容量。为了提高效率，内部以1.5倍速度增长。
                /// </summary>
                /// <param name="_uCapacity">现有容量</param>
                /// <param name="_uCapacityNeed">实际请求的容量</param>
                /// <returns>返回需要开辟的容量</returns>
                inline static size_t __YYAPI GetNewCapacity(size_t _uCapacity, size_t _uCapacityNeed)
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
            
            template<typename _Type>
            struct AllocPolicyArraySOOLargeHeader
            {
                // 数组的起始指针
                _Type* pData;
                // 实际有效的元素个数
                size_t uSize;
                // 当前缓冲区的容量上限
                size_t uCapacity;
                // 表示内存锁定次数。
                size_t uLockCount;
            };

            template<typename _Type, unsigned _uInsideBufferSize>
            struct AllocPolicyArraySOOSmallHeader
            {
                using LargeHeader = AllocPolicyArraySOOLargeHeader<_Type>;

                // 为 0，则表示使用 SmallHeader，为 1则使用 LargeHeader
                size_t bSmallHeader : 1;
                size_t uLockCount : YY_bitsizeof(size_t) - 1;
                // StaticBuffer中实际使用的元素个数
                size_t uSize;

                byte_t InsideBuffer[sizeof(LargeHeader) + sizeof(_Type) * _uInsideBufferSize - sizeof(size_t) * 2];

                _Type* GetData() const
                {
                    return reinterpret_cast<_Type*>(const_cast<byte_t*>(InsideBuffer));
                }

                static constexpr size_t GetCapacity()
                {
                    return sizeof(InsideBuffer) / sizeof(_Type);
                }
            };

            template<typename _Type>
            struct AllocPolicyArraySOOSmallHeader<_Type, 0>
            {
                using LargeHeader = AllocPolicyArraySOOLargeHeader<_Type>;

                // 为 0，则表示使用 SmallHeader，为 1则使用 LargeHeader
                size_t bSmallHeader : 1;
                size_t uLockCount : YY_bitsizeof(size_t) - 1;
                // StaticBuffer中实际使用的元素个数
                size_t uSize;

                byte_t InsideBuffer[sizeof(LargeHeader) - sizeof(size_t) * 2];

                _Type* GetData() const
                {
                    return reinterpret_cast<_Type*>(const_cast<byte_t*>(InsideBuffer));
                }

                static constexpr size_t GetCapacity()
                {
                    return sizeof(InsideBuffer) / sizeof(_Type);
                }
            };

            // Small Object Optimization优化的数组策略
            template<typename _Type, uint32_t _uInsideBufferSize>
            class AllocPolicyArray<_Type, AllocPolicy::SOO, _uInsideBufferSize>
            {
            protected:
                union InternalData
                {
                    using LargeHeader = AllocPolicyArraySOOLargeHeader<_Type>;
                    using SmallHeader = AllocPolicyArraySOOSmallHeader<_Type, _uInsideBufferSize>;
                        
                    SmallHeader Small;
                    LargeHeader Large;
                    struct
                    {
                        // 为 0，则表示使用 SmallHeader，为 1则使用 LargeHeader
                        size_t bSmallHeader : 1;
                        size_t uReserve : YY_bitsizeof(size_t) - 1;
                        // 实际有效的元素个数
                        size_t uSize;
                    };

                    constexpr InternalData()
                        : Small{1}
                    {
                    }

                    ~InternalData()
                    {
                        if (bSmallHeader)
                        {
                            if(Small.uLockCount)
                                abort();

                            ConstructorPolicy<_Type>::Destructor(Small.GetData(), uSize);
                        }
                        else
                        {
                            if(Large.uLockCount)
                                abort();

                            ConstructorPolicy<_Type>::Destructor(Large.pData, uSize);
                            free(Large.pData);
                        }
                    }
            
                    size_t __YYAPI GetSize() const
                    {
                        return uSize;
                    }

                    void __YYAPI SetSize(size_t _uSize)
                    {
                        uSize = _uSize;
                    }

                    size_t __YYAPI GetCapacity() const
                    {
                        if (bSmallHeader)
                        {
                            return Small.GetCapacity();
                        }
                        else
                        {
                            return Large.uCapacity;
                        }
                    }
            
                    _Type* __YYAPI GetData() const
                    {
                        return bSmallHeader ? Small.GetData() : Large.pData;
                    }

                    _Type* __YYAPI GetLast() const
                    {
                        if (bSmallHeader)
                        {
                            return Small.GetData() + uSize;
                        }
                        else
                        {
                            return Large.pData + uSize;
                        }
                    }

                    size_t __YYAPI GetLockedCount() const
                    {
                        if (bSmallHeader)
                        {
                            return Small.uLockCount;
                        }
                        else
                        {
                            return Large.uLockCount;
                        }
                    }

                    bool __YYAPI IsLocked() const
                    {
                        return GetLockedCount() != 0;
                    }

                    void __YYAPI Lock()
                    {
                        if (bSmallHeader)
                        {
                            ++Small.uLockCount;
                        }
                        else
                        {
                            ++Large.uLockCount;
                        }
                    }

                    void __YYAPI Unlock()
                    {
                        if (bSmallHeader)
                        {
                            if (Small.uLockCount == 0)
                            {
                                throw Exception(szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked);
                                return;
                            }

                            --Small.uLockCount;
                        }
                        else
                        {
                            if (Large.uLockCount == 0)
                            {
                                throw Exception(szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked);
                                return;
                            }
                            --Large.uLockCount;
                        }
                    }

            
                    HRESULT __YYAPI Realloc(size_t _uNewCapacity)
                    {
                        if(bSmallHeader)
                        {
                            if(Small.GetCapacity() >= _uNewCapacity)
                                return S_OK;

                            if (Small.uLockCount)
                            {
                                // 已经锁定时无法扩展内存。
                                return E_INVALIDARG;
                            }

                            auto _pData = (_Type*)Alloc(_uNewCapacity * sizeof(_Type));
                            if(!_pData)
                                return E_OUTOFMEMORY;
                
                            // 从 Small切换到 Large
                            ConstructorPolicy<_Type>::MoveConstructor(_pData, Small.GetData(), uSize);
                            ConstructorPolicy<_Type>::Destructor(Small.GetData(), uSize);
                            Large.pData = _pData;
                            Large.uCapacity = _uNewCapacity;
                            Large.uLockCount =0;
                            return S_OK;
                        }
                        else
                        {
                            if(Large.uCapacity >= _uNewCapacity)
                                return S_OK;

                            if (Large.uLockCount)
                            {
                                // 已经锁定时无法扩展内存。
                                return E_INVALIDARG;
                            }

                            if constexpr (std::is_trivially_copyable<_Type>::value)
                            {
                                auto _pData = (_Type*)ReAlloc(Large.pData, _uNewCapacity * sizeof(_Type));
                                if (!_pData)
                                    return E_OUTOFMEMORY;

                                Large.pData = _pData;
                                Large.uCapacity = _uNewCapacity;
                            }
                            else
                            {
                                auto _pData = (_Type*)Alloc(_uNewCapacity * sizeof(_Type));
                                if(!_pData)
                                    return E_OUTOFMEMORY;

                                ConstructorPolicy<_Type>::MoveConstructor(_pData, Large.pData, uSize);
                                ConstructorPolicy<_Type>::Destructor(Large.pData, uSize);
                                free(Large.pData);

                                Large.pData = _pData;
                                Large.uCapacity = _uNewCapacity;
                            }
                            return S_OK;
                        }
                    }
                };
 
                InternalData Header;
        
            public:
                typedef _Type* _ReadPoint;
                typedef _Type _ReadType;

                constexpr AllocPolicyArray()
                {
                }

                ~AllocPolicyArray()
                {
            
                }

                // 内存的申请长度
                size_t __YYAPI GetCapacity() const
                {
                    return Header.GetCapacity();
                }

                // 元素的实际使用个数
                size_t __YYAPI GetSize() const
                {
                    return Header.GetSize();
                }

                _Type* GetData()
                {
                    return Header.bSmallHeader ? Header.Small.GetData() : Header.Large.pData;
                }
        
                const _Type* GetData() const
                {
                    return const_cast<AllocPolicyArray*>(this)->GetData();
                }
        
                void __YYAPI Clear()
                {
                    if(Header.uSize ==0)
                        return;

                    if (Header.bSmallHeader)
                    {
                        if(Header.Small.uLockCount !=0)
                        {
                            throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                            return;
                        }
                        ConstructorPolicy<_Type>::Destructor(Header.Small.GetData(), Header.uSize);
                    }
                    else
                    {
                        if(Header.Large.uLockCount !=0)
                        {
                            throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                            return;
                        }
                        ConstructorPolicy<_Type>::Destructor(Header.Large.pData, Header.uSize);
                    }

                    Header.uSize = 0;
                }

                HRESULT __YYAPI SetArray(_In_reads_(_uCount) const _Type* _pSrc, _In_ size_t _uCount)
                {
                    if (Header.IsLocked())
                    {
                        if (Header.uSize > _uCount)
                        {
                            // 锁定时，无法缩小 Size，无法扩大缓冲区
                            return E_INVALIDARG;
                        }
                    }

                    auto _hr = Header.Realloc(_uCount);
                    if(FAILED(_hr))
                        return _hr;

                    if (Header.uSize >= _uCount)
                    {
                        ConstructorPolicy<_Type>::Copy(Header.GetData(), _pSrc, _uCount);
                        ConstructorPolicy<_Type>::Destructor(Header.GetData() + _uCount, Header.GetLast());
                    }
                    else
                    {
                        ConstructorPolicy<_Type>::Copy(Header.GetData(), _pSrc, Header.uSize);
                        ConstructorPolicy<_Type>::CopyConstructor(Header.GetLast(), _pSrc + Header.uSize, _pSrc + _uCount);
                    }

                    Header.SetSize(_uCount);
                    return S_OK;
                }

                HRESULT __YYAPI SetArray(const AllocPolicyArray& _Other)
                {
                    if(&_Other == this)
                        return S_OK;

                    return SetArray(_Other.GetData(), _Other.GetSize());
                }

                HRESULT __YYAPI SetArray(AllocPolicyArray&& _Other)
                {
                    if(&_Other == this)
                        return S_OK;

                    if(_Other.Header.IsLocked() || Header.IsLocked())
                        return E_INVALIDARG;

                    if (_Other.Header.bSmallHeader)
                    {
                        return SetArray(_Other.GetData(), _Other.GetSize());
                    }
                    else
                    {
                        Header.~InternalData();
                        Header = _Other.Header;
                        new (&_Other.Header) InternalData();
                        return S_OK;
                    }
                }

            protected:

        
                /// <summary>
                /// 返回新的建议的 容量。为了提高效率，内部以1.5倍速度增长。
                /// </summary>
                /// <param name="_uCapacity">现有容量</param>
                /// <param name="_uCapacityNeed">实际请求的容量</param>
                /// <returns>返回需要开辟的容量</returns>
                inline static size_t __YYAPI GetNewCapacity(size_t _uCapacity, size_t _uCapacityNeed)
                {
                    if (_uCapacity >= _uCapacityNeed)
                        return _uCapacity;

                    _uCapacity += _uCapacity / 2;

                    if (_uCapacity >= _uCapacityNeed)
                        return _uCapacity;
                    else
                        return _uCapacityNeed;
                }
        
                _Ret_notnull_ InternalData* __YYAPI GetInternalData() const noexcept
                {
                    return const_cast<InternalData*>(&Header);
                }

                _Ret_maybenull_ InternalData* __YYAPI LockInternalData(_In_ size_t _uNewCapacity)
                {
                    if (_uNewCapacity <= GetCapacity())
                    {
                        Header.Lock();
                        return &Header;
                    }

                    // 向上对齐到 16
                    _uNewCapacity = (_uNewCapacity + 15) & ~15;

                    auto _hr = Header.Realloc(_uNewCapacity);
                    if(FAILED(_hr))
                        return nullptr;

                    Header.Lock();
                    return &Header;
                }
        
                _Ret_maybenull_ InternalData* __YYAPI LockInternalDataIncrement(_In_ size_t _uIncrementCount)
                {
                    auto _uNewCapacity = Header.uSize + _uIncrementCount;
                    auto _uCapacity = GetCapacity();

                    if (_uNewCapacity <= _uCapacity)
                    {
                        Header.Lock();
                        return &Header;
                    }

                    _uNewCapacity = GetNewCapacity(_uCapacity, _uNewCapacity);
                    auto _hr = Header.Realloc(_uNewCapacity);
                    if(FAILED(_hr))
                        return nullptr;
                    Header.Lock();
                    return &Header;
                }

             };

            template<typename _Type, AllocPolicy _eAllocPolicy = AllocPolicy::COW, uint32_t _uInsideBufferSize = 0>
            class Array : public AllocPolicyArray<_Type, _eAllocPolicy, _uInsideBufferSize>
            {
            public:
                using Type = _Type;
                using CurrentAllocPolicyArray = AllocPolicyArray<_Type, _eAllocPolicy, _uInsideBufferSize>;
                using _ReadPoint = typename CurrentAllocPolicyArray::_ReadPoint;
                using _ReadType = typename CurrentAllocPolicyArray::_ReadType;
                static constexpr AllocPolicy eAllocPolicy = _eAllocPolicy;
                
                constexpr static size_t uInvalidIndex = (std::numeric_limits<size_t>::max)();

                constexpr Array()
                {
                }

                Array(const Array& _Other)
                {
                    auto _hr = CurrentAllocPolicyArray::SetArray(_Other);
                    if(FAILED(_hr))
                        throw Exception(_S("DynamicArray构造失败！"), _hr);
                }

                Array(Array&& _Other) noexcept
                {
                    auto _hr = CurrentAllocPolicyArray::SetArray(std::move(_Other));
                    if(FAILED(_hr))
                        abort();
                }

                Array(std::initializer_list<_Type> _List)
                {
                    auto _hr = CurrentAllocPolicyArray::SetArray(_List.begin(), _List.size());

                    if (FAILED(_hr))
                        throw Exception(_S("SharedArray构造失败！"), _hr);
                }

                ~Array()
                {
                }
            
                HRESULT __YYAPI Reserve(size_t _uCapacity)
                {
                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalData(_uCapacity);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    _pInternalData->Unlock();
                    return S_OK;
                }

                HRESULT __YYAPI Resize(size_t _uNewSize)
                {
                    if (_uNewSize == 0)
                    {
                        CurrentAllocPolicyArray::Clear();
                        return S_OK;
                    }

                    auto _pBuffer = LockBufferAndSetSize(_uNewSize);
                    if (!_pBuffer)
                        return E_OUTOFMEMORY;

                    UnlockBuffer();
                    return S_OK;
                }
        
                _Ret_maybenull_ _Type* __YYAPI LockBufferAndSetSize(size_t _uNewSize)
                {
                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalData(_uNewSize);
                    if (!_pInternalData)
                        return nullptr;

                    auto _pBuffer = _pInternalData->GetData();

                    if (_pInternalData->GetSize() <= _uNewSize)
                    {
                        ConstructorPolicy<_Type>::Constructor(_pBuffer + _pInternalData->GetSize(), _pBuffer + _uNewSize);
                    }
                    else
                    {
                        if (_pInternalData->GetLockedCount() > 1)
                        {
                            _pInternalData->Unlock();
                            throw Exception(szReducedSizeIsNotSupportedBecauseBufferLocked);
                            return nullptr;
                        }

                        ConstructorPolicy<_Type>::Destructor(_pBuffer + _uNewSize, _pBuffer + _pInternalData->GetSize());
                    }

                    _pInternalData->SetSize(_uNewSize);
                    return _pBuffer;
                }

                void __YYAPI UnlockBuffer()
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    if (_pInternalData->IsLocked() == false)
                    {
                        throw Exception(szUnlockBufferFunctionCouldNotBeCalledBecauseBufferNotLocked);
                        return;
                    }

                    _pInternalData->Unlock();
                }

                _Ret_maybenull_ _ReadPoint __YYAPI GetItemPtr(size_t _uIndex)
                {
                    if (CurrentAllocPolicyArray::GetSize() <= _uIndex)
                        return nullptr;

                    return CurrentAllocPolicyArray::GetData() + _uIndex;
                }

                _Ret_maybenull_ const _Type* __YYAPI GetItemPtr(size_t _uIndex) const
                {
                    if (CurrentAllocPolicyArray::GetSize() <= _uIndex)
                        return nullptr;

                    return CurrentAllocPolicyArray::GetData() + _uIndex;
                }

                HRESULT __YYAPI SetItem(size_t _uIndex, const _Type& _NewItem)
                {
                    if (CurrentAllocPolicyArray::GetSize() <= _uIndex)
                        return E_BOUNDS;

                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalDataIncrement(0);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    _pInternalData->GetData()[_uIndex] = _NewItem;
                    _pInternalData->Unlock();
                    return S_OK;
                }

                size_t __YYAPI GetItemIndex(const _Type* _pItem) const
                {
                    size_t _uIndex = _pItem - CurrentAllocPolicyArray::GetData();
                    if (_uIndex < CurrentAllocPolicyArray::GetSize())
                        return _uIndex;

                    return uInvalidIndex;
                }

                size_t __YYAPI FindItemIndex(const _Type& _Item) const
                {
                    for (const auto& _Current : *this)
                    {
                        if (_Current == _Item)
                            return &_Current - CurrentAllocPolicyArray::GetData();
                    }

                    return uInvalidIndex;
                }

                HRESULT __YYAPI Add(const _Type& _NewItem)
                {
                    auto _pData = EmplacePtr(_NewItem);
                    return _pData ? S_OK : E_OUTOFMEMORY;
                }

                HRESULT __YYAPI Add(_In_ const _Type* _pSrc, _In_ uint_t _uCount)
                {
                    if (_uCount == 0)
                        return S_OK;

                    if (!_pSrc)
                        return E_INVALIDARG;

                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalDataIncrement(_uCount);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    ConstructorPolicy<_Type>::CopyConstructor(_pInternalData->GetLast(), _pSrc, _uCount);
                    _pInternalData->SetSize(_pInternalData->GetSize() + _uCount);
                    _pInternalData->Unlock();
                    return S_OK;
                }

                template<typename... Args>
                _Type* __YYAPI EmplacePtr(Args... args)
                {
                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalDataIncrement(1);
                    if (!_pInternalData)
                        return nullptr;

                    auto _pItem = _pInternalData->GetLast();
                    new (_pItem) _Type(args...);
                    _pInternalData->SetSize(_pInternalData->GetSize() + 1);
                    _pInternalData->Unlock();
                    return _pItem;
                }

                HRESULT __YYAPI Insert(uint_t _uIndex, const _Type& _NewItem)
                {
                    const auto _uSize = CurrentAllocPolicyArray::GetSize();

                    if (_uIndex > _uSize)
                        return E_BOUNDS;
            
                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalDataIncrement(1);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    auto _pData = _pInternalData->GetData();
                    auto _pLast = _pInternalData->GetLast() ;

                    auto _pInsert = _pData + _uIndex;

                    if (_pInsert == _pLast)
                    {
                        new (_pLast) _Type(_NewItem);
                    }
                    else
                    {
                        new (_pLast) _Type(std::move(_pLast[-1]));
                        ConstructorPolicy<_Type>::Move(_pInsert + 1, _pInsert, _pLast - 1);
                        _pData[_uIndex] = _NewItem;
                    }
                    _pInternalData->SetSize(_pInternalData->GetSize() + 1);
                    _pInternalData->Unlock();

                    return S_OK;
                }

                HRESULT __YYAPI Remove(_In_ uint_t _uIndex, _In_ uint_t _uCount = 1)
                {
                    if (_uCount == 0)
                        return S_OK;

                    const auto _uSize = CurrentAllocPolicyArray::GetSize();
                    if (_uSize <= _uIndex)
                        return S_OK;

                    auto _uRemoveCount = _uSize - _uIndex;
                    if (_uRemoveCount > _uCount)
                        _uRemoveCount = _uCount;

                    if (_uRemoveCount == _uSize)
                    {
                        CurrentAllocPolicyArray::Clear();
                        return S_OK;
                    }

                    auto _pInternalData = CurrentAllocPolicyArray::LockInternalDataIncrement(0);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    auto _pData = _pInternalData->GetData();
                    auto _pLast = _pInternalData->GetLast();

                    ConstructorPolicy<_Type>::Move(_pData + _uIndex, _pData + _uIndex + _uRemoveCount, _pLast);
                    ConstructorPolicy<_Type>::Destructor(_pLast - _uRemoveCount, _pLast);
                    _pInternalData->SetSize(_uSize - _uRemoveCount);
                    _pInternalData->Unlock();

                    return S_OK;
                }

                HRESULT __YYAPI Sort(int(* _pFuncCompare)(const _Type* _pLeft, const _Type* _pRigth))
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    if (_pInternalData->GetSize() <= 1)
                        return S_FALSE;

                    _pInternalData = CurrentAllocPolicyArray::LockInternalData(0);
                    if (!_pInternalData)
                        return E_OUTOFMEMORY;

                    qsort(_pInternalData->GetData(), _pInternalData->GetSize(), sizeof(_Type), (int (*)(const void*, const void*))_pFuncCompare);

                    _pInternalData->Unlock();
                    return S_OK;
                }

                _ReadType& __YYAPI operator[](_In_ uint_t _uIndex)
                {
                    if (_uIndex >= CurrentAllocPolicyArray::GetSize())
                        throw Exception();

                    return CurrentAllocPolicyArray::GetData()[_uIndex];
                }

                const _ReadType& __YYAPI operator[](_In_ uint_t _uIndex) const
                {
                    if (_uIndex >= CurrentAllocPolicyArray::GetSize())
                        throw Exception();

                    return CurrentAllocPolicyArray::GetData()[_uIndex];
                }

                _ReadPoint __YYAPI begin() noexcept
                {
                    return CurrentAllocPolicyArray::GetData();
                }

                _ReadPoint __YYAPI end() noexcept
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    return _pInternalData->GetLast();
                }

                const _Type* __YYAPI begin() const noexcept
                {
                    return CurrentAllocPolicyArray::GetData();
                }

                const _Type* __YYAPI end() const noexcept
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    return _pInternalData->GetLast();
                }
            
                _ReadPoint __YYAPI _Unchecked_begin() noexcept
                {
                    return CurrentAllocPolicyArray::GetData();
                }

                _ReadPoint __YYAPI _Unchecked_end() noexcept
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    return _pInternalData->GetLast();
                }

                const _Type* __YYAPI _Unchecked_begin() const noexcept
                {
                    return CurrentAllocPolicyArray::GetData();
                }

                const _Type* __YYAPI _Unchecked_end() const noexcept
                {
                    auto _pInternalData = CurrentAllocPolicyArray::GetInternalData();
                    return _pInternalData->GetLast();
                }
            
                bool __YYAPI operator==(const Array& _Array) const
                {
                    auto _Size = CurrentAllocPolicyArray::GetSize();

                    if (_Size != _Array.GetSize())
                        return false;

                    for (size_t i = 0; i != _Size; ++i)
                    {
                        if (CurrentAllocPolicyArray::GetData()[i] != _Array.GetData()[i])
                            return false;
                    }

                    return true;
                }

                Array& __YYAPI operator=(const Array& _Array)
                {
                    auto _hr = CurrentAllocPolicyArray::SetArray(_Array);
                    if (FAILED(_hr))
                        throw Exception();

                    return *this;
                }

                Array& __YYAPI operator=(Array&& _Array) noexcept
                {
                    auto _hr = CurrentAllocPolicyArray::SetArray(std::move(_Array));
                    if (FAILED(_hr))
                        abort();

                    return *this;
                }
            };

        } // namespace Containers
    } // namespace Base
    
    using namespace YY::Base::Containers;
} // namespace YY

#pragma pack(pop)
