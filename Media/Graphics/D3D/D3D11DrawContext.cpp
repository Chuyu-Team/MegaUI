#include "pch.h"

#include "D3D11DrawContext.h"

#include <DirectXMath.h>

#include <MegaUI/Render/D2D/DWrite/DWriteHelper.h>
#include <HLSL/SolidColorPixelShader.h>
#include <HLSL/VertexShader.h>
#include <Media/Brushes/Brush.h>

#pragma warning(disable : 28251)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            MegaUI::DWriteHelper g_DWrite;

            static DirectX::XMFLOAT3 __YYAPI PointToNdcPoint(Point _oPoint0, Size _oSize)
            {
                /*
                窗口坐标系
                0 ------------- X
                |
                |
                |
                |
                Y

                D3D NDC坐标系
                         Y
                         |
                         |
                         |
                -------- 0 -------- X
                         |
                         |
                         |
                         |
                */

                DirectX::XMFLOAT3 _oNdcPoint((_oPoint0.X / _oSize.Width) * 2 - 1, 1 - (_oPoint0.Y / _oSize.Height) * 2, 0.0f);
                return _oNdcPoint;
            }

            static DirectX::XMFLOAT4 __YYAPI RectToNdcRect(const Rect& _oRect, Size _oSize)
            {
                DirectX::XMFLOAT4 _oNdcRect(
                    (_oRect.Left / _oSize.Width) * 2 - 1,
                    1 - (_oRect.Top / _oSize.Height) * 2,
                    (_oRect.Right / _oSize.Width) * 2 - 1,
                    1 - (_oRect.Bottom / _oSize.Height) * 2);
                return _oNdcRect;
            }

            static DirectX::XMFLOAT4 __YYAPI ColorToD3DColorUnorm(Color _oColor)
            {
                DirectX::XMFLOAT4 _D3DColorUnorm(_oColor.Red / 255.0f, _oColor.Green / 255.0f, _oColor.Blue / 255.0f, _oColor.Alpha / 255.0f);
                return _D3DColorUnorm;
            }
            
            struct VertexInputType
            {
                DirectX::XMFLOAT3 Position;
                DirectX::XMFLOAT4 Color;


                static constexpr const D3D11_INPUT_ELEMENT_DESC StatisInputElementDesc[] =
                {
                        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA},
                        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA},
                };
            };




            HRESULT D3D11DrawContext::Init(HWND _hWnd)
            {
                hWnd = _hWnd;
                RECT _Client;
                if (!::GetClientRect(_hWnd, &_Client))
                    return __HRESULT_FROM_WIN32(GetLastError());

                PixelSize.Width = _Client.right - _Client.left;
                PixelSize.Height = _Client.bottom - _Client.top;

                DXGI_MODE_DESC bufferDesc = {};
                bufferDesc.Width = PixelSize.Width;
                bufferDesc.Height = PixelSize.Height;
                bufferDesc.RefreshRate.Numerator = 60;
                bufferDesc.RefreshRate.Denominator = 1;
                bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

                // Describe our SwapChain
                DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
                swapChainDesc.BufferDesc = bufferDesc;
                swapChainDesc.SampleDesc.Count = 1;
                swapChainDesc.SampleDesc.Quality = 0;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 1;
                swapChainDesc.OutputWindow = _hWnd;
                swapChainDesc.Windowed = TRUE;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
 
#ifdef _DEBUG
                const UINT _uFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
                const UINT _uFlags = 0;
#endif
                constexpr D3D_FEATURE_LEVEL FeatureLevel[] = {D3D_FEATURE_LEVEL_9_1};
                // Create our SwapChain
                auto _hr = D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    _uFlags,
                    FeatureLevel,
                    std::size(FeatureLevel),
                    D3D11_SDK_VERSION,
                    &swapChainDesc,
                    &pSwapChain,
                    &pD3D11Device,
                    nullptr,
                    &pD3D11ImmediateContext);

                if (FAILED(_hr))
                {
                    _hr = D3D11CreateDeviceAndSwapChain(
                        nullptr,
                        D3D_DRIVER_TYPE_WARP,
                        nullptr,
                        _uFlags,
                        FeatureLevel,
                        std::size(FeatureLevel),
                        D3D11_SDK_VERSION,
                        &swapChainDesc,
                        &pSwapChain,
                        &pD3D11Device,
                        nullptr,
                        &pD3D11ImmediateContext);

                    if (FAILED(_hr))
                    {
                        return _hr;
                    }
                }
                
                RefPtr<ID3D11Texture2D> pBackBuffer;
                // Create our BackBuffer
                _hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
                if (FAILED(_hr))
                    return _hr;

                // Create our Render Target
                _hr = pD3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &pD3D11RenderTargetView);
                if (FAILED(_hr))
                    return _hr;

                // Set our Render Target
                pD3D11ImmediateContext->OMSetRenderTargets(1, &pD3D11RenderTargetView, nullptr);

                return S_OK;
            }

            std::unique_ptr<D3D11DrawContext> D3D11DrawContext::CreateDrawTarget(HWND _hWnd)
            {
                std::unique_ptr<D3D11DrawContext> _pDrawContext(new D3D11DrawContext);
                if (_pDrawContext == nullptr)
                    return nullptr;

                auto _hr = _pDrawContext->Init(_hWnd);
                if (FAILED(_hr))
                    return nullptr;

                return _pDrawContext;
            }

            HRESULT D3D11DrawContext::BeginDraw()
            {
                D3D11_VIEWPORT Port = {};
                Port.Width = PixelSize.Width;
                Port.Height = PixelSize.Height;
                Port.MinDepth = 0.0f;
                Port.MaxDepth = 1.0f;
                Port.TopLeftX = 0.0f;
                Port.TopLeftY = 0.0f;

                pD3D11ImmediateContext->RSSetViewports(1, &Port);
                return S_OK;
            }

            void D3D11DrawContext::EndDraw()
            {
                pSwapChain->Present(0, 0);
            }


            HRESULT D3D11DrawContext::SetPixelSize(const Size& _PixelSize)
            {
                if (PixelSize == _PixelSize)
                    return S_OK;

                pD3D11RenderTargetView = nullptr;
                auto _hr = pSwapChain->ResizeBuffers(1, _PixelSize.Width, _PixelSize.Height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
                if (FAILED(_hr))
                    return _hr;

                RefPtr<ID3D11Texture2D> pBackBuffer;
                // Create our BackBuffer
                _hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
                if (FAILED(_hr))
                    return _hr;
                D3D11_TEXTURE2D_DESC Desc;
                pBackBuffer->GetDesc(&Desc);

                // Create our Render Target
                _hr = pD3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &pD3D11RenderTargetView);
                if (FAILED(_hr))
                    return _hr;

                // Set our Render Target
                pD3D11ImmediateContext->OMSetRenderTargets(1, &pD3D11RenderTargetView, nullptr);

                PixelSize = _PixelSize;
                return S_OK;
            }
            
            void D3D11DrawContext::DrawLine(Brush _oBrush, Point _oPoint0, Point _oPoint1, float _iLineWidth)
            {
                RefPtr<ID3D11VertexShader> _pD3D11VertexShader;
                auto _hr = pD3D11Device->CreateVertexShader(g_VertexShader, sizeof(g_VertexShader), nullptr, &_pD3D11VertexShader);
                if (FAILED(_hr))
                    return;
                {
                    RefPtr<ID3D11InputLayout> _pInputLayout;
                    _hr = pD3D11Device->CreateInputLayout(
                        VertexInputType::StatisInputElementDesc,
                        std::size(VertexInputType::StatisInputElementDesc),
                        g_VertexShader,
                        sizeof(g_VertexShader), &_pInputLayout);
                    if (FAILED(_hr))
                        return;

                    pD3D11ImmediateContext->IASetInputLayout(_pInputLayout);
                }

                RefPtr<ID3D11PixelShader> _pPixelShader;
                _hr = pD3D11Device->CreatePixelShader(g_SolidColorPixelShader, sizeof(g_SolidColorPixelShader), nullptr, &_pPixelShader);
                if (FAILED(_hr))
                    return;

                auto _SolidColorBrush = _oBrush.TryCast<SolidColorBrush>();

                auto _SolidColor = _SolidColorBrush.GetColor();



                DirectX::XMFLOAT4 _SolidColorUnorm = ColorToD3DColorUnorm(_SolidColor);

                VertexInputType _VertexPoint[] =
                {
                    { PointToNdcPoint(_oPoint0, PixelSize), _SolidColorUnorm },
                    { PointToNdcPoint(_oPoint1, PixelSize), _SolidColorUnorm },
                };

                static const D3D11_BUFFER_DESC _BufferDesc =
                {
                    sizeof(_VertexPoint),
                    D3D11_USAGE::D3D11_USAGE_DEFAULT,
                    D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
                    0,
                    0,
                    0
                };

                const D3D11_SUBRESOURCE_DATA _InitData = {_VertexPoint};
                RefPtr<ID3D11Buffer> _pVertexPointInputBuffer;

                _hr = pD3D11Device->CreateBuffer(&_BufferDesc, &_InitData, &_pVertexPointInputBuffer);
                if (FAILED(_hr))
                    return;

                UINT Strides = sizeof(_VertexPoint[0]);
                UINT Offsets = 0;
                pD3D11ImmediateContext->IASetVertexBuffers(0, 1, &_pVertexPointInputBuffer, &Strides, &Offsets);

                pD3D11ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);

                pD3D11ImmediateContext->VSSetShader(_pD3D11VertexShader, nullptr, 0);
                pD3D11ImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);

                pD3D11ImmediateContext->Draw(std::size(_VertexPoint), 0);
            }

            void D3D11DrawContext::FillRectangle(
                Brush _oBrush,
                const Rect& _oRect)
            {
                RefPtr<ID3D11VertexShader> _pD3D11VertexShader;
                auto _hr = pD3D11Device->CreateVertexShader(g_VertexShader, sizeof(g_VertexShader), nullptr, &_pD3D11VertexShader);
                if (FAILED(_hr))
                    return;
                {
                    RefPtr<ID3D11InputLayout> _pInputLayout;
                    _hr = pD3D11Device->CreateInputLayout(
                        VertexInputType::StatisInputElementDesc,
                        std::size(VertexInputType::StatisInputElementDesc),
                        g_VertexShader,
                        sizeof(g_VertexShader), &_pInputLayout);
                    if (FAILED(_hr))
                        return;

                    pD3D11ImmediateContext->IASetInputLayout(_pInputLayout);
                }

                RefPtr<ID3D11PixelShader> _pPixelShader;
                _hr = pD3D11Device->CreatePixelShader(g_SolidColorPixelShader, sizeof(g_SolidColorPixelShader), nullptr, &_pPixelShader);
                if (FAILED(_hr))
                    return;

                auto _SolidColorBrush = _oBrush.TryCast<SolidColorBrush>();
                const auto _SolidColorUnorm = ColorToD3DColorUnorm(_SolidColorBrush.GetColor());
                const auto _NdcRect = RectToNdcRect(_oRect, PixelSize);

                const VertexInputType _VertexPoint[] =
                {
                    { DirectX::XMFLOAT3(_NdcRect.x, _NdcRect.y, 0.0f), _SolidColorUnorm },
                    { DirectX::XMFLOAT3(_NdcRect.z, _NdcRect.y, 0.0f), _SolidColorUnorm },
                    { DirectX::XMFLOAT3(_NdcRect.x, _NdcRect.w, 0.0f), _SolidColorUnorm },
                    { DirectX::XMFLOAT3(_NdcRect.z, _NdcRect.w, 0.0f), _SolidColorUnorm },
                };

                static const D3D11_BUFFER_DESC _BufferDesc =
                {
                    sizeof(_VertexPoint),
                    D3D11_USAGE::D3D11_USAGE_DEFAULT,
                    D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
                    0,
                    0,
                    0
                };

                const D3D11_SUBRESOURCE_DATA _InitData = {_VertexPoint};
                RefPtr<ID3D11Buffer> _pVertexPointInputBuffer;

                _hr = pD3D11Device->CreateBuffer(&_BufferDesc, &_InitData, &_pVertexPointInputBuffer);
                if (FAILED(_hr))
                    return;

                UINT Strides = sizeof(_VertexPoint[0]);
                UINT Offsets = 0;
                pD3D11ImmediateContext->IASetVertexBuffers(0, 1, &_pVertexPointInputBuffer, &Strides, &Offsets);

                pD3D11ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                pD3D11ImmediateContext->VSSetShader(_pD3D11VertexShader, nullptr, 0);
                pD3D11ImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);

                pD3D11ImmediateContext->Draw(std::size(_VertexPoint), 0);
            }


            void D3D11DrawContext::DrawString(
                uString _szText,
                const Font& _FontInfo,
                Color _crTextColor,
                const Rect& _LayoutRect,
                ContentAlignStyle _fTextAlign)
            {
                if (_szText.GetSize() == 0 || _LayoutRect.IsEmpty())
                    return;


                RefPtr<IDWriteTextFormat> _pTextFormat;
                auto _hr = g_DWrite.CreateTextFormat(_FontInfo, (YY::MegaUI::ContentAlignStyle)_fTextAlign, L"", &_pTextFormat);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                RefPtr<IDWriteTextLayout> _pTextLayout;
                _hr = g_DWrite.CreateTextLayout(_szText, _pTextFormat, _FontInfo.fStyle, _LayoutRect.GetSize(), &_pTextLayout);
                if (FAILED(_hr))
                {
                    throw Exception(_hr);
                }

                _pTextLayout->Draw(nullptr, this, _LayoutRect.Left, _LayoutRect.Top);
                // _pDWriteFactory->CreateGlyphRunAnalysis()
            }


            HRESULT D3D11DrawContext::QueryInterface(
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject)
            {
                if (!ppvObject)
                    return E_POINTER;

                *ppvObject = nullptr;

                if (riid == __uuidof(IUnknown))
                {
                    AddRef();
                    *ppvObject = static_cast<IUnknown*>(this);
                    return S_OK;
                }
                else if (riid == __uuidof(IDWritePixelSnapping))
                {
                    AddRef();
                    *ppvObject = static_cast<IDWritePixelSnapping*>(this);
                    return S_OK;
                }
                else if (riid == __uuidof(IDWriteTextRenderer))
                {
                    AddRef();
                    *ppvObject = static_cast<IDWriteTextRenderer*>(this);
                    return S_OK;
                }
                //else if (riid == __uuidof(IDWriteTextRenderer1))
                //{
                //    AddRef();
                //    *ppvObject = static_cast<IDWriteTextRenderer1*>(this);
                //    return S_OK;
                //}
                else
                {
                    return E_NOINTERFACE;
                }
            }

            ULONG D3D11DrawContext::AddRef()
            {
                return 1;
            }

            ULONG D3D11DrawContext::Release()
            {
                return 1;
            }

            HRESULT D3D11DrawContext::IsPixelSnappingDisabled(
                void* clientDrawingContext,
                BOOL* isDisabled
                )
            {
                *isDisabled = FALSE;
                return S_OK;
            }

            HRESULT D3D11DrawContext::GetCurrentTransform(
                void* clientDrawingContext,
                DWRITE_MATRIX* transform
                )
            {
                *(D2D1_MATRIX_3X2_F*)transform = D2D1::Matrix3x2F::Rotation(
                    0.0f,
                    D2D1::Point2F(0.0f, 0.0f));

                return S_OK;
            }

            HRESULT D3D11DrawContext::GetPixelsPerDip(
                void* clientDrawingContext,
                FLOAT* pixelsPerDip)
            {
                *pixelsPerDip = 96.0;
                return S_OK;
            }

            class DWriteGeometrySinkForD3D11 : public IDWriteGeometrySink
            {
            private:
                DrawContext* pDrawContext;
                Point BasePoint;
                Point startPoint;
                SolidColorBrush _ColorBrush = Color(0xFFu, 237, 28, 36);

            public:

                DWriteGeometrySinkForD3D11(_In_ DrawContext* _pDrawContext, Point _BasePoint)
                    : pDrawContext(_pDrawContext)
                    , BasePoint(_BasePoint)
                    , startPoint {}
                {

                }

                virtual HRESULT STDMETHODCALLTYPE QueryInterface(
                    /* [in] */ REFIID riid,
                    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
                {
                    if (!ppvObject)
                        return E_POINTER;
                    *ppvObject = nullptr;

                    if (riid == __uuidof(IUnknown))
                    {
                        AddRef();
                        *ppvObject = static_cast<IUnknown*>(this);
                        return S_OK;
                    }
                    else if (riid == __uuidof(IDWriteGeometrySink))
                    {
                        AddRef();
                        *ppvObject = static_cast<IDWriteGeometrySink*>(this);
                        return S_OK;
                    }
                    else
                    {
                        return E_NOINTERFACE;
                    }
                }

                virtual ULONG STDMETHODCALLTYPE AddRef() override
                {
                    // 内部使用，因此不完全实现
                    return 1;
                }

                virtual ULONG STDMETHODCALLTYPE Release() override
                {
                    // 内部使用，因此不完全实现
                    return 1;
                }

                void STDMETHODCALLTYPE SetFillMode(
                    D2D1_FILL_MODE fillMode) override
                {
                }

                void STDMETHODCALLTYPE SetSegmentFlags(
                    D2D1_PATH_SEGMENT vertexFlags) override
                {

                }

                void STDMETHODCALLTYPE BeginFigure(
                    D2D1_POINT_2F _StartPoint,
                    D2D1_FIGURE_BEGIN figureBegin) override
                {
                    startPoint.X = _StartPoint.x + BasePoint.X;
                    startPoint.Y = _StartPoint.y + BasePoint.Y;
                }

                void STDMETHODCALLTYPE AddLines(
                    _In_reads_(pointsCount) CONST D2D1_POINT_2F* points,
                    UINT32 pointsCount) override
                {
                    for (UINT32 i = 0; i != pointsCount; ++i)
                    {
                        const Point _NewPoint(points[i].x + BasePoint.X, points[i].y + BasePoint.Y);

                        pDrawContext->DrawLine(_ColorBrush, startPoint, _NewPoint);
                        startPoint = _NewPoint;
                    }
                }

                void STDMETHODCALLTYPE AddBeziers(
                    _In_reads_(beziersCount) CONST D2D1_BEZIER_SEGMENT* beziers,
                    UINT32 beziersCount) override
                {
                    for (UINT32 i = 0; i != beziersCount; ++i)
                    {
                        Point point1(beziers[i].point1.x + BasePoint.X, beziers[i].point1.y + BasePoint.Y);
                        Point point2(beziers[i].point2.x + BasePoint.X, beziers[i].point2.y + BasePoint.Y);
                        Point point3(beziers[i].point3.x + BasePoint.X, beziers[i].point3.y + BasePoint.Y);
                        pDrawContext->DrawLine(_ColorBrush, point1, point2);
                        pDrawContext->DrawLine(_ColorBrush, point2, point3);
                    }
                }

                void STDMETHODCALLTYPE EndFigure(
                    D2D1_FIGURE_END figureEnd) override
                {

                }

                HRESULT STDMETHODCALLTYPE Close() override
                {
                    // 故意什么也不做。
                    return S_OK;
                }
            };

            HRESULT D3D11DrawContext::DrawGlyphRun(
                void* clientDrawingContext,
                FLOAT baselineOriginX,
                FLOAT baselineOriginY,
                DWRITE_MEASURING_MODE measuringMode,
                DWRITE_GLYPH_RUN const* glyphRun,
                DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                IUnknown* clientDrawingEffect)
            {
                DWriteGeometrySinkForD3D11 _DWriteGeometrySink(this, Point(baselineOriginX, baselineOriginY));

                return glyphRun->fontFace->GetGlyphRunOutline(
                    glyphRun->fontEmSize,
                    glyphRun->glyphIndices,
                    glyphRun->glyphAdvances,
                    glyphRun->glyphOffsets,
                    glyphRun->glyphCount,
                    glyphRun->isSideways,
                    glyphRun->bidiLevel & 1,
                    &_DWriteGeometrySink);

                #if 0

                auto _pDWriteFactory = g_DWrite.TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                RefPtr<IDWriteGdiInterop> gdiInterop;
                _pDWriteFactory->GetGdiInterop(&gdiInterop);


                //HDC hdc = CreateCompatibleDC(NULL);
                auto hdc = GetDC(hWnd);

                RefPtr<IDWriteBitmapRenderTarget> renderTarget;
                gdiInterop->CreateBitmapRenderTarget(hdc, 500, 500, &renderTarget);

                RefPtr<IDWriteRenderingParams> renderingParams;
                _pDWriteFactory->CreateRenderingParams(&renderingParams);

                return renderTarget->DrawGlyphRun(baselineOriginX, baselineOriginY, measuringMode, glyphRun, renderingParams, RGB(255, 255, 255));


                DWRITE_RENDERING_MODE renderingMode = DWRITE_RENDERING_MODE::DWRITE_RENDERING_MODE_DEFAULT;
                RefPtr<IDWriteGlyphRunAnalysis> glyphRunAnalysis;
                auto _hr = _pDWriteFactory->CreateGlyphRunAnalysis(glyphRun, 1.0f, nullptr, renderingMode, measuringMode, baselineOriginX, baselineOriginY, &glyphRunAnalysis);
                if (FAILED(_hr))
                    return _hr;

                const DWRITE_TEXTURE_TYPE textureType = DWRITE_TEXTURE_ALIASED_1x1;

                RECT boundingBox;
                _hr = glyphRunAnalysis->GetAlphaTextureBounds(textureType, &boundingBox);
                if (FAILED(_hr))
                    return _hr;

                // 矩阵如果为空，那么无需继续绘制。
                if (boundingBox.left <= boundingBox.right || boundingBox.top <= boundingBox.bottom)
                {
                    return S_OK;
                }

                UINT32 width = boundingBox.right - boundingBox.left;
                UINT32 height = boundingBox.bottom - boundingBox.top;
                UINT32 textureStride = width;
                if (textureType == DWRITE_TEXTURE_CLEARTYPE_3x1)
                {
                    // ClearType bitmaps (DWRITE_TEXTURE_CLEARTYPE_3x1) contain 3 bytes per pixel,
                    // Aliased bitmaps only contain 1.
                    textureStride *= 3;
                }
                UINT32 textureSize = textureStride * height;

                std::unique_ptr<BYTE> _pAlphaValues(new BYTE[textureSize]);

                _hr = glyphRunAnalysis->CreateAlphaTexture(textureType, &boundingBox, _pAlphaValues.get(), textureSize);
                if (FAILED(_hr))
                    return _hr;

                return S_OK;
                #endif
            }

            HRESULT D3D11DrawContext::DrawUnderline(
                void* clientDrawingContext,
                FLOAT baselineOriginX,
                FLOAT baselineOriginY,
                DWRITE_UNDERLINE const* underline,
                IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }

            HRESULT D3D11DrawContext::DrawStrikethrough(
                void* clientDrawingContext,
                FLOAT baselineOriginX,
                FLOAT baselineOriginY,
                DWRITE_STRIKETHROUGH const* strikethrough,
                IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }

            HRESULT D3D11DrawContext::DrawInlineObject(
                void* clientDrawingContext,
                FLOAT originX,
                FLOAT originY,
                IDWriteInlineObject* inlineObject,
                BOOL isSideways,
                BOOL isRightToLeft,
                IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }

            HRESULT D3D11DrawContext::DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle, DWRITE_MEASURING_MODE measuringMode, DWRITE_GLYPH_RUN const* glyphRun, DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription, IUnknown* clientDrawingEffect)
            {
                auto _pDWriteFactory = g_DWrite.TryGetDWriteFactory();
                if (!_pDWriteFactory)
                    return E_NOINTERFACE;

                RefPtr<IDWriteGdiInterop> gdiInterop;
                _pDWriteFactory->GetGdiInterop(&gdiInterop);

                // HDC hdc = CreateCompatibleDC(NULL);
                auto hdc = GetDC(hWnd);

                RefPtr<IDWriteBitmapRenderTarget> renderTarget;
                gdiInterop->CreateBitmapRenderTarget(hdc, 500, 500, &renderTarget);

                RefPtr<IDWriteRenderingParams> renderingParams;
                _pDWriteFactory->CreateRenderingParams(&renderingParams);

                return renderTarget->DrawGlyphRun(baselineOriginX, baselineOriginY, measuringMode, glyphRun, renderingParams, RGB(255, 255, 255));

                DWRITE_RENDERING_MODE renderingMode = DWRITE_RENDERING_MODE::DWRITE_RENDERING_MODE_DEFAULT;
                RefPtr<IDWriteGlyphRunAnalysis> glyphRunAnalysis;
                auto _hr = _pDWriteFactory->CreateGlyphRunAnalysis(glyphRun, 1.0f, nullptr, renderingMode, measuringMode, baselineOriginX, baselineOriginY, &glyphRunAnalysis);
                if (FAILED(_hr))
                    return _hr;

                const DWRITE_TEXTURE_TYPE textureType = DWRITE_TEXTURE_ALIASED_1x1;

                RECT boundingBox;
                _hr = glyphRunAnalysis->GetAlphaTextureBounds(textureType, &boundingBox);
                if (FAILED(_hr))
                    return _hr;

                // 矩阵如果为空，那么无需继续绘制。
                if (boundingBox.left <= boundingBox.right || boundingBox.top <= boundingBox.bottom)
                {
                    return S_OK;
                }

                UINT32 width = boundingBox.right - boundingBox.left;
                UINT32 height = boundingBox.bottom - boundingBox.top;
                UINT32 textureStride = width;
                if (textureType == DWRITE_TEXTURE_CLEARTYPE_3x1)
                {
                    // ClearType bitmaps (DWRITE_TEXTURE_CLEARTYPE_3x1) contain 3 bytes per pixel,
                    // Aliased bitmaps only contain 1.
                    textureStride *= 3;
                }
                UINT32 textureSize = textureStride * height;

                std::unique_ptr<BYTE> _pAlphaValues(new BYTE[textureSize]);

                _hr = glyphRunAnalysis->CreateAlphaTexture(textureType, &boundingBox, _pAlphaValues.get(), textureSize);
                if (FAILED(_hr))
                    return _hr;

                return S_OK;
            }

            HRESULT D3D11DrawContext::DrawUnderline(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle, DWRITE_UNDERLINE const* underline, IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }

            HRESULT D3D11DrawContext::DrawStrikethrough(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle, DWRITE_STRIKETHROUGH const* strikethrough, IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }

            HRESULT D3D11DrawContext::DrawInlineObject(void* clientDrawingContext, FLOAT originX, FLOAT originY, DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, IUnknown* clientDrawingEffect)
            {
                return E_NOTIMPL;
            }
        } // namespace Graphics
    } // namespace Media
} // namespace YY
