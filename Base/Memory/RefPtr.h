/*

RefPtr 是 类似于shared_ptr的智能指针。
他们不同的是 RefPtr 自生提供引用计数。

因此使用RefPtr的类必须拥有 AddRef 以及 Release

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
            class RefPtr
            {
            private:
                _Type* p;

            public:
                RefPtr()
                    : p(nullptr)
                {
                }

                RefPtr(_Type* _pOther)
                    : p(_pOther)
                {
                    if (p)
                        p->AddRef();
                }

                RefPtr(const RefPtr& _Other)
                    : p(_Other.Clone())
                {
                }

                RefPtr(RefPtr&& _Other) noexcept
                    : p(_Other.Detach())
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

                RefPtr& __YYAPI operator=(_In_opt_ _Type* _pOther)
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

                _Ret_maybenull_ __YYAPI operator _Type*() const
                {
                    return p;
                }

                _Ret_maybenull_ _Type* __YYAPI operator->() const
                {
                    return p;
                }
            };
        } // namespace Memory
    } // namespace Base

    using namespace YY::Base::Memory;

} // namespace YY

#pragma pack(pop)
