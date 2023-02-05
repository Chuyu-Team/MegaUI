#pragma once
#include "MegaUITypeInt.h"

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        template<typename _Type>
        class ComPtr
        {
        private:
            _Type* p;
            
        public:
            ComPtr()
                : p(nullptr)
            {
            }

            ComPtr(const ComPtr& _Other)
                : p(_Other.p)
            {
                if (p)
                    p->AddRef();
            }


            ~ComPtr()
            {
                if (p)
                    p->Release();
            }

            _Type** __YYAPI operator&()
            {
                return &p;
            }

            __YYAPI operator _Type*() const
            {
                return p;
            }

            _Type * __YYAPI operator->() const
            {
                return p;
            }


            inline _Type* __YYAPI Detach()
            {
                auto _p = p;
                p = nullptr;
                return _p;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
