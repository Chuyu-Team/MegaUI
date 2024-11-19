#pragma once

/*

RefPtr 是 类似于shared_ptr的智能指针。
他们不同的是 RefPtr 自生提供引用计数。

因此使用RefPtr的类必须拥有 AddRef 以及 Release

*/

/*
与 std::unique_ptr 作用相同。
*/

#pragma once
#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Memory
        {
            template<typename _Type>
            class UniquePtr
            {
            private:
                _Type* p;

            public:
                constexpr UniquePtr()
                    : p(nullptr)
                {
                }

                // 此构造不安全，因此刻意explicit
                explicit UniquePtr(_Type* _pOther) noexcept
                    : p(_pOther)
                {
                }

                UniquePtr(UniquePtr&& _pOther) noexcept
                    : p(_pOther.Detach())
                {
                }

                ~UniquePtr()
                {
                    if (p)
                        Delete(p);
                }

                _Ret_maybenull_ _Type* __YYAPI Get() const
                {
                    return p;
                }

                inline void __YYAPI Attach(_In_opt_ _Type* _pOther)
                {
                    if (p)
                        Delete(p);
                    p = _pOther;
                }

                inline _Ret_maybenull_ _Type* __YYAPI Detach()
                {
                    auto _p = p;
                    p = nullptr;
                    return _p;
                }

                inline _Ret_notnull_ _Type** __YYAPI ReleaseAndGetAddressOf() noexcept
                {
                    Attach(nullptr);
                    return &p;
                }

                _Ret_maybenull_ __YYAPI operator _Type*() const
                {
                    return p;
                }

                _Ret_maybenull_ _Type* __YYAPI operator->() const
                {
                    return p;
                }

                UniquePtr& __YYAPI operator=(_In_opt_ const UniquePtr& _pOther) = delete;

                UniquePtr& __YYAPI operator=(std::nullptr_t)
                {
                    Attach(nullptr);
                    return *this;
                }

                UniquePtr& __YYAPI operator=(_In_opt_ UniquePtr&& _Other) noexcept
                {
                    if (p != _Other.p)
                    {
                        Attach(_Other.Detach());
                    }

                    return *this;
                }
            };

            template<typename _Type>
            class UniquePtr<_Type[]>
            {
            private:
                _Type* p;

            public:
                UniquePtr()
                    : p(nullptr)
                {
                }

                // 此构造不安全，因此刻意explicit
                explicit UniquePtr(_Type* _pOther) noexcept
                    : p(_pOther)
                {
                }

                UniquePtr(UniquePtr&& _pOther) noexcept
                    : p(_pOther.Detach())
                {
                }

                ~UniquePtr()
                {
                    if (p)
                        delete[] p;
                }

                _Type* __YYAPI Get() const
                {
                    return p;
                }

                inline void __YYAPI Attach(_In_opt_ _Type* _pOther)
                {
                    if (p)
                        delete p;
                    p = _pOther;
                }

                inline _Ret_maybenull_ _Type* __YYAPI Detach()
                {
                    auto _p = p;
                    p = nullptr;
                    return _p;
                }

                inline _Ret_notnull_ _Type** __YYAPI ReleaseAndGetAddressOf() noexcept
                {
                    Attach(nullptr);
                    return &p;
                }

                __YYAPI operator _Type*() const
                {
                    return p;
                }

                _Type* __YYAPI operator->() const
                {
                    return p;
                }

                UniquePtr& __YYAPI operator=(_In_opt_ const UniquePtr& _pOther) = delete;

                UniquePtr& __YYAPI operator=(std::nullptr_t)
                {
                    Attach(nullptr);
                    return *this;
                }

                UniquePtr& __YYAPI operator=(_In_opt_ UniquePtr&& _Other) noexcept
                {
                    if (p != _Other.p)
                    {
                        Attach(_Other.Detach());
                    }

                    return *this;
                }
            };
        } // namespace Memory
    }     // namespace Base

    using namespace YY::Base::Memory;
} // namespace YY
