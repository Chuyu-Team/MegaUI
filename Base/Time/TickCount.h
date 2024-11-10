#pragma once
#include <Base/YY.h>
#include <Base/Time/Common.h>
#include <Base/Time/TimeSpan.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Time
        {
            template<TimePrecise kPrecise>
            class TickCountCommon;

            template<>
            class TickCountCommon<TimePrecise::Microsecond>
            {
            public:
#if _WIN32
                static uint64_t __YYAPI GetCurrentInternalValue() noexcept
                {
                    LARGE_INTEGER _PerformanceCounter = {};
                    QueryPerformanceCounter(&_PerformanceCounter);
                    return _PerformanceCounter.QuadPart;
                }

                static int64_t __YYAPI GetSecondsPerInternal() noexcept
                {
                    static LARGE_INTEGER s_Frequency = {};
                    if (s_Frequency.QuadPart == 0)
                    {
                        QueryPerformanceFrequency(&s_Frequency);
                    }
                    return s_Frequency.QuadPart;
                }
#else
                static uint64_t __YYAPI GetCurrentInternalValue() noexcept;

                static int64_t __YYAPI GetSecondsPerInternal() noexcept;
#endif
            };

            template<>
            class TickCountCommon<TimePrecise::Millisecond>
            {
            public:
#if _WIN32
                static uint64_t __YYAPI GetCurrentInternalValue() noexcept
                {
                    return GetTickCount64();
                }
#else
                static uint64_t __YYAPI GetCurrentInternalValue() noexcept;
#endif
                constexpr static int64_t __YYAPI GetSecondsPerInternal() noexcept
                {
                    return SecondsPerMillisecond;
                }
            };

            template<TimePrecise ePrecise>
            class TickCount
            {
            private:
                // 防止错误的从 uint64_t 构造，声明 为 private
                // 如有需要请使用 TickCount::FromInternalValue
                constexpr TickCount(uint64_t _uTickCountInternal) noexcept
                    : uTickCountInternal(_uTickCountInternal)
                {
                }

            public:
                uint64_t uTickCountInternal = 0u;

                using TickCountCommon_t = TickCountCommon<ePrecise>;

                constexpr TickCount() noexcept = default;

                constexpr TickCount(const TickCount&) noexcept = default;

                static TickCount __YYAPI GetCurrent() noexcept
                {
                    return TickCount(TickCountCommon_t::GetCurrentInternalValue());
                }

                constexpr static TickCount __YYAPI FromInternalValue(uint64_t _uTickCountInternal) noexcept
                {
                    return TickCount(_uTickCountInternal);
                }

                constexpr uint64_t __YYAPI GetInternalValue() const noexcept
                {
                    return uTickCountInternal;
                }

                constexpr static int64_t __YYAPI GetSecondsPerInternal() noexcept
                {
                    return TickCountCommon_t::GetSecondsPerInternal();
                }
        
                constexpr TickCount& operator=(const TickCount&) noexcept = default;

#if defined(_HAS_CXX20) && _HAS_CXX20
                constexpr auto operator<=>(const TickCount& _oOther) const noexcept = default;
#else
                constexpr bool operator>(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal > _oOther.uTickCountInternal;
                }

                constexpr bool operator>=(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal >= _oOther.uTickCountInternal;
                }

                constexpr bool operator==(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal == _oOther.uTickCountInternal;
                }

                constexpr bool operator<=(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal <= _oOther.uTickCountInternal;
                }

                constexpr bool operator<(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal < _oOther.uTickCountInternal;
                }
#endif

                template<TimePrecise InputPrecise>
                constexpr TickCount& operator+=(const TimeSpan<InputPrecise>& _nSpan) noexcept
                {
                    uTickCountInternal += _nSpan.GetInternalValue() * GetSecondsPerInternal() / TimeSpan<InputPrecise>::GetSecondsPerInternal();
                    return *this;
                }

                template<TimePrecise InputPrecise>
                constexpr TickCount operator+(const TimeSpan<InputPrecise>& _nSpan) noexcept
                {
                    TickCount _oTmp = *this;
                    _oTmp += _nSpan;
                    return _oTmp;
                }

                template<TimePrecise InputPrecise>
                constexpr TickCount& operator-=(const TimeSpan<InputPrecise>& _nSpan) noexcept
                {
                    uTickCountInternal -= _nSpan.GetInternalValue() * GetSecondsPerInternal() / TimeSpan<InputPrecise>::GetSecondsPerInternal();
                    return *this;
                }

                template<TimePrecise InputPrecise>
                constexpr TickCount operator-(const TimeSpan<InputPrecise>& _nSpan) noexcept
                {
                    TickCount _oTmp = *this;
                    _oTmp -= _nSpan;
                    return _oTmp;
                }

                constexpr TimeSpan<ePrecise> operator-(const TickCount& _oOther) const noexcept
                {
                    return TimeSpan<ePrecise>::FromInternalValue((uTickCountInternal - _oOther.uTickCountInternal) * TimeSpan<ePrecise>::GetSecondsPerInternal() / GetSecondsPerInternal());
                }
            };
        }
    }
}

#pragma pack(pop)
