#pragma once
#include <memory>

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <Base/YY.h>
#include <Media/Font.h>
#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Containers/Optional.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class D2D1_0DrawContext : public DrawContext
            {
            private:
                HWND hWnd;
                D2D1_SIZE_U PixelSize;
                Optional<Rect> oPaint;
                RefPtr<ID2D1HwndRenderTarget> pRenderTarget;

            private:
                D2D1_0DrawContext()
                    : hWnd(NULL)
                    , PixelSize {}
                {
                }
    
            public:
                D2D1_0DrawContext(const D2D1_0DrawContext&) = delete;

                /// <summary>
                /// 返回全局的 D2D 1.0 厂类。注意：该指针无需释放也不能释放！
                /// </summary>
                /// <returns>失败返回 nullptr </returns>
                static _Ret_maybenull_ ID2D1Factory* __YYAPI GetD2D1Factory();

                static HRESULT __YYAPI CreateDrawTarget(_In_ HWND _hWnd, _Outptr_ DrawContext** _ppDrawContext);

                Size __YYAPI GetPixelSize() override;

                HRESULT __YYAPI SetPixelSize(
                    _In_ const Size& _PixelSize) override;

                HRESULT __YYAPI BeginDraw(_In_opt_ const Rect* _pPaint) override;

                HRESULT __YYAPI EndDraw() override;

                void __YYAPI PushAxisAlignedClip(
                    _In_ const Rect& _ClipRect) override;

                void __YYAPI PopAxisAlignedClip() override;

                void __YYAPI DrawLine(
                    _In_ Point _oPoint0,
                    _In_ Point _oPoint1,
                    _In_ Pen _oPen
                    ) override;

                void __YYAPI DrawRectangle(
                    _In_ const Rect& _oRect,
                    _In_opt_ Pen _oPen,
                    _In_opt_ Brush _oBrush
                    ) override;

                void
                __YYAPI
                DrawString(
                    _In_ uString _szText,
                    _In_ const Font& _FontInfo,
                    _In_ Brush _oBrush,
                    _In_ const Rect& _LayoutRect,
                    _In_ ContentAlignStyle _fTextAlign) override;

                void
                __YYAPI
                MeasureString(
                    _In_ uStringView _szText,
                    _In_ const Font& _FontInfo,
                    _In_ const Size& _LayoutSize,
                    _In_ ContentAlignStyle _fTextAlign,
                    _Out_ Size* _pExtent) override;

            private:
                HRESULT __YYAPI TryInitializeRenderTarget();

                HRESULT __YYAPI UpdateTargetPixelSize(_Outptr_opt_ ID2D1Bitmap** _ppBackupBitmap);

                RefPtr<ID2D1Brush> __YYAPI GetNativeBrush(_In_ Brush _oBrush);
            };
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
