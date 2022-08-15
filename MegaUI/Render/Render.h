#pragma once
#include "..\base\MegaUITypeInt.h"
#include "..\base\Rect.h"
#include "..\base\Color.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Render
        {
        public:
            Render() = default;
            virtual ~Render() = default;

            Render(const Render&) = delete;
            Render& __fastcall operator=(const Render&) = delete;

            virtual
            HRESULT
            __fastcall
            BeginDraw(Rect* _pNeedPaintRect) =0;

            virtual
            HRESULT
            __fastcall
            EndDraw() = 0;

            virtual
            void
            __fastcall
            PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) = 0;

            virtual
            void
            __fastcall
            PopAxisAlignedClip() = 0;

            virtual
            void
            __fastcall
            FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) = 0;
                
            virtual
            HRESULT
            __fastcall
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) = 0;

            virtual
            HRESULT
            __fastcall
            SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) = 0;

            virtual
            D2D1_SIZE_U __fastcall GetPixelSize() = 0;
        };

        HRESULT __fastcall CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender);
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
