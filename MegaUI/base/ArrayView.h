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
            ArrayView()
                : pData(nullptr)
                , cData(0)
            {
            }

            ArrayView(_Type* _pData, uint_t _cData)
                : pData(_pData)
                , cData(_pData ? _cData : 0)
            {
            }
            
            template<uint_t _uArrayCount>
            ArrayView(_Type (&_Array)[_uArrayCount])
                : pData(_Array)
                , cData(_uArrayCount)
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

            void __MEGA_UI_API Slice(uint_t _uRemoveStart, uint_t _uRemoveEnd = 0u)
            {
                if (_uRemoveStart + _uRemoveEnd >= cData)
                {
                    cData = 0;
                }
                else
                {
                    pData += _uRemoveStart;
                    cData -= _uRemoveStart;
                    cData -= _uRemoveEnd;
                }
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
