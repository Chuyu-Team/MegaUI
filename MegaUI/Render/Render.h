#pragma once
// #include <dwrite.h>

#include <Base/YY.h>
#include <Media/Rect.h>
#include <Media/Color.h>
#include <Base/Strings/StringView.h>
#include <Media/Font.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // 用于保存 Element 的位置以及是否缓存绘制信息
        struct ElementRenderNode
        {
            enum
            {
                // 控件位置发送变化
                InvalidatePosition = 0x00000001u,
                // 控件大小发生变化
                InvalidateExtent = 0x00000002u,
                // Node 中的显示内容发生变化。
                InvalidateContent = 0x00000004u,
            };
            // 相对于窗口的坐标系
            Rect Bounds;

            // Invalidate 位组合
            uint32_t uInvalidateMarks;
        };

#define LIGHT 0.5
#define VERYLIGHT 0.8
#define DARK -0.3
#define VERYDARK -0.75

        /// <summary>
        /// 调整亮度
        /// </summary>
        /// <param name="cr"></param>
        /// <param name="fIllum">1 >= fIllum >= -1</param>
        /// <returns></returns>
        inline Color __YYAPI AdjustBrightness(Color cr, double fIllum)
        {
            double r = cr.Red, g = cr.Green, b = cr.Blue;

            if (fIllum > 0.0)
            {
                r += (255.0 - r) * fIllum;
                g += (255.0 - g) * fIllum;
                b += (255.0 - b) * fIllum;
            }
            else
            {
                r += r * fIllum;
                g += g * fIllum;
                b += b * fIllum;
            }

            return Color(cr.Alpha, (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
        
        float __YYAPI DevicePixelToPixel(float _iRelativePixel, int32_t _DPI);

        float __YYAPI PointToPixel(float _iFontPoint, int32_t _DPI);

        Rect __YYAPI DevicePixelToPixel(Rect _iRelativePixelRect, int32_t _DPI);

        Size __YYAPI DevicePixelToPixel(Size _iRelativePixelSize, int32_t _DPI);

        float __YYAPI UpdatePixel(float _iOldPixel, int32_t _OldDPI, int32_t _NewDPI);

    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
