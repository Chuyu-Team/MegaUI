#pragma once
#include <memory>

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <d3d10_1.h>

#include <YY/Base/YY.h>
#include <Media/Font.h>
#include <YY/Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <YY/Base/Containers/Optional.h>

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
                RefPtr<ID2D1RenderTarget> pRenderTarget;
                RefPtr<ID3D10Device1> pD3DDevice;
                RefPtr<IDXGISwapChain> pSwapChain;
                RefPtr<ID3D10Texture2D> pBackBuffer;
                bool bFirstPaint;

            private:
                D2D1_0DrawContext()
                    : hWnd(NULL)
                    , PixelSize {}
                    , bFirstPaint(true)
                {
                }

                D2D1_0DrawContext(const D2D1_0DrawContext&) = delete;

                D2D1_0DrawContext& operator=(const D2D1_0DrawContext&) = delete;

            public:
                static _Ret_maybenull_ DrawContextFactory* __YYAPI GetDrawContextFactory();

                /// <summary>
                /// 返回全局的 D2D 1.0 厂类。注意：该指针无需释放也不能释放！
                /// </summary>
                /// <returns>失败返回 nullptr </returns>
                static _Ret_maybenull_ ID2D1Factory* __YYAPI GetD2D1Factory();

                static RefPtr<ID3D10Device1> __YYAPI GetD3D10DeviceCache();

                static void __YYAPI InvalidD3D10DeviceCache(RefPtr<ID3D10Device1>&& _pD3D10Device1Cache);

                static HRESULT __YYAPI CreateDrawTarget(_In_ HWND _hWnd, _Outptr_ DrawContext** _ppDrawContext);

                static bool __YYAPI IsSupport();

                static bool __YYAPI IsMicrosoftCompositionEngineSupport();

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

                RefPtr<IDWriteTextLayout> __YYAPI CreateTextLayout(
                    _In_ uString _szText,
                    _In_ const Font& _FontInfo,
                    _In_ const Size& _LayoutSize,
                    _In_ ContentAlignStyle _fTextAlign) override;

                void
                __YYAPI
                DrawString2(
                    _In_ const Point& _Origin,
                    _In_ RefPtr<IDWriteTextLayout> _pTextLayout,
                    _In_ Brush _oBrush) override;

            private:
                HRESULT __YYAPI TryInitializeRenderTarget();

                HRESULT __YYAPI UpdateTargetPixelSize(_In_ bool _bCopyToNewTarget);

                RefPtr<ID2D1Brush> __YYAPI GetNativeBrush(_In_ Brush _oBrush);
            };
        } // namespace Graphics
    }     // namespace Media
} // namespace YY

#pragma pack(pop)
