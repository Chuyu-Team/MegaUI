#pragma once
//#include <functional>

// #include <optional> 

#include <Media/Brushes/Brush.h>
#include <Media/Point.h>
#include <Base/Containers/Array.h>
#include <Base/Sync/InterlockedSingleLinkedList.h>
#include <Base/Containers/ArrayView.h>
#include <Media/Graphics/DrawContext.h>
#include <Base/Memory/UniquePtr.h>
#include <Base/Containers/Optional.h>
#include <Base/Threading/TaskRunner.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            typedef YY::Base::Containers::Array<byte_t, AllocPolicy::COW> DrawCommand;
            // typedef std::function<HRESULT(DrawContext** _ppTargetDrawContext)> CreateTargetDrawContext;

            class DrawAsyncCommandContext : public DrawContext
            {
            private:
                UniquePtr<DrawContext> pTargetDrawContext;

                struct DrawCommandEntry : public InterlockedSingleLinkedEntryBase<DrawCommandEntry>
                {
                    DrawCommand oCommandData;
                    Size PixelSize;
                    Optional<Rect> oPaint;
                };

                InterlockedSingleLinkedList<DrawCommandEntry> oCommandList;
                DrawCommandEntry* pCurrentCommandEntry;
                Size PixelSize;
                RefPtr<SequencedTaskRunner> pSequencedTaskRunner;

                // 如果线程退出或者对象准备销毁时将为 true
                volatile bool bCancel;

                DrawAsyncCommandContext();

            public:
                ~DrawAsyncCommandContext();

                static HRESULT __YYAPI CreateDrawTarget(
                    _In_ UniquePtr<DrawContext>&& _pTargetDrawContext,
                    _Outptr_ DrawContext** _ppDrawContext
                );

                HRESULT __YYAPI BeginDraw(_In_opt_ const Rect* _pPaint) override;

                HRESULT __YYAPI EndDraw() override;

                Size __YYAPI GetPixelSize() override;

                HRESULT __YYAPI SetPixelSize(
                    _In_ const Size& _PixelSize) override;

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
                static void FreeDrawingCommand(DrawCommandEntry* _pEntry);

                HRESULT RunLoop();
            };
        }
    }

    using namespace YY::Media::Graphics;
} // namespace YY

#pragma pack(pop)
