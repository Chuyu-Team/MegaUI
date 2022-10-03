#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "..\..\base\MegaUITypeInt.h"
#include "..\Render.h"
#include "..\..\base\alloc.h"
#include <MegaUI/base/Font.h>
#include <MegaUI/base/ComPtr.h>

#pragma pack(push, __MEGA_UI_PACKING)

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")

namespace YY
{
    namespace MegaUI
    {
        // D2D本身内部就是双缓冲……,此版本适用于Windows Vista SP2以及Windows 7
        class D2D1_0Render : public Render
        {
        private:
            HWND hWnd;
            ID2D1Factory* pD2DFactory;
            ID2D1HwndRenderTarget* pRenderTarget;
            D2D1_SIZE_U PixelSize;
            IDWriteFactory* pDWriteFactory;

        public:
            D2D1_0Render(HWND _hWnd, ID2D1Factory* _pD2DFactory, ID2D1HwndRenderTarget* _pRenderTarget, const D2D1_SIZE_U& _PixelSize)
                : hWnd(_hWnd)
                , pD2DFactory(_pD2DFactory)
                , pRenderTarget(_pRenderTarget)
                , PixelSize(_PixelSize)
                , pDWriteFactory(nullptr)
            {
            }

            D2D1_0Render(const D2D1_0Render&) = delete;

            ~D2D1_0Render()
            {
                if (pRenderTarget)
                    pRenderTarget->Release();

                if (pD2DFactory)
                    pD2DFactory->Release();
                
                if (pDWriteFactory)
                    pDWriteFactory->Release();
            }

            void __MEGA_UI_API operator=(const D2D1_0Render&) = delete;

            HRESULT __MEGA_UI_API InitializeRenderTarget()
            {
                if (!pRenderTarget)
                {
                    auto _hr = pD2DFactory->CreateHwndRenderTarget(
                        D2D1::RenderTargetProperties(),
                        D2D1::HwndRenderTargetProperties(hWnd, PixelSize),
                        &pRenderTarget);

                    if (FAILED(_hr))
                        return _hr;

                    if (!pRenderTarget)
                    {
                        return E_UNEXPECTED;
                    }
                }


                return S_OK;
            }

            static HRESULT __MEGA_UI_API CreateRender(_In_ HWND _hWnd, _Outptr_ Render** _ppRender)
            {
                if (_ppRender == nullptr)
                    return E_INVALIDARG;
                    
                *_ppRender = nullptr;

                if (_hWnd == NULL)
                    return E_INVALIDARG;

                RECT _ClientRect;
                if (!GetClientRect(_hWnd, &_ClientRect))
                    return E_UNEXPECTED;

                HRESULT _hr = S_OK;
                ID2D1Factory* _pD2DFactory = nullptr;
                ID2D1HwndRenderTarget* _pRenderTarget = nullptr;

                do
                {
                    _hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
                    if (FAILED(_hr))
                        break;

                    if (!_pD2DFactory)
                    {
                        _hr =  E_UNEXPECTED;
                        break;
                    }

                    D2D1_SIZE_U _PixelSize = D2D1::SizeU(
                        static_cast<UINT>(_ClientRect.right - _ClientRect.left),
                        static_cast<UINT>(_ClientRect.bottom - _ClientRect.top));


                    _hr = _pD2DFactory->CreateHwndRenderTarget(
                        D2D1::RenderTargetProperties(),
                        D2D1::HwndRenderTargetProperties(_hWnd, _PixelSize),
                        &_pRenderTarget);

                    if (FAILED(_hr))
                        break;
                        
                    if (!_pRenderTarget)
                    {
                        _hr = E_UNEXPECTED;
                        break;
                    }

                    auto _pD2DRender = HNew<D2D1_0Render>(_hWnd, _pD2DFactory, _pRenderTarget, _PixelSize);
                    if (!_pD2DRender)
                    {
                        _hr = E_OUTOFMEMORY;
                        break;
                    }

                    *_ppRender = _pD2DRender;
                    return S_OK;
                } while (false);

                if (_pRenderTarget)
                    _pRenderTarget->Release();

                if (_pD2DFactory)
                    _pD2DFactory->Release();

                return _hr;
            }

            virtual HRESULT __MEGA_UI_API BeginDraw(Rect* _pNeedPaintRect) override
            {
                if (_pNeedPaintRect)
                {
                    _pNeedPaintRect->Left = 0;
                    _pNeedPaintRect->Top = 0;
                    _pNeedPaintRect->Right = (float)PixelSize.width;
                    _pNeedPaintRect->Bottom = (float)PixelSize.height;
                }
                auto _hr = InitializeRenderTarget();
                if (FAILED(_hr))
                    return _hr;
                
                pRenderTarget->BeginDraw();
                return S_OK;
            }

            virtual HRESULT __MEGA_UI_API EndDraw() override
            {
                if (!pRenderTarget)
                    return E_UNEXPECTED;

                HRESULT _hr = _hr = pRenderTarget->EndDraw();
 
                // 需要重新呈现
                if (_hr == D2DERR_RECREATE_TARGET)
                {
                    _hr = S_OK;
                    pRenderTarget->Release();
                    pRenderTarget = nullptr;
                }

                return _hr;
            }

            virtual void __MEGA_UI_API PushAxisAlignedClip(
                _In_ const Rect& _ClipRect) override
            {
                pRenderTarget->PushAxisAlignedClip(_ClipRect, D2D1_ANTIALIAS_MODE_ALIASED);
            }

            virtual void __MEGA_UI_API PopAxisAlignedClip() override
            {
                pRenderTarget->PopAxisAlignedClip();
            }

            virtual void __MEGA_UI_API FillRectangle(
                _In_ const Rect& _Rect,
                _In_ ID2D1Brush* _pBrush) override
            {
                pRenderTarget->FillRectangle(_Rect, _pBrush);
            }

            virtual HRESULT __MEGA_UI_API SetPixelSize(
                _In_ const D2D1_SIZE_U& _PixelSize) override
            {
                if (_PixelSize.width == PixelSize.width && _PixelSize.height == PixelSize.height)
                    return S_OK;

                PixelSize.width = _PixelSize.width;
                PixelSize.height = _PixelSize.height;
                if (pRenderTarget)
                    pRenderTarget->Resize(PixelSize);

                return S_OK;
            }

            D2D1_SIZE_U __MEGA_UI_API GetPixelSize()
            {
                return PixelSize;
            }

            virtual
            HRESULT
            __MEGA_UI_API
            CreateSolidColorBrush(
                Color _Color,
                _Outptr_ ID2D1SolidColorBrush** _ppSolidColorBrush) override
            {
                return pRenderTarget->CreateSolidColorBrush(_Color, _ppSolidColorBrush);
            }
            
            _Ret_maybenull_ IDWriteFactory* __MEGA_UI_API TryGetDWriteFactory()
            {
                if (pDWriteFactory)
                    return pDWriteFactory;

                auto _hr = DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&pDWriteFactory));

                return pDWriteFactory;
            }

            virtual
            HRESULT
            __MEGA_UI_API CreateTextFormat(
                _In_z_ uchar_t const* _szFontFamilyName,
                _In_opt_ IDWriteFontCollection* _pFontCollection,
                _In_ const Font& _FontInfo,
                _In_z_ uchar_t const* _szLocaleName,
                _COM_Outptr_ IDWriteTextFormat** _ppTextFormat
                ) override
            {
                if (!_ppTextFormat)
                    return E_INVALIDARG;
                *_ppTextFormat = nullptr;

                auto _pDWriteFactory = TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                DWRITE_FONT_STYLE _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
                if (_FontInfo.fStyle & FontStyle::Italic)
                    _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;

                return _pDWriteFactory->CreateTextFormat(
                    _szFontFamilyName,
                    _pFontCollection,
                    DWRITE_FONT_WEIGHT(_FontInfo.uWeight),
                    _eFontStyle,
                    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
                    _FontInfo.iSize,
                    _szLocaleName,
                    _ppTextFormat);
            }

            virtual
            HRESULT
            __MEGA_UI_API CreateTextLayout(
                _In_ uStringView _szText,
                _In_ IDWriteTextFormat* _pTextFormat,
                _In_ Size _MaxBound,
                _COM_Outptr_ IDWriteTextLayout** _ppTextLayout
                ) override
            {
                if (!_ppTextLayout)
                    return E_INVALIDARG;

                *_ppTextLayout = nullptr;
                
                if (_szText.GetSize() > uint32_max)
                    return E_UNEXPECTED;

                auto _pDWriteFactory = TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                return _pDWriteFactory->CreateTextLayout(
                    _szText.GetConstString(),
                    (UINT32)_szText.GetSize(),
                    _pTextFormat,
                    _MaxBound.Width,
                    _MaxBound.Height,
                    _ppTextLayout);
            }

            virtual
            void
            __MEGA_UI_API DrawTextLayout(
                _In_ Point _Origin,
                _In_ IDWriteTextLayout* _pTextLayout,
                _In_ ID2D1Brush* _pDefaultFillBrush,
                _In_ D2D1_DRAW_TEXT_OPTIONS _eOptions
                ) override
            {
                pRenderTarget->DrawTextLayout(
                    _Origin,
                    _pTextLayout,
                    _pDefaultFillBrush,
                    _eOptions);
            }
            
            virtual
            void
            __MEGA_UI_API
            DrawString(
                _In_ uStringView _szText,
                _In_ const Font& _FontInfo,
                _In_ const Rect& _LayoutRect,
                _In_ int32_t _fTextAlign
                ) override
            {
                if (_szText.GetSize() == 0 || _LayoutRect.IsEmpty() || pRenderTarget == nullptr)
                    return;

                if (_szText.GetSize() > uint32_max)
                    throw Exception();

                auto _pDWriteFactory = TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return;

                DWRITE_FONT_STYLE _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
                if (_FontInfo.fStyle & FontStyle::Italic)
                    _eFontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC;

                ComPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = _pDWriteFactory->CreateTextFormat(
                    _FontInfo.szFace,
                    nullptr,
                    DWRITE_FONT_WEIGHT(_FontInfo.uWeight),
                    _eFontStyle,
                    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL,
                    _FontInfo.iSize,
                    _S(""),
                    &_pTextFormat);

                if (FAILED(_hr))
                    return;
                DWRITE_TEXT_ALIGNMENT _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
                if (_fTextAlign & ContentAlign::Right)
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING;
                }
                else if (_fTextAlign & ContentAlign::Center)
                {
                    _TextAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER;
                }

                _pTextFormat->SetTextAlignment(_TextAlignment);

                DWRITE_PARAGRAPH_ALIGNMENT _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
                if (_fTextAlign & ContentAlign::Bottom)
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR;
                }
                else if (_fTextAlign & ContentAlign::Middle)
                {
                    _ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
                }
                _pTextFormat->SetParagraphAlignment(_ParagraphAlignment);

                auto _WordWrapping = (_fTextAlign & ContentAlign::Wrap) ? DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP;
                _pTextFormat->SetWordWrapping(_WordWrapping);
                
                if (_fTextAlign & ContentAlign::EndEllipsis)
                {
                    ComPtr<IDWriteInlineObject> pDWriteInlineObject;
                    auto _hr = _pDWriteFactory->CreateEllipsisTrimmingSign(_pTextFormat, &pDWriteInlineObject);
                    if (SUCCEEDED(_hr))
                    {
                        DWRITE_TRIMMING trim;
                        trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
                        trim.delimiter = 1;
                        trim.delimiterCount = 3;
                        _pTextFormat->SetTrimming(&trim, pDWriteInlineObject);
                    }
                }

                ComPtr<IDWriteTextLayout> _pTextLayout;
                _hr = _pDWriteFactory->CreateTextLayout(
                    _szText.GetConstString(),
                    (UINT32)_szText.GetSize(),
                    _pTextFormat,
                    _LayoutRect.GetWidth(),
                    _LayoutRect.GetHeight(),
                    &_pTextLayout);

                if (FAILED(_hr))
                    return;

                if (_FontInfo.fStyle & FontStyle::Underline)
                    _pTextLayout->SetUnderline(TRUE, DWRITE_TEXT_RANGE {0, (UINT32)_szText.GetSize()});

                if (_FontInfo.fStyle & FontStyle::StrikeOut)
                    _pTextLayout->SetStrikethrough(TRUE, DWRITE_TEXT_RANGE {0, (UINT32)_szText.GetSize()});

                ComPtr<ID2D1SolidColorBrush> _pDefaultFillBrush;
                _hr = pRenderTarget->CreateSolidColorBrush(_FontInfo.Color, &_pDefaultFillBrush);
                if (FAILED(_hr))
                    return;

                pRenderTarget->DrawTextLayout(
                    _LayoutRect.GetPoint(),
                    _pTextLayout,
                    _pDefaultFillBrush,
                    D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);

            }
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
