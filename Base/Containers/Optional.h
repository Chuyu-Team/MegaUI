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
                char oBuffer[sizeof(_Type)];
                bool bHasValue;

            public:
                Optional()
                    : bHasValue(false)
                {
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
                    return bHasValue ? (_Type*)oBuffer : nullptr;
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
                void Emplace(Args&&... oArgs)
                {
                    Reset();
                    new (oBuffer) _Type(std::forward<Args>(oArgs)...);
                    bHasValue = true;
                }

                void Reset()
                {
                    if (bHasValue)
                    {
                        ((_Type*)oBuffer)->~_Type();
                        bHasValue = false;
                    }
                }

                Optional& operator=(const _Type& _oOther)
                {
                    if (bHasValue)
                    {
                        *(_Type*)oBuffer = _oOther;
                    }
                    else
                    {
                        new (oBuffer) _Type(_oOther);
                        bHasValue = true;
                    }

                    return *this;
                }
            };
        }
    } // namespace Base
} // namespace YY

#pragma pack(pop)
