#pragma once
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>

#include <Base/YY.h>
#include <Media/Font.h>
#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Containers/Optional.h>

#pragma pack(push, __YY_PACKING)

// 使用反转模型？
#define __ENABLE_FLIP_SEQUENTIAL 0
// 使用Windows组合层
#define __ENABLE_COMPOSITION_ENGINE 0

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class D2D1_1DrawContext : public DrawContext
            {
            private:
                HWND hWnd;
                D2D1_SIZE_U PixelSize;
                Optional<RECT> oPaint;
                RefPtr<ID2D1DeviceContext> pDeviceContext;
                RefPtr<ID2D1Device> pD2DDevice;
                RefPtr<ID2D1Bitmap1> pD2DTargetBitmap;
                // RefPtr<ID3D11DeviceContext> _pD3DDeviceContext;
                // RefPtr<ID2D1Bitmap> pD2DBitmapBackup;
                RefPtr<IDXGISwapChain1> pSwapChain;
                D3D_FEATURE_LEVEL FeatureLevel;
                DXGI_PRESENT_PARAMETERS PresentParameters;
#if __ENABLE_COMPOSITION_ENGINE
                RefPtr<IDCompositionDevice> pDcompDevice;
                RefPtr<IDCompositionTarget> pTarget;
#endif
                bool bFirstPaint;

                D2D1_1DrawContext(HWND _hWnd, const D2D1_SIZE_U& _oPixelSize)
                    : hWnd(_hWnd)
                    , PixelSize(_oPixelSize)
                    , FeatureLevel(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_0)
                    , PresentParameters {}
                    , bFirstPaint(true)
                {
                }

            public:
                D2D1_1DrawContext(const D2D1_1DrawContext&) = delete;

                /// <summary>
                /// 返回全局的 D2D 1.1 厂类。注意：该指针无需释放也不能释放！
                /// </summary>
                /// <returns>失败返回 nullptr </returns>
                static _Ret_maybenull_ ID2D1Factory1* __YYAPI GetD2D1Factory1();
                
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

                RefPtr<ID2D1Brush> __YYAPI GetNativeBrush(Brush _oBrush);
            };
        }
    } // namespace Media
} // namespace YY

#pragma pack(pop)
