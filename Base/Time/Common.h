#pragma once

namespace YY
{
    namespace Base
    {
        namespace Time
        {
            enum class TimePrecise
            {
                // Nanosecond,
                Microsecond,
                Millisecond,
            };

            constexpr int64_t MicrosecondPerNanosecond = 1000000;
            constexpr int64_t MillisecondsPerMicrosecond = 1000;
            constexpr int64_t SecondsPerMillisecond = 1000;
            constexpr int64_t MinutesPerSecond = 60;
            constexpr int64_t HoursPerMinute = 60;
            constexpr int64_t DaysPerHour = 24;
        }
    }
}

namespace YY
{
    using namespace YY::Base::Time;
}
