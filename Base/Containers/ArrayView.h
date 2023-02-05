#pragma once

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Containers
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

                _Type* __YYAPI GetData() noexcept
                {
                    return pData;
                }

                const _Type* __YYAPI GetData() const noexcept
                {
                    return pData;
                }

                uint_t __YYAPI GetSize() const
                {
                    return cData;
                }

                void __YYAPI Slice(uint_t _uRemoveStart, uint_t _uRemoveEnd = 0u)
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

                _Type& __YYAPI operator[](_In_ uint_t _uIndex)
                {
                    return pData[_uIndex];
                }

                const _Type& __YYAPI operator[](_In_ uint_t _uIndex) const
                {
                    return pData[_uIndex];
                }

                _Type* __YYAPI begin() noexcept
                {
                    return pData;
                }

                _Type* __YYAPI end() noexcept
                {
                    return pData + GetSize();
                }

                const _Type* __YYAPI begin() const noexcept
                {
                    return pData;
                }

                const _Type* __YYAPI end() const noexcept
                {
                    return pData + GetSize();
                }

                _Type* __YYAPI _Unchecked_begin() noexcept
                {
                    return pData;
                }

                _Type* __YYAPI _Unchecked_end() noexcept
                {
                    return pData + GetSize();
                }

                const _Type* __YYAPI _Unchecked_begin() const noexcept
                {
                    return pData;
                }

                const _Type* __YYAPI _Unchecked_end() const noexcept
                {
                    return pData + GetSize();
                }
            };
        } // namespace Containers
    } // namespace Base

    using namespace YY::Base::Containers;
} // namespace YY

#pragma pack(pop)
