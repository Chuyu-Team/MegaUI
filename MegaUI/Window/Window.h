#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include <MegaUI/base/MegaUITypeInt.h>
#include "WindowElement.h"
#include <Base/Containers/Array.h>
#include <MegaUI/core/UIEvent.h>

#pragma pack(push, __YY_PACKING)

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
            uint32_t fTrackMouse;
            uint32_t bCapture;
            // TODO 需要更换为块式队列更友好。
            Array<Element*> DelayedDestroyList;
            // 鼠标焦点
            Element* pLastMouseFocusedElement = nullptr;
            // 逻辑焦点
            Element* pLastFocusedElement = nullptr;
            // 键盘焦点（全局）
            static Element* g_pLastKeyboardFocusedElement;
            Element* pLastPressedElement;
            int32_t iDpi;
            // WM_UPDATEUISTATE的缓存
            uint16_t fUIState = 0;

        public:
            Window(_In_ int32_t _DefaultDpi = 96);

            Window(const Window&) = delete;
            void operator=(const Window&) = delete;

            virtual ~Window();
            
            HRESULT __YYAPI Initialize(
                _In_opt_ HWND _hWndParent,
                _In_opt_ HICON _hIcon,
                _In_ int _dX,
                _In_ int _dY,
                _In_ DWORD _fExStyle,
                _In_ DWORD _fStyle,
                _In_ UINT _nOptions
                );

            HRESULT __YYAPI SetHost(_In_ WindowElement* _pHost);

            WindowElement* __YYAPI GetHost();

            static UINT __YYAPI AsyncDestroyMsg();

            void __YYAPI DestroyWindow();
            
            bool __YYAPI IsMinimized() const;

            bool __YYAPI CanMinimize() const;
            
            bool __YYAPI IsTopmost() const;

            bool __YYAPI CanMaximize() const;

            void __YYAPI ShowWindow(int _iCmdShow);

            void __YYAPI InvalidateRect(_In_opt_ const Rect* _pRect);

            __inline void __YYAPI InvalidateRect(const Rect& _Rect)
            {
                InvalidateRect(&_Rect);
            }

            HRESULT __YYAPI PostDelayedDestroyElement(Element* _pElement);
            
            bool __YYAPI HandleVisiblePropChanged(_In_ OnPropertyChangedHandleData* _pHandle);

            bool __YYAPI HandleEnabledPropChanged(_In_ OnPropertyChangedHandleData* _pHandle);

            constexpr static auto FindActionMouse = 0x00000001;
            constexpr static auto FindActionKeyboard = 0x00000002;
            constexpr static auto FindActionMarks = 0x00000003;
            constexpr static auto FindVisible = 0x00000004;
            constexpr static auto FindEnable = 0x00000008;


            Element* __YYAPI FindElementFromPoint(_In_ const Point& _ptPoint, _In_ uint32_t fFindMarks = FindVisible);

            int32_t __YYAPI GetDpi() const;

            /// <summary>
            /// 原生窗口是否已经创建？
            /// </summary>
            /// <returns></returns>
            bool __YYAPI IsInitialized() const;

            _Ret_notnull_ Render* __YYAPI GetRender();
            
            /// <summary>
            /// 设置键盘焦点。
            /// </summary>
            /// <param name="_pElement">需要设置焦点的控件</param>
            /// <returns>如果设置成功，则返回 true。</returns>
            static bool __YYAPI SetKeyboardFocus(_In_opt_ Element* _pElement);

            /// <summary>
            /// 设置键盘焦点。
            /// </summary>
            /// <param name="_pElement">需要设置焦点的控件</param>
            /// <returns>如果设置成功，则返回 true。</returns>
            bool __YYAPI SetFocus(_In_opt_ Element* _pElement);

            Element* __YYAPI GetFocus();

            void __YYAPI GetRect(Rect* _Bounds);

            void __YYAPI ClientToScreen(_Inout_ Rect* _Bounds);
            
            void __YYAPI ClientToScreen(_Inout_ Point* _pPoint);

            void __YYAPI ScreenToClient(_Inout_ Rect* _Bounds);
            
            void __YYAPI ScreenToClient(_Inout_ Point* _pPoint);

            HWND __YYAPI GetWnd();

            Element* __YYAPI GetPressed();

            void __YYAPI SetPressed(Element* _pElement);

        protected:
            static LRESULT CALLBACK StaticWndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

            virtual LRESULT __thiscall WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam);

            bool __YYAPI OnCreate();

            HRESULT __YYAPI OnPaint();

            HRESULT __YYAPI PaintElement(
                _In_ Render* _pRender,
                _In_ Element* _pElement,
                _In_ const Rect& _ParentBounds,
                _In_ const Rect& _ParentPaintRect);

            void __YYAPI OnSize(UINT _uWidth, UINT _uHeight);

            void __YYAPI ClearDelayedDestroyList();

            void __YYAPI UpdateStyles(_In_opt_ uint32_t _uOld, _In_ uint32_t _uNew);

            void __YYAPI OnDpiChanged(int32_t _iNewDPI, const Rect* _pNewRect);

            HRESULT __YYAPI UpdateDpi(Element* _pElement, Value _OldValue, const Value& _NewValue);

            void __YYAPI OnUpdateUiState(uint16_t _eType, uint16_t _fState);

            void __YYAPI OnMouseMove(Point _MousePoint, uint32_t _fFlags);
            
            void __YYAPI OnMouseFocusMoved(Element* _pFrom, Element* _pTo);

            bool __YYAPI OnLeftButtonDown(const MouseEvent& _Event);

            bool __YYAPI OnLeftButtonUp(const MouseEvent& _Event);

            bool __YYAPI OnKeyDown(const KeyboardEvent& _KeyEvent);
            
            bool __YYAPI OnKeyUp(const KeyboardEvent& _KeyEvent);
           
            bool __YYAPI OnChar(const KeyboardEvent& _KeyEvent);

            bool __YYAPI OnGetObject(uint32_t _fFlags, int32_t _uObjectId, LRESULT* _plResult);
        };
    }
} // namespace YY

#pragma pack(pop)
