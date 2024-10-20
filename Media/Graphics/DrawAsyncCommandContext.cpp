#include "pch.h"
#include "DrawAsyncCommandContext.h"

// #include <process.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

// #pragma comment(lib, "Synchronization.lib")

#define __DRAWING_COMMAND_ID_TABLE(_APPLY, _pCommandData)                                   \
    _APPLY(PushAxisAlignedClip, _pCommandData->oClipRect)                                   \
    _APPLY(PopAxisAlignedClip)                                                              \
    _APPLY(DrawLine, _pCommandData->oPoint0, _pCommandData->oPoint1, _pCommandData->oPen)   \
    _APPLY(DrawRectangle, _pCommandData->oRect, _pCommandData->oPen, _pCommandData->oBrush) \
    _APPLY(DrawString, _pCommandData->szText, _pCommandData->FontInfo, _pCommandData->oBrush, _pCommandData->LayoutRect, _pCommandData->fTextAlign) \
    _APPLY(DrawString2, _pCommandData->oOrigin, _pCommandData->pTextLayout, _pCommandData->oBrush)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            enum class CommandType : uint32_t
            {
                Unknow,
                PushAxisAlignedClip,
                PopAxisAlignedClip,
                DrawLine,
                DrawRectangle,
                DrawString,
                DrawString2,
            };

            template<CommandType _eType>
            struct DrawCommandData;

            template<>
            struct DrawCommandData<CommandType::Unknow>
            {
                CommandType eCommandType;
            };

            template<>
            struct DrawCommandData<CommandType::PushAxisAlignedClip>
            {
                CommandType eCommandType;
                Rect oClipRect;
            };

            template<>
            struct DrawCommandData<CommandType::PopAxisAlignedClip>
            {
                CommandType eCommandType;
            };

            template<>
            struct DrawCommandData<CommandType::DrawLine>
            {
                CommandType eCommandType;
                Point oPoint0;
                Point oPoint1;
                Pen oPen;
            };

            template<>
            struct DrawCommandData<CommandType::DrawRectangle>
            {
                CommandType eCommandType;
                Rect oRect;
                Pen oPen;
                Brush oBrush;  
            };

            template<>
            struct DrawCommandData<CommandType::DrawString>
            {
                CommandType eCommandType;
                uString szText;
                Font FontInfo;
                Brush oBrush;
                Rect LayoutRect;
                ContentAlignStyle fTextAlign;
            };

            template<>
            struct DrawCommandData<CommandType::DrawString2>
            {
                CommandType eCommandType;
                Point oOrigin;
                RefPtr<IDWriteTextLayout> pTextLayout;
                Brush oBrush;
            };

            template<CommandType _eType, class... Args>
            static HRESULT RecordDrawCommand(DrawCommand& _oCommandData, Args... _args)
            {
                using CommandType = DrawCommandData<_eType>;

                const auto _uOldSize = _oCommandData.GetSize();
                auto _pBuffer = _oCommandData.LockBufferAndSetSize(_uOldSize + sizeof(CommandType));
                if (!_pBuffer)
                    return E_OUTOFMEMORY;

                auto _pCommandData = (CommandType*)(_pBuffer + _uOldSize);
                new (_pCommandData) CommandType {_eType, _args...};

                _oCommandData.UnlockBuffer();

                return S_OK;
            }

            DrawAsyncCommandContext::DrawAsyncCommandContext()
                : pTargetDrawContext()
                , pCurrentCommandEntry(nullptr)
                , PixelSize {}
                , bCancel(false)
            {
            }

            DrawAsyncCommandContext::~DrawAsyncCommandContext()
            {
                bCancel = true;
                FreeDrawingCommand(pCurrentCommandEntry);
                pCurrentCommandEntry = nullptr;

                if (pSequencedTaskRunner)
                {
                    // TODO:尽可能减少同步
                    pSequencedTaskRunner->SendTask(
                        [&]()
                        {

                        });
                }

                FreeDrawingCommand(oCommandList.Flush());
                pTargetDrawContext = nullptr;                
            }

            HRESULT DrawAsyncCommandContext::CreateDrawTarget(
                UniquePtr<DrawContext>&& _pTargetDrawContext,
                DrawContext** _ppDrawContext)
            {
                if (!_ppDrawContext)
                    return E_INVALIDARG;
                *_ppDrawContext = nullptr;

                if (!_pTargetDrawContext)
                    return E_INVALIDARG;
                UniquePtr<DrawAsyncCommandContext> _pDrawAsyncCommandContext(new (std::nothrow) DrawAsyncCommandContext());
                if (!_pDrawAsyncCommandContext)
                    return E_OUTOFMEMORY;
                 
                _pDrawAsyncCommandContext->pTargetDrawContext.Attach(_pTargetDrawContext.Detach());
                *_ppDrawContext = _pDrawAsyncCommandContext.Detach();
                return S_OK;
            }

            HRESULT DrawAsyncCommandContext::BeginDraw(const Rect* _pPaint)
            {
                if (bCancel)
                    return E_ABORT;

                // 只能 BeginDraw - EndDraw必须成对
                if (pCurrentCommandEntry)
                    abort();

                if (!pSequencedTaskRunner)
                {
                    pSequencedTaskRunner = SequencedTaskRunner::Create();
                    if (!pSequencedTaskRunner)
                        return E_OUTOFMEMORY;
                }

                pCurrentCommandEntry = new(std::nothrow) DrawCommandEntry;
                if (!pCurrentCommandEntry)
                    return E_OUTOFMEMORY;

                pCurrentCommandEntry->PixelSize = PixelSize;
                if (_pPaint)
                {
                    pCurrentCommandEntry->oPaint = *_pPaint;
                }
                return S_OK;
            }

            HRESULT DrawAsyncCommandContext::EndDraw()
            {
                if (!pCurrentCommandEntry)
                {
                    abort();
                    return E_UNEXPECTED;
                }

                if (bCancel)
                {
                    FreeDrawingCommand(pCurrentCommandEntry);
                    pCurrentCommandEntry = nullptr;
                    return E_ABORT;
                }
                oCommandList.Push(pCurrentCommandEntry);
                pCurrentCommandEntry = nullptr;

                pSequencedTaskRunner->PostTask(
                    [](void* _pUserData)
                    {
                        auto _pAsyncCommandContext = (DrawAsyncCommandContext*)_pUserData;
                        _pAsyncCommandContext->RunLoop();
                    }, this);

                // 唤醒异步绘制队列
                return S_OK;
            }

            Size DrawAsyncCommandContext::GetPixelSize()
            {
                return PixelSize;
            }

            HRESULT DrawAsyncCommandContext::SetPixelSize(const Size& _PixelSize)
            {
                PixelSize = _PixelSize;
                return S_OK;
            }

            void DrawAsyncCommandContext::PushAxisAlignedClip(const Rect& _ClipRect)
            {
                if (!pCurrentCommandEntry)
                    abort();
                
                RecordDrawCommand<CommandType::PushAxisAlignedClip>(pCurrentCommandEntry->oCommandData, _ClipRect);
            }

            void DrawAsyncCommandContext::PopAxisAlignedClip()
            {
                if (!pCurrentCommandEntry)
                    abort();

                RecordDrawCommand<CommandType::PopAxisAlignedClip>(pCurrentCommandEntry->oCommandData);
            }

            void DrawAsyncCommandContext::DrawLine(
                Point _oPoint0,
                Point _oPoint1,
                Pen _oPen
                )
            {
                if (!pCurrentCommandEntry)
                    abort();

                RecordDrawCommand<CommandType::DrawLine>(pCurrentCommandEntry->oCommandData, _oPoint0, _oPoint1, _oPen);
            }

            void DrawAsyncCommandContext::DrawRectangle(
                _In_ const Rect& _oRect,
                _In_opt_ Pen _oPen,
                _In_opt_ Brush _oBrush
                )
            {
                if (!pCurrentCommandEntry)
                    abort();

                RecordDrawCommand<CommandType::DrawRectangle>(pCurrentCommandEntry->oCommandData, _oRect, _oPen, _oBrush);
            }

            void DrawAsyncCommandContext::DrawString(uString _szText, const Font& _FontInfo, Brush _oBrush, const Rect& _LayoutRect, ContentAlignStyle _fTextAlign)
            {
                if (!pCurrentCommandEntry)
                    abort();

                RecordDrawCommand<CommandType::DrawString>(pCurrentCommandEntry->oCommandData, _szText, _FontInfo, _oBrush, _LayoutRect, _fTextAlign);
            }

            void DrawAsyncCommandContext::MeasureString(uStringView _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign, Size* _pExtent)
            {
                pTargetDrawContext->MeasureString(_szText, _FontInfo, _LayoutSize, _fTextAlign, _pExtent);
            }

            RefPtr<IDWriteTextLayout>__YYAPI DrawAsyncCommandContext::CreateTextLayout(uString _szText, const Font& _FontInfo, const Size& _LayoutSize, ContentAlignStyle _fTextAlign)
            {
                return pTargetDrawContext->CreateTextLayout(_szText, _FontInfo, _LayoutSize, _fTextAlign);
            }

            void __YYAPI DrawAsyncCommandContext::DrawString2(const Point& _Origin, RefPtr<IDWriteTextLayout> _pTextLayout, Brush _oBrush)
            {
                if (!pCurrentCommandEntry)
                    abort();

                RecordDrawCommand<CommandType::DrawString2>(pCurrentCommandEntry->oCommandData, _Origin, _pTextLayout, _oBrush);
            }

            void DrawAsyncCommandContext::FreeDrawingCommand(DrawCommandEntry* _pEntry)
            {
                while (_pEntry)
                {
                    auto _pNext = _pEntry->pNext;

                    YY::Base::Containers::ArrayView<const byte_t> _CommandDataView(_pEntry->oCommandData.GetData(), _pEntry->oCommandData.GetSize());

                    while (_CommandDataView.GetSize() != 0)
                    {
                        if (_CommandDataView.GetSize() < sizeof(DrawCommandData<CommandType::Unknow>))
                        {
                            abort();
                        }

#define __APPLY_FREE_DRAWING_COMMAND_ID(_Id, ...)                       \
    case CommandType::_Id:                                              \
    {                                                                   \
        using DrawCommandDataType = DrawCommandData<CommandType::_Id>;  \
        if (_CommandDataView.GetSize() < sizeof(DrawCommandDataType))   \
            abort();                                                    \
        auto _pCommandData = (DrawCommandDataType*)_pUnknowCommandData; \
        _pCommandData->~DrawCommandDataType();                          \
        _CommandDataView.Slice(sizeof(DrawCommandDataType));            \
        break;                                                          \
    }

                        auto _pUnknowCommandData = (DrawCommandData<CommandType::Unknow>*)_CommandDataView.GetData();
                        switch (_pUnknowCommandData->eCommandType)
                        {
                            __DRAWING_COMMAND_ID_TABLE(__APPLY_FREE_DRAWING_COMMAND_ID, _pCommandData)
                        default:
                            // 未知的Id，理论上不应该出现，所以立即终止程序
                            abort();
                            break;
                        }
                    }

                    delete _pEntry;
                    _pEntry = _pNext;
                }
            }
            
            HRESULT DrawAsyncCommandContext::RunLoop()
            {
                HRESULT _hr = S_OK;
                if (bCancel)
                    return S_OK;

                for (;;)
                {
                    auto _pLastPaint = oCommandList.Flush();
                    if (!_pLastPaint)
                        break;

                    // 搜索顶层或者全屏刷新的命令
                    DrawCommandEntry* _pFirstPaint = nullptr;
                    for (auto _pItem = _pLastPaint;;)
                    {
                        auto _pNext = _pItem->pNext;
                        _pItem->pNext = _pFirstPaint;
                        _pFirstPaint = _pItem;
                        _pItem = _pNext;
                        
                        if (_pItem == nullptr || _pFirstPaint->oPaint.HasValue() == false)
                        {
                            _pLastPaint->pNext = _pItem;
                            break;
                        }
                    }

                    if (!_pLastPaint->PixelSize.IsEmpty())
                    {
                        pTargetDrawContext->SetPixelSize(_pLastPaint->PixelSize);
                    }

                    for (auto _pItem = _pFirstPaint; _pItem; _pItem = _pItem->pNext)
                    {
                        if (bCancel)
                            break;

                        auto _hr = pTargetDrawContext->BeginDraw(_pItem->oPaint.GetValuePtr());

                        pTargetDrawContext->PushAxisAlignedClip(Rect(0, 0, _pItem->PixelSize.Width, _pItem->PixelSize.Height));

                        YY::Base::Containers::ArrayView<const byte_t> _CommandDataView(_pItem->oCommandData.GetData(), _pItem->oCommandData.GetSize());

                        while (_CommandDataView.GetSize() != 0)
                        {
                            if (bCancel)
                                break;

                            if (_CommandDataView.GetSize() < sizeof(DrawCommandData<CommandType::Unknow>))
                            {
                                abort();
                            }

#define __APPLY_DRAWING_COMMAND_ID(_Id, ...)                            \
    case CommandType::_Id:                                              \
    {                                                                   \
        using DrawCommandDataType = DrawCommandData<CommandType::_Id>;  \
        if (_CommandDataView.GetSize() < sizeof(DrawCommandDataType))   \
            abort();                                                    \
        auto _pCommandData = (DrawCommandDataType*)_pUnknowCommandData; \
        pTargetDrawContext->_Id(__VA_ARGS__);                           \
        _CommandDataView.Slice(sizeof(DrawCommandDataType));            \
        break;                                                          \
    }

                            auto _pUnknowCommandData = (DrawCommandData<CommandType::Unknow>*)_CommandDataView.GetData();
                            switch (_pUnknowCommandData->eCommandType)
                            {
                                __DRAWING_COMMAND_ID_TABLE(__APPLY_DRAWING_COMMAND_ID, _pCommandData);
                            default:
                                // 未知的Id，理论上不应该出现，所以立即终止程序
                                abort();
                                break;
                            }
                        }

                        pTargetDrawContext->PopAxisAlignedClip();
                        _hr = pTargetDrawContext->EndDraw();

                        if (_pItem == _pLastPaint)
                            break;
                    }

                    // 绘制期间仅可能不做非绘制相关，减少呈现延迟
                    // 因此将一些清理工作放结束
                    FreeDrawingCommand(_pFirstPaint);
                }
                return _hr;
            }
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
