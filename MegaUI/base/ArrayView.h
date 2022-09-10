#pragma once

#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        template<typename _Type>
        class ArrayView
        {
        private:
            _Type* pData;
            uint_t cData;

        public:
            ArrayView(_Type* _pData = nullptr, uint_t _cData = 0)
                : pData(_pData)
                , cData(_pData ? _cData : 0)
            {
            }

            _Type* __MEGA_UI_API GetData() noexcept
            {
                return pData;
            }

            const _Type* __MEGA_UI_API GetData() const noexcept
            {
                return pData;
            }

            uint_t __MEGA_UI_API GetSize() const
            {
                return cData;
            }

            
            _Type& __MEGA_UI_API operator[](_In_ uint_t _uIndex)
            {
                return pData[_uIndex];
            }

            const _Type& __MEGA_UI_API operator[](_In_ uint_t _uIndex) const
            {
                return pData[_uIndex];
            }
            

            _Type* __MEGA_UI_API begin() noexcept
            {
                return pData;
            }

            _Type* __MEGA_UI_API end() noexcept
            {
                return pData + GetSize();
            }

            const _Type* __MEGA_UI_API begin() const noexcept
            {
                return pData;
            }

            const _Type* __MEGA_UI_API end() const noexcept
            {
                return pData + GetSize();
            }

            _Type* __MEGA_UI_API _Unchecked_begin() noexcept
            {
                return pData;
            }

            _Type* __MEGA_UI_API _Unchecked_end() noexcept
            {
                return pData + GetSize();
            }

            const _Type* __MEGA_UI_API _Unchecked_begin() const noexcept
            {
                return pData;
            }

            const _Type* __MEGA_UI_API _Unchecked_end() const noexcept
            {
                return pData + GetSize();
            }
        };
    }
} // namespace YY

#pragma pack(pop)
