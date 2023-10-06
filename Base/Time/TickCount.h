#pragma once
#include <Base/YY.h>
#include <Base/Time/Common.h>
#include <Base/Time/TimeSpan.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::Time
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
#endif

#if _WIN32
        static int64_t __YYAPI GetSecondsPerInternal() noexcept
        {
            static LARGE_INTEGER s_Frequency = {};
            if (s_Frequency.QuadPart == 0)
            {
                QueryPerformanceFrequency(&s_Frequency);
            }
            return s_Frequency.QuadPart;
        }
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
        uint64_t uTickCountInternal;

        using TickCountCommon = TickCountCommon<ePrecise>;

        constexpr TickCount() noexcept
            : uTickCountInternal(0u)
        {
        }

        constexpr TickCount(const TickCount&) noexcept = default;

        static TickCount __YYAPI GetCurrent() noexcept
        {
            return TickCount(TickCountCommon::GetCurrentInternalValue());
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
            return TickCountCommon::GetSecondsPerInternal();
        }
        
        constexpr TickCount& operator=(const TickCount&) noexcept = default;

        constexpr bool operator==(const TickCount& _oOther) const noexcept
        {
            return uTickCountInternal == _oOther.uTickCountInternal;
        }

        constexpr auto operator<=>(const TickCount& _oOther) const noexcept = default;

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
            return TimeSpan<ePrecise>((uTickCountInternal - _oOther.uTickCountInternal) * TimeSpan<ePrecise>::GetSecondsPerInternal() / GetSecondsPerInternal());
        }
    };
}

#pragma pack(pop)
