#pragma once
#include "..\base\MegaUITypeInt.h"
#include "..\base\Rect.h"
#include "..\base\Color.h"

#pragma pack(push, __MEGA_UI_PACKING)

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

        class Render
        {
        public:
            Render() = default;
            virtual ~Render() = default;

            Render(const Render&) = delete;
            Render& __MEGA_UI_API operator=(const Render&) = delete;

            virtual
            HRESULT
            __MEGA_UI_API
            BeginDraw(Rect* _pNeedPaintRect) =0;

            virtual
            HRESULT
            __MEGA_UI_API
            EndDraw() = 0;

            virtual
            void
            __MEGA_UI_API
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) = 0;

            virtual
            void
            __MEGA_UI_API
            PopAxisAlignedClip() = 0;

            virtual
            void
            __MEGA_UI_API
            FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) = 0;
                
            virtual
            HRESULT
            __MEGA_UI_API
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) = 0;

            virtual
            HRESULT
            __MEGA_UI_API
            SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) = 0;

            virtual
            D2D1_SIZE_U __MEGA_UI_API GetPixelSize() = 0;
        };

        HRESULT __MEGA_UI_API CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender);
        
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
        inline Color __MEGA_UI_API AdjustBrightness(Color cr, double fIllum)
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

    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
