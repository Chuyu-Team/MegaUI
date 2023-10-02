#pragma once

#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Memory/Alloc.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Memory
{
    template<typename _Type>
    class RefPtr;
    
    template<typename _Type>
    class WeakPtrRef
    {
    };

    template<typename _Type>
    class WeakPtr
    {
    private:
        _Type* p;

    public:
        constexpr WeakPtr() noexcept
            : p(nullptr)
        {
        }

        WeakPtr(_In_opt_ _Type* _pOther) noexcept
            : p(_pOther)
        {
            if (p)
                p->AddWeakRef();
        }

        constexpr WeakPtr(_In_opt_ WeakPtrRef<_Type>* _pOther) noexcept
            : p(reinterpret_cast<_Type*>(_pOther))
        {
        }

        WeakPtr(_In_ const WeakPtr& _pOther) noexcept
            : WeakPtr(_pOther.p)
        {
        }

        WeakPtr(_In_ WeakPtr&& _pOther) noexcept
            : p(_pOther.p)
        {
            _pOther.p = nullptr;
        }

        ~WeakPtr()
        {
            if (p)
                p->ReleaseWeak();
        }

        inline void __YYAPI Attach(_In_opt_ WeakPtrRef<_Type>* _pOther) noexcept
        {
            if (p)
                p->ReleaseWeak();
            p = reinterpret_cast<_Type*>(_pOther);
        }

        inline _Ret_maybenull_ WeakPtrRef<_Type>* __YYAPI Detach() noexcept
        {
            auto _p = p;
            p = nullptr;
            return reinterpret_cast<WeakPtrRef<_Type>*>(_p);
        }

        _Ret_maybenull_ RefPtr<_Type> __YYAPI Get() const noexcept
        {
            RefPtr<_Type> _pTmp;

            if (p && p->TryAddRef())
            {
                _pTmp.Attach(p);
            }
            return _pTmp;
        }

        _Ret_maybenull_ RefPtr<_Type> __YYAPI Get() noexcept
        {
            RefPtr<_Type> _pTmp;

            if (p)
            {
                if (p->TryAddRef())
                {
                    _pTmp.Attach(p);
                }
                else
                {
                    // weak ptr已经无效，所以我们释放自己。
                    *this = nullptr;
                }
            }
            return _pTmp;
        }

        bool operator==(_In_opt_ _Type* _pOther) const noexcept
        {
            return p == _pOther;
        }
        
        bool operator==(_In_opt_ const WeakPtr& _pOther) const noexcept
        {
            return p == _pOther.p;
        }
        
        WeakPtr& operator=(std::nullptr_t) noexcept
        {
            if (p)
                p->ReleaseWeak();

            p = nullptr;
            return *this;
        }

        WeakPtr& operator=(_In_opt_ _Type* _pOther) noexcept
        {
            if (p != _pOther)
            {
                if (p)
                    p->ReleaseWeak();

                p = _pOther;

                if (p)
                    p->AddWeakRef();
            }
            return *this;
        }
        
        WeakPtr& operator=(const WeakPtr& _pOther) noexcept
        {
            return this->operator=(_pOther.p);
        }

        WeakPtr& operator=(const WeakPtr&& _pOther) noexcept
        {
            if (&_pOther != this)
            {
                if (p)
                    p->ReleaseWeak();

                p = _pOther.p;
                _pOther.p = nullptr;
            }

            return *this;
        }
    };
}

namespace YY
{
    using namespace YY::Base::Memory;
}

#pragma pack(pop)
