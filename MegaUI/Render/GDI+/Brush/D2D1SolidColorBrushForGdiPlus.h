#pragma once
#include <d2d1.h>
#include <GdiPlus.h>

#include <Base/Sync/Interlocked.h>
#include <Multimedia/Graphics/Color.h>

#include "../../../base/MegaUITypeInt.h"
#include "../../../base/alloc.h"

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class D2D1SolidColorBrushForGdiPlus : public ID2D1SolidColorBrush
        {
        private:
            uint32_t uRef;
            Gdiplus::SolidBrush Brush;
        public:
            D2D1SolidColorBrushForGdiPlus(Color _Color)
                : uRef(1u)
                , Brush(Gdiplus::Color(_Color.Alpha, _Color.Red, _Color.Green, _Color.Blue))
            {
            }

            Gdiplus::SolidBrush* __YYAPI GetBrush()
            {
                return &Brush;
            }

            virtual HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
            {
                if (!ppvObject)
                    return E_INVALIDARG;
                *ppvObject = nullptr;

                if (riid == __uuidof(ID2D1SolidColorBrush))
                {
                    AddRef();
                    *ppvObject = static_cast<ID2D1SolidColorBrush*>(this);
                    return S_OK;
                }
                else if (riid == __uuidof(ID2D1Brush))
                {
                    AddRef();
                    *ppvObject = static_cast<ID2D1Brush*>(this);
                    return S_OK;
                }
                else if (riid == __uuidof(ID2D1Resource))
                {
                    AddRef();
                    *ppvObject = static_cast<ID2D1Resource*>(this);
                    return S_OK;
                }
                else if (riid == __uuidof(IUnknown))
                {
                    AddRef();
                    *ppvObject = static_cast<IUnknown*>(this);
                    return S_OK;
                }

                return E_NOINTERFACE;
            }

            virtual ULONG STDMETHODCALLTYPE AddRef(void) override
            {
                return Sync::Increment(&uRef);
            }

            virtual ULONG STDMETHODCALLTYPE Release(void) override
            {
                auto _uNewRef = Sync::Decrement(&uRef);
                if (_uNewRef == 0)
                    HDelete(this);
                return _uNewRef;
            }

            // 不支持此接口
            virtual void STDMETHODCALLTYPE GetFactory(
                ID2D1Factory** factory) const override
            {
                if (factory)
                    *factory = nullptr;
                __debugbreak();
            }

            virtual void STDMETHODCALLTYPE SetOpacity(
                FLOAT opacity) override
            {
                __debugbreak();
            }

            virtual void STDMETHODCALLTYPE SetTransform(
                _In_ CONST D2D1_MATRIX_3X2_F* transform) override
            {
                __debugbreak();
            }

            virtual FLOAT STDMETHODCALLTYPE GetOpacity() CONST override
            {
                __debugbreak();
                return 1.0f;
            }

            virtual void STDMETHODCALLTYPE GetTransform(
                _Out_ D2D1_MATRIX_3X2_F* transform) CONST override
            {
                __debugbreak();
            }

            virtual void STDMETHODCALLTYPE SetColor(
                _In_ CONST D2D1_COLOR_F* color) override
            {
                Gdiplus::Color _Color(
                    color->a < 1.0f ? BYTE(color->a * 0xFFu) : BYTE(0xFFu),
                    color->r < 1.0f ? BYTE(color->r * 0xFFu) : BYTE(0xFFu),
                    color->g < 1.0f ? BYTE(color->g * 0xFFu) : BYTE(0xFFu),
                    color->b < 1.0f ? BYTE(color->b * 0xFFu) : BYTE(0xFFu));
                Brush.SetColor(_Color);
            }

            virtual D2D1_COLOR_F STDMETHODCALLTYPE GetColor() CONST override
            {
                Gdiplus::Color _Color;

                Brush.GetColor(&_Color);
                return D2D1_COLOR_F {float(_Color.GetRed()), float(_Color.GetGreen()), float(_Color.GetBlue()), float(_Color.GetAlpha()) / 255.0F};
            }
        };
    }
} // namespace YY

#pragma pack(pop)
