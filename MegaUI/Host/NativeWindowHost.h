#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include "../base/MegaUITypeInt.h"
#include "../core/Element.h"


#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class NativeWindowHost
        {
        private:
            HWND hWnd;
            Element* pHost;

            ID2D1Factory* m_pD2DFactory;
            IWICImagingFactory* m_pWICFactory;
            IDWriteFactory* m_pDWriteFactory;
            ID2D1HwndRenderTarget* m_pRenderTarget;
            ID2D1BitmapRenderTarget* m_pCompatibleRenderTarget;
            IDWriteTextFormat* m_pTextFormat;
            ID2D1PathGeometry* m_pPathGeometry;
            ID2D1LinearGradientBrush* m_pLinearGradientBrush;
            ID2D1SolidColorBrush* m_pBlackBrush;
            ID2D1BitmapBrush* m_pGridPatternBitmapBrush;
            ID2D1Bitmap* m_pBitmap;
            ID2D1Bitmap* m_pAnotherBitmap;
        public:
            NativeWindowHost();

            ~NativeWindowHost();

            static
            HRESULT
            __fastcall
            Create(
                _In_opt_ LPCWSTR            _szTitle,
                _In_opt_ HWND               _hWndParent,
                _In_opt_ HICON              _hIcon,
                _In_     int                _dX,
                _In_     int                _dY,
                _In_     int                _dWidth,
                _In_     int                _dHeight,
                _In_     DWORD              _fExStyle,
                _In_     DWORD              _fStyle,
                _In_     UINT               _nOptions,
                _Outptr_ NativeWindowHost** _pWindow
                );
            
            HRESULT
            __fastcall
            Initialize(
                _In_opt_ LPCWSTR _szTitle,
                _In_opt_ HWND    _hWndParent,
                _In_opt_ HICON   _hIcon,
                _In_     int     _dX,
                _In_     int     _dY,
                _In_     int     _dWidth,
                _In_     int     _dHeight,
                _In_     DWORD   _fExStyle,
                _In_     DWORD   _fStyle,
                _In_     UINT    _nOptions
                );

            void __fastcall ShowWindow(int _iCmdShow);

            void __fastcall DestroyWindow();
            
            HRESULT __fastcall SetHost(_In_ Element* _pHost);

            static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            static UINT __fastcall AsyncDestroyMsg();

        private:
            HRESULT __fastcall InitializeD2D();

            HRESULT __fastcall OnPaint();

            HRESULT __fastcall InitializeRenderTarget();

            void __fastcall OnSize(UINT _uWidth, UINT _uHeight);
        };
    } // namespace MegaUI
} // namespace YY