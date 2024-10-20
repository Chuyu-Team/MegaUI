#pragma once
#include <d2d1_1.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <dcomp.h>

#include <Base/YY.h>
#include <Media/Font.h>
#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Containers/Optional.h>
#include <Base/Sync/Interlocked.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class D2D1_1DrawContext : public DrawContext
            {
            public:
                class DirectDeviceCache
                {
                private:
                    uint32_t uRef;

                public:
                    D3D_FEATURE_LEVEL FeatureLevel;
                    RefPtr<ID3D11Device> pD3DDevice;
                    RefPtr<ID3D11DeviceContext> pD3DDeviceContext;
                    RefPtr<ID2D1Device> pD2DDevice;
                    // 可选
                    RefPtr<IDCompositionDevice> pDcompDevice;

                    DirectDeviceCache()
                        : uRef(1)
                        , FeatureLevel((D3D_FEATURE_LEVEL)0)
                    {
                    }

                    DirectDeviceCache(const DirectDeviceCache&) = delete;
                    DirectDeviceCache& operator=(const DirectDeviceCache&) = delete;

                    uint32_t AddRef()
                    {
                        return Sync::Increment(&uRef);
                    }

                    uint32_t Release()
                    {
                        auto _uNewRef = Sync::Decrement(&uRef);
                        if (_uNewRef == 0)
                            delete this;
                        return _uNewRef;
                    }
                };

            private:
                HWND hWnd;
                D2D1_SIZE_U PixelSize;
                Optional<RECT> oPaint;
                RefPtr<ID2D1DeviceContext> pDeviceContext;
                RefPtr<DirectDeviceCache> pDirectDeviceCache;
                RefPtr<ID2D1Bitmap1> pD2DTargetBitmap;
                RefPtr<IDXGISwapChain1> pSwapChain;
                DXGI_PRESENT_PARAMETERS PresentParameters;
                RefPtr<IDCompositionTarget> pTarget;
                uint32_t uSwapChainBufferCount;

                bool bFirstPaint;

                D2D1_1DrawContext(HWND _hWnd, const D2D1_SIZE_U& _oPixelSize)
                    : hWnd(_hWnd)
                    , PixelSize(_oPixelSize)
                    , PresentParameters {}
                    , uSwapChainBufferCount(0)
                    , bFirstPaint(true)
                {
                }

                D2D1_1DrawContext(const D2D1_1DrawContext&) = delete;
                D2D1_1DrawContext& operator=(const D2D1_1DrawContext&) = delete;

            public:
                static _Ret_maybenull_ DrawContextFactory* __YYAPI GetDrawContextFactory();

                /// <summary>
                /// 返回全局的 D2D 1.1 厂类。注意：该指针无需释放也不能释放！
                /// </summary>
                /// <returns>失败返回 nullptr </returns>
                static _Ret_maybenull_ ID2D1Factory1* __YYAPI GetD2D1Factory();            

                static RefPtr<DirectDeviceCache> __YYAPI GetDirectDeviceCache();

                /// <summary>
                /// 将之前获取到的DirectDeviceCache无效处理。仅在设备需要重新创建时调用。
                /// </summary>
                static void __YYAPI InvalidDirectDeviceCache(RefPtr<DirectDeviceCache>&& _oD3DDeviceCache);

                static bool __YYAPI IsSupport();

                static bool __YYAPI IsMicrosoftCompositionEngineSupport();

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
                
                HRESULT __YYAPI UpdateTargetPixelSize(bool _bCopyToNewTarget);

                RefPtr<ID2D1Brush> __YYAPI GetNativeBrush(Brush _oBrush);
            };
        }
    } // namespace Media
} // namespace YY

#pragma pack(pop)
