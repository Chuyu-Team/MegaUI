#pragma once
#include <Base/YY.h>
#include <Base/Exception.h>
#include <Base/Sync/Interlocked.h>
#include <Base/Containers/ConstructorPolicy.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            template<typename _Type>
            class Optional
            {
            private:
                union
                {
                    char oValueBuffer[sizeof(_Type)] = {};
                    _Type oValue;
                };

                bool bHasValue = false;

            public:
                constexpr Optional() = default;

                constexpr Optional(const _Type& _oValue)
                    : oValue(_oValue)
                    , bHasValue(true)
                {
                }
                
                constexpr Optional(_Type&& _oValue)
                    : oValue(std::move(_oValue))
                    , bHasValue(true)
                {
                }

                constexpr Optional(const Optional& _oOther)
                {
                    if (_oOther.bHasValue)
                    {
                        new (&oValue) _Type(_oOther.oValue);
                        bHasValue = true;
                    }
                }

                constexpr Optional(Optional&& _oOther) noexcept
                {
                    if (_oOther.bHasValue)
                    {
                        new (&oValue) _Type(std::move(_oOther.oValue));
                        _oOther.Reset();
                        bHasValue = true;
                    }
                }

                ~Optional()
                {
                    Reset();
                }

                inline bool HasValue() const
                {
                    return bHasValue;
                }

                _Type* GetValuePtr()
                {
                    return bHasValue ? &oValue : nullptr;
                }

                inline const _Type* GetValuePtr() const
                {
                    return const_cast<Optional*>(this)->GetValuePtr();
                }

                inline _Type& GetValue()
                {
                    return *GetValuePtr();
                }

                inline const _Type& GetValue() const
                {
                    return *GetValuePtr();
                }

                template<typename... Args>
                _Type& Emplace(Args&&... oArgs)
                {
                    Reset();
                    new (&oValue) _Type(std::forward<Args>(oArgs)...);
                    bHasValue = true;
                    return oValue;
                }

                void Reset()
                {
                    if (bHasValue)
                    {
                        oValue.~_Type();
                        bHasValue = false;
                    }
                }

                Optional& operator=(const _Type& _oValue)
                {
                    if (bHasValue)
                    {
                        if (&oValue != &_oValue)
                        {
                            oValue = _oValue;
                        }
                    }
                    else
                    {
                        new (&oValue) _Type(_oValue);
                        bHasValue = true;
                    }

                    return *this;
                }

                Optional& operator=(_Type&& _oValue)
                {
                    if (bHasValue)
                    {
                        if (&oValue != &_oValue)
                        {
                            oValue = std::move(_oValue);
                        }
                    }
                    else
                    {
                        new (&oValue) _Type(std::move(_oValue));
                        bHasValue = true;
                    }

                    return *this;
                }

                Optional& operator=(const Optional& _oOther)
                {
                    if (_oOther.HasValue())
                    {
                        operator=(_oOther);
                    }
                    else
                    {
                        Reset();
                    }

                    return *this;
                }

                Optional& operator=(Optional&& _oOther)
                {
                    if(this == &_oOther)
                        return *this;

                    if (_oOther.HasValue())
                    {
                        operator=(std::move(_oOther));
                        _oOther.Reset();
                    }
                    else
                    {
                        Reset();
                    }

                    return *this;
                }
                
                bool operator==(const Optional& _oOther) const
                {
                    if (HasValue() != _oOther.HasValue())
                        return false;

                    if (!HasValue())
                    {
                        return true;
                    }

                    return GetValue() == _oOther.GetValue();
                }

#if !defined(_HAS_CXX20) || _HAS_CXX20 == 0
                bool operator!=(const Optional& _oOther) const
                {
                    if (HasValue() != _oOther.HasValue())
                        return true;

                    if (!HasValue())
                    {
                        return false;
                    }

                    return GetValue() != _oOther.GetValue();
                }
#endif
            };
        }
    } // namespace Base

    using namespace YY::Base::Containers;
} // namespace YY

#pragma pack(pop)
