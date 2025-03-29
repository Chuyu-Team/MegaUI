#pragma once
#include <Base/YY.h>
#include <Base/Time/Common.h>
#include <Base/Time/TimeSpan.h>
#include <Base/Utils/MathUtils.h>

#if defined(_HAS_CXX20) && _HAS_CXX20
#include <compare>
#endif

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
#if defined(_DEBUG)
                    static uint64_t s_uTestValue = 0;
                    if(s_uTestValue)
                        return s_uTestValue * GetSecondsPerInternal() / SecondsPerMillisecond;
#endif
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

                constexpr static TickCount __YYAPI GetMax() noexcept
                {
                    return TickCount(UINT64_MAX);
                }

                constexpr static TickCount __YYAPI FromInternalValue(uint64_t _uTickCountInternal) noexcept
                {
                    return TickCount(_uTickCountInternal);
                }

                /// <summary>
                /// </summary>
                /// <param name="_uTickCountMicroseconds">开机以来的微秒数</param>
                /// <returns></returns>
                constexpr static TickCount __YYAPI FromMicroseconds(int64_t _uTickCountMicroseconds) noexcept
                {
                    return TickCount(_uTickCountMicroseconds * GetSecondsPerInternal() / (SecondsPerMillisecond * MillisecondsPerMicrosecond));
                }

                constexpr static TickCount __YYAPI FromMilliseconds(int64_t _uTickCountMilliseconds) noexcept
                {
                    return TickCount(_uTickCountMilliseconds * GetSecondsPerInternal() / SecondsPerMillisecond);
                }

                constexpr static TickCount __YYAPI FromSeconds(int64_t _uTickCountSeconds) noexcept
                {
                    return TickCount(_uTickCountSeconds * GetSecondsPerInternal());
                }

                constexpr static TickCount __YYAPI FromMinutes(int64_t _uTickCountMinutes) noexcept
                {
                    return TickCount(_uTickCountMinutes * GetSecondsPerInternal() * MinutesPerSecond);
                }

                constexpr static TickCount __YYAPI FromHours(int64_t _uTickCountHours) noexcept
                {
                    return TickCount(_uTickCountHours * GetSecondsPerInternal() * MinutesPerSecond * HoursPerMinute);
                }

                constexpr static TickCount __YYAPI FromDays(int64_t _uTickCountDays) noexcept
                {
                    return TickCount(_uTickCountDays * GetSecondsPerInternal() * MinutesPerSecond * HoursPerMinute * DaysPerHour);
                }

                constexpr uint64_t __YYAPI GetInternalValue() const noexcept
                {
                    return uTickCountInternal;
                }

                constexpr static uint64_t __YYAPI GetSecondsPerInternal() noexcept
                {
                    return TickCountCommon_t::GetSecondsPerInternal();
                }

                constexpr uint64_t __YYAPI GetMicroseconds() const noexcept
                {
                    return uTickCountInternal * SecondsPerMillisecond * MillisecondsPerMicrosecond / GetSecondsPerInternal();
                }

                constexpr uint64_t __YYAPI GetMilliseconds() const noexcept
                {
                    return uTickCountInternal * SecondsPerMillisecond / GetSecondsPerInternal();
                }

                constexpr uint64_t __YYAPI GetSeconds() const noexcept
                {
                    return uTickCountInternal / GetSecondsPerInternal();
                }

                constexpr uint64_t __YYAPI GetMinutes() const noexcept
                {
                    return uTickCountInternal / (GetSecondsPerInternal() * MinutesPerSecond);
                }

                constexpr uint64_t __YYAPI GetHours() const noexcept
                {
                    return uTickCountInternal / (GetSecondsPerInternal() * MinutesPerSecond * HoursPerMinute);
                }

                constexpr uint64_t __YYAPI GetDays() const noexcept
                {
                    return uTickCountInternal / (GetSecondsPerInternal() * MinutesPerSecond * HoursPerMinute * DaysPerHour);
                }
        
                constexpr TickCount& operator=(const TickCount&) noexcept = default;

                constexpr bool operator==(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal == _oOther.uTickCountInternal;
                }

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

                constexpr bool operator<=(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal <= _oOther.uTickCountInternal;
                }

                constexpr bool operator<(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal < _oOther.uTickCountInternal;
                }

                constexpr bool operator!=(const TickCount& _oOther) const noexcept
                {
                    return uTickCountInternal != _oOther.uTickCountInternal;
                }
#endif

                template<TimePrecise InputPrecise>
                constexpr TickCount& operator+=(const TimeSpan<InputPrecise>& _nSpan) noexcept
                {
                    // uTickCountInternal += _nSpan.GetInternalValue() * GetSecondsPerInternal() / TimeSpan<InputPrecise>::GetSecondsPerInternal();
                    uTickCountInternal += UMulDiv64Fast(_nSpan.GetInternalValue(), GetSecondsPerInternal(), TimeSpan<InputPrecise>::GetSecondsPerInternal());
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
                    // uTickCountInternal -= _nSpan.GetInternalValue() * GetSecondsPerInternal() / TimeSpan<InputPrecise>::GetSecondsPerInternal();
                    uTickCountInternal -= UMulDiv64Fast(_nSpan.GetInternalValue(), GetSecondsPerInternal(), TimeSpan<InputPrecise>::GetSecondsPerInternal());
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
                    int64_t _nSpanInternal = uTickCountInternal - _oOther.uTickCountInternal;
                    // return TimeSpan<ePrecise>::FromInternalValue(_nSpanInternal * TimeSpan<ePrecise>::GetSecondsPerInternal() / int64_t(GetSecondsPerInternal()));
                    return TimeSpan<ePrecise>::FromInternalValue(MulDiv64Fast(_nSpanInternal, TimeSpan<ePrecise>::GetSecondsPerInternal(), GetSecondsPerInternal()));
                }
            };
        }
    }
}

#pragma pack(pop)
