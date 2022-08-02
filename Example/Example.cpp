// Example.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Example.h"

#include <MegaUI/Host/NativeWindowHost.h>


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    YY::MegaUI::NativeWindowHost* _pWindows;
    YY::MegaUI::NativeWindowHost::Create(L"YY Mega DirectUI Test", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, &_pWindows);

    YY::MegaUI::Element::Register();

    YY::MegaUI::Element* p;

    YY::MegaUI::Element::Create(0, nullptr, nullptr, &p);

    _pWindows->SetHost(p);

    _pWindows->ShowWindow(SW_SHOW);

    MSG _Msg;
    while (GetMessageW(&_Msg, nullptr, 0, 0))
    {
        TranslateMessage(&_Msg);
        DispatchMessage(&_Msg);
    }

    return (int)_Msg.wParam;
}