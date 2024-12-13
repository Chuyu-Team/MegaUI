#pragma once
#include <Base/YY.h>

#if defined(_HAS_CXX20) && _HAS_CXX20
#include <coroutine>

#include <Base/Containers/Optional.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            template<typename ReturnType_, typename TaskType>
            struct Promise
            {
                using ReturnType = ReturnType_;
                YY::Optional<ReturnType> oValue;

                TaskType get_return_object() noexcept
                {
                    return {};
                }
                std::suspend_never initial_suspend() noexcept
                {
                    return {};
                }

                std::suspend_never final_suspend() noexcept
                {
                    return {};
                }

                void return_value(ReturnType&& _oValue) noexcept
                {
                    oValue.Emplace(std::move(_oValue));
                }

                void unhandled_exception()
                {
                }
            };

            template<typename TaskType>
            struct Promise<void, TaskType>
            {
                using ReturnType = void;

                TaskType get_return_object() noexcept
                {
                    return {};
                }
                std::suspend_never initial_suspend() noexcept
                {
                    return {};
                }

                std::suspend_never final_suspend() noexcept
                {
                    return {};
                }

                void return_void() noexcept
                {
                }
                void unhandled_exception()
                {
                }
            };


            template<typename _ReturnType>
            struct Coroutine
            {
                using promise_type = Promise<_ReturnType, Coroutine>;
            };
        }
    }

    using namespace YY::Base::Threading;
}
#pragma pack(pop)

#endif
