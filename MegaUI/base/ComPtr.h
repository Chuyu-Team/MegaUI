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

            _Type** __MEGA_UI_API operator&()
            {
                return &p;
            }

            __MEGA_UI_API operator _Type*() const
            {
                return p;
            }

            _Type * __MEGA_UI_API operator->() const
            {
                return p;
            }


            inline _Type* __MEGA_UI_API Detach()
            {
                auto _p = p;
                p = nullptr;
                return _p;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
