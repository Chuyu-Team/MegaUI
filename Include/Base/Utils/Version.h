#pragma once
#include <Base/YY.h>

#if defined(_HAS_CXX20) && _HAS_CXX20
#include <compare>
#endif

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
            union Version
            {
            public:
                uint64_t uInternalValue = 0ull;

                struct
                {
                    uint16_t uRevision;
                    uint16_t uBuild;
                    uint16_t uMinor;
                    uint16_t uMajor;
                };

                struct
                {
                    uint32_t uLowPart;
                    uint32_t uHightPart;
                };

                constexpr Version() = default;

                constexpr Version(uint16_t _uMajor, uint16_t _uMinor, uint16_t _uBuild = 0, uint16_t _uRevision = 0)
                    : uMajor(_uMajor)
                    , uMinor(_uMinor)
                    , uBuild(_uBuild)
                    , uRevision(_uRevision)
                {
                }

                Version& __YYAPI operator=(const Version& _oOther)
                {
                    uInternalValue = _oOther.uInternalValue;
                    return *this;
                }

                constexpr bool __YYAPI operator==(const Version& _oOther) const noexcept
                {
                    return uInternalValue == _oOther.uInternalValue;
                }

#if defined(_HAS_CXX20) && _HAS_CXX20
                constexpr auto __YYAPI operator<=>(const Version& _oOther) const noexcept
                {
                    return uInternalValue <=> _oOther.uInternalValue;
                }
#else
                constexpr bool __YYAPI operator<(const Version& _oOther) const noexcept
                {
                    return uInternalValue < _oOther.uInternalValue;
                }

                constexpr bool __YYAPI operator<=(const Version& _oOther) const noexcept
                {
                    return uInternalValue <= _oOther.uInternalValue;
                }

                constexpr bool __YYAPI operator>=(const Version& _oOther) const noexcept
                {
                    return uInternalValue >= _oOther.uInternalValue;
                }

                constexpr bool __YYAPI operator>(const Version& _oOther) const noexcept
                {
                    return uInternalValue > _oOther.uInternalValue;
                }

                constexpr bool __YYAPI operator!=(const Version& _oOther) const noexcept
                {
                    return uInternalValue > _oOther.uInternalValue;
                }
#endif
            };

            static_assert(sizeof(Version) == sizeof(uint64_t), "");
        }
    }

    using namespace YY::Base::Utils;
}

#pragma pack(pop)
