#pragma once

#include <MegaUI/base/MegaUITypeInt.h>
#include <MegaUI/base/StringBase.h>
#include <MegaUI/base/Color.h>

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        namespace FontWeight
        {
            constexpr auto Thin = 100;
            constexpr auto ExtraLight = 200;
            constexpr auto UltraLight = 200;
            constexpr auto Light = 300;
            constexpr auto SemiLight = 350;
            constexpr auto Normal = 400;
            constexpr auto Regular = 400;
            constexpr auto Medium = 500;
            constexpr auto DemiBold = 600;
            constexpr auto SemiBold = 600;
            constexpr auto Bold = 700;
            constexpr auto ExtraBold = 800;
            constexpr auto UltraBold = 800;
            constexpr auto Black = 900;
            constexpr auto Heavy = 900;
            constexpr auto ExtraBlack = 950;
            constexpr auto UltraBlack = 950;
        } // namespace FontWeight

        namespace FontStyle
        {
            constexpr auto None = 0x00000000;
            constexpr auto Italic = 0x00000001;
            constexpr auto Underline = 0x00000002;
            constexpr auto StrikeOut = 0x00000004;
        }

        // ��������Ļ�����Ϣ
        struct Font
        {
            // ��������
            uString szFace;
            // �����С
            float iSize = 0;
            // ����Ĵ�ϸ��FontWeight
            uint32_t uWeight = 0;
            // FontStyle ��λ���
            uint32_t fStyle = 0;
            // ������ɫ
            Color Color = 0;
        };
    }
} // namespace YY

#pragma pack(pop)
