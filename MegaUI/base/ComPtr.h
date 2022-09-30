#pragma once
#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

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

            _Type** operator&()
            {
                return &p;
            }

            operator _Type*() const
            {
                return p;
            }

            _Type* operator->() const
            {
                return p;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
