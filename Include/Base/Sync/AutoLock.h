#pragma once

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Sync
        {
            template<typename LockType>
            class AutoLock
            {
            private:
                LockType& oLock;

            public:
                constexpr AutoLock(LockType& _oLock)
                    : oLock(_oLock)
                {
                    oLock.Lock();
                }

                AutoLock(const AutoLock&) = delete;
                AutoLock& operator=(const AutoLock&) = delete;

                ~AutoLock()
                {
                    oLock.Unlock();
                }
            };

            template<typename LockType>
            class AutoSharedLock
            {
            private:
                LockType& oLock;

            public:
                constexpr AutoSharedLock(LockType& _oLock)
                    : oLock(_oLock)
                {
                    oLock.LockShared();
                }

                AutoSharedLock(const AutoSharedLock&) = delete;
                AutoSharedLock& operator=(const AutoSharedLock&) = delete;

                ~AutoSharedLock()
                {
                    oLock.UnlockShared();
                }
            };
        }
    }
}

#pragma pack(pop)
