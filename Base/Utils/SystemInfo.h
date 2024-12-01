#pragma once
#include <Base/Utils/Version.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
#if defined(_WIN32)
            // 版本号参考来源：
            // https://github.com/MouriNaruto/MouriDocs/tree/main/docs/18

            // Windows 2000 RTM x86
            constexpr Version kWindowsNT5(5, 0, 2195, 0);

            // Windows XP RTM x86
            constexpr Version kWindowsNT5_1(5, 1, 2600, 0);

            constexpr Version kWindowsNT5_1_SP1(5, 1, 2600, 1106);

            constexpr Version kWindowsNT5_1_SP2(5, 1, 2600, 2180);

            constexpr Version kWindowsNT5_1_SP3(5, 1, 2600, 5512);

            // Windows 2003 RTM x86
            constexpr Version kWindowsNT5_2(5, 2, 3790, 0);

            // Windows XP RTM x64，Windows 2003 SP1
            constexpr Version kWindowsNT5_2_SP1(5, 2, 3790, 1830);

            // Windows Vista RTM, Windows Server 2008 RTM
            constexpr Version kWindowsNT6(6, 0, 6000, 0);

            constexpr Version kWindowsNT6_SP1(6, 0, 6001, 0);

            constexpr Version kWindowsNT6_SP2(6, 0, 6002, 0);

            // Windows 7 RTM, Windows Server 2008 R2 RTM
            constexpr Version kWindowsNT6_1(6, 1, 7600, 0);

            constexpr Version kWindowsNT6_1_SP1(6, 1, 7601, 0);

            // Windows 8 RTM, Windows Server 2012 RTM
            constexpr Version kWindowsNT6_2(6, 2, 9200, 0);

            // Windows 8.1 RTM, Windows Server 2012 R2 RTM
            constexpr Version kWindowsNT6_3(6, 3, 9600, 0);

            // Windows 10 1507
            constexpr Version kWindowsNT10_10240(10, 0, 10240, 0);

            // Windows 10 1607(RS1)
            constexpr Version kWindowsNT10_14393(10, 0, 14393, 0);

            // Windows 10 1703(RS2)
            constexpr Version kWindowsNT10_15603(10, 0, 15063, 0);

            // Windows 10 1709(RS3)，注意ARM64从这个版本开始才支持。
            constexpr Version kWindowsNT10_16299(10, 0, 16299, 0);

            // Windows 10 2004(VB)
            constexpr Version kWindowsNT10_19041(10, 0, 19041, 0);

            // Windows Server 2022(FE)
            constexpr Version kWindowsNT10_20348(10, 0, 20348, 0);
#endif
            /// <summary>
            /// 获取操作系统版本号。
            /// * Windows平台：不受系统兼容性影响，始终可获得正确版本号。
            /// </summary>
            /// <returns></returns>
            Version __YYAPI GetOperatingSystemVersion() noexcept;
        }
    }

    using namespace YY::Base::Utils;
}

#pragma pack(pop)
