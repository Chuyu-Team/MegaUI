/*

# RefPtr
RefPtr 是 类似于shared_ptr的智能指针。他们不同的是 RefPtr 自生提供引用计数。
因此使用RefPtr的类必须拥有 AddRef 以及 Release

# RefValue
提供 AddRef 以及 Release的基础能力包装，如果不满意可自行定义，实现 AddRef 以及 Release即可。

*/


#pragma once
#include <utility>

#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/Alloc.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Memory
        {
            class RefValue
            {
            private:
                volatile uint32_t uRef;
                volatile uint32_t uWeakRef;

            public:
                constexpr RefValue()
                    : uRef(1u)
                    , uWeakRef(1u)
                {
                }

                virtual ~RefValue() noexcept
                {
                }

                uint32_t __YYAPI AddRef() noexcept
                {
                    return Sync::Increment(&uRef);
                }

                uint32_t __YYAPI Release() noexcept
                {
                    auto _uNewRef = Sync::Decrement(&uRef);
                    if (_uNewRef == 0)
                    {
                        this->~RefValue();

                        ReleaseWeak();
                    }

                    return _uNewRef;
                }

                bool __YYAPI IsShared() const noexcept
                {
                    return uRef > 1;
                }

                uint32_t __YYAPI AddWeakRef() noexcept
                {
                    return Sync::Increment(&uWeakRef);
                }

                uint32_t __YYAPI ReleaseWeak() noexcept
                {
                    auto _uNewWeakRef = Sync::Decrement(&uWeakRef);

                    if (_uNewWeakRef == 0)
                    {
                        Free(this);
                    }

                    return _uNewWeakRef;
                }

                bool __YYAPI TryAddRef() noexcept
                {
                    auto _uCurrentRef = uRef;
                    for (; _uCurrentRef;)
                    {
                        const auto _uLast = Sync::CompareExchange(&uRef, _uCurrentRef + 1, _uCurrentRef);
                        if (_uLast == _uCurrentRef)
                            return true;

                        _uCurrentRef = _uLast;
                    }

                    return false;
                }
            };

            template<typename _Type>
            class RefPtr
            {
            private:
                _Type* p;

            public:
                RefPtr() noexcept
                    : p(nullptr)
                {
                }

                RefPtr(_Type* _pOther) noexcept
                    : p(_pOther)
                {
                    if (p)
                        p->AddRef();
                }

                RefPtr(const RefPtr& _Other) noexcept
                    : p(_Other.Clone())
                {
                }

                template<typename _TypeSource>
                RefPtr(const RefPtr<_TypeSource>& _Other) noexcept
                    : p(static_cast<_Type*>(_Other.Clone()))
                {
                }

                RefPtr(RefPtr&& _Other) noexcept
                    : p(_Other.Detach())
                {
                }

                template<typename _TypeSource>
                RefPtr(RefPtr<_TypeSource>&& _Other) noexcept
                    : p(static_cast<_Type*>(_Other.Detach()))
                {
                }

                ~RefPtr()
                {
                    if (p)
                        p->Release();
                }

                _Type* __YYAPI Get() const
                {
                    return p;
                }

                inline void __YYAPI Attach(_In_opt_ _Type* _pOther) noexcept
                {
                    if (p)
                        p->Release();
                    p = _pOther;
                }

                inline _Ret_maybenull_ _Type* __YYAPI Detach() noexcept
                {
                    auto _p = p;
                    p = nullptr;
                    return _p;
                }

                inline _Ret_maybenull_ _Type* __YYAPI Clone() const noexcept
                {
                    if (p)
                        p->AddRef();
                    return p;
                }

                inline _Ret_notnull_ _Type** __YYAPI ReleaseAndGetAddressOf() noexcept
                {
                    Attach(nullptr);
                    return &p;
                }

                inline void Reset(_In_opt_ _Type* _pOther = nullptr) noexcept
                {
                    Attach(_pOther);
                }

                RefPtr& __YYAPI operator=(std::nullptr_t) noexcept
                {
                    Attach(nullptr);
                    return *this;
                }

                RefPtr& __YYAPI operator=(_In_opt_ _Type* _pOther) noexcept
                {
                    if (_pOther)
                        _pOther->AddRef();

                    Attach(_pOther);

                    return *this;
                }

                RefPtr& __YYAPI operator=(const RefPtr& _Other) noexcept
                {
                    Attach(_Other.Clone());
                    return *this;
                }

                RefPtr& __YYAPI operator=(RefPtr&& _Other) noexcept
                {
                    Attach(_Other.Detach());
                    return *this;
                }

                _Ret_maybenull_ __YYAPI operator _Type* () const noexcept
                {
                    return p;
                }

                _Ret_maybenull_ _Type* __YYAPI operator->() const noexcept
                {
                    return p;
                }

                template<typename... Args>
                static RefPtr __YYAPI Create(Args&&... _args) noexcept
                {
                    return FromPtr(New<_Type>(std::forward<Args>(_args)...));
                }

                /// <summary>
                /// 从裸指针构建 RefPtr，注意，这里不增加引用计数。
                /// </summary>
                /// <param name="_pOther"></param>
                /// <returns></returns>
                static RefPtr __YYAPI FromPtr(_In_opt_ _Type* _pOther) noexcept
                {
                    RefPtr _p;
                    _p.p = _pOther;
                    return _p;
                }
            };
        }
    }
} // namespace YY::Base::Memory

namespace YY
{
    using namespace YY::Base::Memory;
}

#pragma pack(pop)
