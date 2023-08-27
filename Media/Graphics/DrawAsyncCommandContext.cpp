#include "pch.h"
#include "DrawAsyncCommandContext.h"

#include <process.h>

#pragma warning(disable : 28251)

#pragma comment(lib, "Synchronization.lib")

#define __DRAWING_COMMAND_ID_TABLE(_APPLY, _pCommandData)                                   \
    _APPLY(PushAxisAlignedClip, _pCommandData->oClipRect)                                   \
    _APPLY(PopAxisAlignedClip)                                                              \
    _APPLY(DrawLine, _pCommandData->oPoint0, _pCommandData->oPoint1, _pCommandData->oPen)   \
    _APPLY(DrawRectangle, _pCommandData->oRect, _pCommandData->oPen, _pCommandData->oBrush) \
    _APPLY(DrawString, _pCommandData->szText, _pCommandData->FontInfo, _pCommandData->oBrush, _pCommandData->LayoutRect, _pCommandData->fTextAlign)

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
                , hThread(NULL)
                , bCancel(false)
            {
            }

            DrawAsyncCommandContext::~DrawAsyncCommandContext()
            {
                if (hThread)
                {
                    bCancel = true;
                    WakeByAddressSingle(&oCommandList.pHead);
                    WaitForSingleObject(hThread, INFINITE);
                    CloseHandle(hThread);
                    hThread = NULL;
                }
                FreeDrawingCommand(pCurrentCommandEntry);
                pCurrentCommandEntry = nullptr;

                FreeDrawingCommand(oCommandList.Flush());

                pTargetDrawContext = nullptr;
            }

            HRESULT DrawAsyncCommandContext::CreateDrawTarget(
                UniquePtr<DrawContext> _pTargetDrawContext,
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
                 
                auto _hThread = (HANDLE)_beginthreadex(
                    nullptr, 0,
                    [](void* _pUserData) -> unsigned
                    {
                        auto _pDrawContext = (DrawAsyncCommandContext*)_pUserData;        
                        return _pDrawContext->RunLoop();
                    },
                    _pDrawAsyncCommandContext.Get(),
                    0,
                    nullptr);
                if (_hThread == NULL)
                {
                    return __HRESULT_FROM_WIN32(GetLastError());
                }

                _pDrawAsyncCommandContext->hThread = _hThread;
                _pDrawAsyncCommandContext->pTargetDrawContext.Attach(_pTargetDrawContext.Detach());
                *_ppDrawContext = _pDrawAsyncCommandContext.Detach();
                return S_OK;
            }

            HRESULT DrawAsyncCommandContext::BeginDraw(const Rect* _pPaint)
            {
                if (bCancel)
                    return E_ABORT;

                if (hThread == NULL)
                {
                    /*hThread = (HANDLE)_beginthreadex(
                        nullptr, 0,
                        [](void* _pUserData) -> unsigned
                        {
                            auto _pDrawCommandContext = (DrawAsyncCommandContext*)_pUserData;
                            return _pDrawCommandContext->RunLoop();
                        },
                        this, 0, nullptr);

                    if (!hThread)*/
                    abort();
                }

                // 只能 BeginDraw - EndDraw必须成对
                if (pCurrentCommandEntry)
                    abort();

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
                // 唤醒异步绘制队列
                WakeByAddressSingle(&oCommandList.pHead);

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

            void DrawAsyncCommandContext::FreeDrawingCommand(DrawCommandEntry* _pEntry)
            {
                while (_pEntry)
                {
                    auto _pNext = _pEntry->pNext;

                    YY::Base::Containers::ArrayView<const byte> _CommandDataView(_pEntry->oCommandData.GetData(), _pEntry->oCommandData.GetSize());

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
                    static const void* _pEmptyHeader = nullptr;
                    auto _bRet = WaitOnAddress(&oCommandList.pHead, &_pEmptyHeader, sizeof(_pEmptyHeader), INFINITE);
                    if (bCancel)
                        break;

                    if (!_bRet)
                        continue;

                    auto _pLast = oCommandList.Flush();
                    if (!_pLast)
                        continue;

                    if (!_pLast->PixelSize.IsEmpty())
                        pTargetDrawContext->SetPixelSize(_pLast->PixelSize);

                    pTargetDrawContext->BeginDraw(_pLast->oPaint.GetValuePtr());

                    YY::Base::Containers::ArrayView<const byte> _CommandDataView(_pLast->oCommandData.GetData(), _pLast->oCommandData.GetSize());

                    while (_CommandDataView.GetSize() != 0)
                    {
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

                    pTargetDrawContext->EndDraw();

                    FreeDrawingCommand(_pLast);
                }

                bCancel = true;
                return _hr;
            }
        } // namespace Graphics
    }     // namespace Media
} // namespace YY
