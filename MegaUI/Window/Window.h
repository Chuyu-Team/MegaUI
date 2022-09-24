#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include "../base/MegaUITypeInt.h"
#include "WindowElement.h"
#include "../base/DynamicArray.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {

        class Render;

        class Window
        {
        protected:
            HWND hWnd;
            WindowElement* pHost;
            Render* pRender;
            D2D1_SIZE_U LastRenderSize;
            uint32_t fTrackMouse = 0u;
            // TODO 需要更换为块式队列更友好。
            DynamicArray<Element*> DelayedDestroyList;

            // bit位
            uint8_t bWindowVisible : 1;
        public:
            Window();

            Window(const Window&) = delete;
            void operator=(const Window&) = delete;

            virtual ~Window();
            
            HRESULT __MEGA_UI_API Initialize(
                _In_opt_ HWND _hWndParent,
                _In_opt_ HICON _hIcon,
                _In_ int _dX,
                _In_ int _dY,
                _In_ DWORD _fExStyle,
                _In_ DWORD _fStyle,
                _In_ UINT _nOptions
                );

            HRESULT __MEGA_UI_API SetHost(_In_ WindowElement* _pHost);

            static UINT __MEGA_UI_API AsyncDestroyMsg();

            void __MEGA_UI_API DestroyWindow();
            
            bool __MEGA_UI_API IsMinimized() const;

            void __MEGA_UI_API ShowWindow(int _iCmdShow);

            void __MEGA_UI_API InvalidateRect(_In_opt_ const Rect* _pRect);

            __inline void __MEGA_UI_API InvalidateRect(const Rect& _Rect)
            {
                InvalidateRect(&_Rect);
            }

            HRESULT __MEGA_UI_API PostDelayedDestroyElement(Element* _pElement);
            
            void __MEGA_UI_API HandleVisiblePropChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _NewValue);

            void __MEGA_UI_API HandleEnabledPropChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ const Value& _pOldValue, _In_ const Value& _NewValue);

        protected:
            static LRESULT CALLBACK WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

            virtual LRESULT __thiscall CurrentWndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

            HRESULT __MEGA_UI_API OnPaint();

            HRESULT __MEGA_UI_API PaintElement(
                _In_ Render* _pRender,
                _In_ Element* _pElement,
                _In_ const Rect& _ParentBounds,
                _In_ const Rect& _ParentPaintRect);

            void __MEGA_UI_API OnSize(UINT _uWidth, UINT _uHeight);
            
            void __MEGA_UI_API UpdateMouseWithin(
                Element* _pElement,
                const Rect& _ParentBounds,
                const Rect& _ParentVisibleBounds,
                const POINT& _ptPoint);

            void __MEGA_UI_API ClearDelayedDestroyList();

            void __MEGA_UI_API UpdateStyles(_In_opt_ uint32_t _uOld, _In_ uint32_t _uNew);
        };
    }
} // namespace YY

#pragma pack(pop)
