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
                    if (_pOther)
                        _pOther->AddRef();
                }

                RefPtr(const RefPtr& _Other)
                    : p(_Other.p)
                {
                    if (p)
                        p->AddRef();
                }

                RefPtr(RefPtr&& _Other) noexcept
                    : p(_Other.p)
                {
                    _Other.p = nullptr;
                }

                ~RefPtr()
                {
                    if (p)
                        p->Release();
                }

                _Type* __YYAPI Get()
                {
                    return p;
                }

                inline void __YYAPI Attach(_In_opt_ _Type* _pOther)
                {
                    if (p)
                        p->Release();
                    p = _pOther;
                }

                inline _Ret_maybenull_ _Type* __YYAPI Detach()
                {
                    auto _p = p;
                    p = nullptr;
                    return _p;
                }

                RefPtr& __YYAPI operator=(_In_opt_ _Type* _pOther)
                {
                    if (_pOther)
                        _pOther->AddRef();

                    Attach(_pOther);

                    return *this;
                }
                RefPtr& __YYAPI operator=(RefPtr _pOther)
                {
                    Attach(_pOther.Detach());

                    return *this;
                }

                RefPtr& __YYAPI operator=(RefPtr&& _Other) noexcept
                {
                    if (p != _Other.p)
                    {
                        Attach(_Other.Detach());
                    }

                    return *this;
                }

                _Type** __YYAPI operator&()
                {
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
            };
        } // namespace Memory
    } // namespace Base

    using namespace YY::Base::Memory;

} // namespace YY

#pragma pack(pop)
