#pragma once
#include <Windows.h>
#include <atlcomcli.h>
#include <GdiPlus.h>

#include <Base/YY.h>
#include <Media/Font.h>
#include <Base/Memory/RefPtr.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Containers/Array.h>
#include <Media/Graphics/GDIPlus/GDIPlusHelper.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class GDIPlusDrawContext : public DrawContext
            {
            private:
                HWND hWnd;
                SIZE oPixelSize;
                DWORD uThreadId;
                HDC hDC;
                // 无效区域
                Rect oPaint;
                GdiplusStatic<Gdiplus::Bitmap> oSurfaceBitmap;
                GdiplusStatic<Gdiplus::Graphics> oSurface;
                Array<Gdiplus::RectF, AllocPolicy::SOO> vecClip;
                bool bFirstPaint;

                GDIPlusDrawContext();

            public:
                ~GDIPlusDrawContext();

                static _Ret_notnull_ DrawContextFactory* __YYAPI GetDrawContextFactory();

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
                
            private:
                _Ret_notnull_ Gdiplus::Graphics* __YYAPI GetSurface();

                HRESULT __YYAPI UpdateTargetPixelSize(bool _bCopyToNewTarget);

                static RefPtr<GdiplusRef<Gdiplus::Brush>> __YYAPI GetNativeBrush(_In_ Brush _oBrush);

                static RefPtr<GdiplusRef<Gdiplus::Pen>> __YYAPI GetNativePen(_In_ Pen _oPen);
            };
        }
    } // namespace Media
} // namespace YY

#pragma pack(pop)
