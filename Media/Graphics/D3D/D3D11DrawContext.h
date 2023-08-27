#pragma once
#include <memory>

#include <d3d11.h>
#include <dwrite.h>
#include <dwrite_2.h>

#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Sync/SingleLinkedList.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class D3D11DrawContext
                : public DrawContext
                , public IDWriteTextRenderer1
            {
            private:
                RefPtr<IDXGISwapChain> pSwapChain;
                RefPtr<ID3D11Device> pD3D11Device;
                RefPtr<ID3D11DeviceContext> pD3D11ImmediateContext;
                RefPtr<ID3D11RenderTargetView> pD3D11RenderTargetView;
                // 当前缓冲区像素大小
                Size PixelSize;
                HWND hWnd;

                D3D11DrawContext()
                {
                }

                HRESULT Init(_In_ HWND _hWnd);

            public:
                static std::unique_ptr<D3D11DrawContext> CreateDrawTarget(_In_ HWND _hWnd);

                HRESULT __YYAPI BeginDraw() override;

                void EndDraw() override;

                void __YYAPI DrawLine(
                    Brush _oBrush,
                    Point _oPoint0,
                    Point _oPoint1,
                    float iLineWidth) override;

                HRESULT __YYAPI SetPixelSize(
                    _In_ const Size& _PixelSize) override;

                void __YYAPI FillRectangle(
                    _In_ Brush _oBrush,
                    _In_ const Rect& _oRect) override;

                void
                __YYAPI
                DrawString(
                    _In_ uString _szText,
                    _In_ const Font& _FontInfo,
                    _In_ Color _crTextColor,
                    _In_ const Rect& _LayoutRect,
                    _In_ ContentAlignStyle _fTextAlign) override;

                ///////////////////////////////////////////////////////////////
                // IDWriteTextRenderer

                HRESULT STDMETHODCALLTYPE QueryInterface(
                    /* [in] */ REFIID riid,
                    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;

                ULONG STDMETHODCALLTYPE AddRef() override;

                ULONG STDMETHODCALLTYPE Release() override;

                HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(
                    _In_opt_ void* clientDrawingContext,
                    _Out_ BOOL* isDisabled
                    ) override;

                HRESULT STDMETHODCALLTYPE GetCurrentTransform(
                    _In_opt_ void* clientDrawingContext,
                    _Out_ DWRITE_MATRIX* transform
                    ) override;

                HRESULT STDMETHODCALLTYPE GetPixelsPerDip(
                    _In_opt_ void* clientDrawingContext,
                    _Out_ FLOAT* pixelsPerDip
                    ) override;

                HRESULT STDMETHODCALLTYPE DrawGlyphRun(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    DWRITE_MEASURING_MODE measuringMode,
                    _In_ DWRITE_GLYPH_RUN const* glyphRun,
                    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawUnderline(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    _In_ DWRITE_UNDERLINE const* underline,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawStrikethrough(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawInlineObject(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT originX,
                    FLOAT originY,
                    _In_ IDWriteInlineObject* inlineObject,
                    BOOL isSideways,
                    BOOL isRightToLeft,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawGlyphRun(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
                    DWRITE_MEASURING_MODE measuringMode,
                    _In_ DWRITE_GLYPH_RUN const* glyphRun,
                    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawUnderline(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
                    _In_ DWRITE_UNDERLINE const* underline,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawStrikethrough(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT baselineOriginX,
                    FLOAT baselineOriginY,
                    DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
                    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
                    _In_opt_ IUnknown* clientDrawingEffect) override;

                HRESULT STDMETHODCALLTYPE DrawInlineObject(
                    _In_opt_ void* clientDrawingContext,
                    FLOAT originX,
                    FLOAT originY,
                    DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
                    _In_ IDWriteInlineObject* inlineObject,
                    BOOL isSideways,
                    BOOL isRightToLeft,
                    _In_opt_ IUnknown* clientDrawingEffect) override;
            };

        } // namespace Graphics
    } // namespace Media
    using namespace YY::Media::Graphics;
} // namespace YY

#pragma pack(pop)
