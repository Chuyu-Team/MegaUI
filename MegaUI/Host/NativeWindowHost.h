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
        class Render;

        class NativeWindowHost
        {
        private:
            HWND hWnd;
            Element* pHost;
            Render* pRender;
            D2D1_SIZE_U LastRenderSize;
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

            bool __fastcall IsMinimized() const;

            static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            static UINT __fastcall AsyncDestroyMsg();

        private:
            HRESULT __fastcall OnPaint();
            
            HRESULT __fastcall PaintElement(
                _In_ Render* _pRender,
                _In_ Element* _pElement,
                _In_ const Rect& _ParentBounds,
                _In_ const Rect& _ParentPaintRect
                );

            void __fastcall OnSize(UINT _uWidth, UINT _uHeight);

            void __fastcall UpdateMouseWithin(Element* _pElement, const Rect& _ParentBounds, const Rect& _ParentVisibleBounds, const POINT& ptPoint);

            void __fastcall UpdateMouseWithinToFalse(Element* _pElement);
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
