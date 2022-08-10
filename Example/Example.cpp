// Example.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Example.h"

#include <MegaUI/Host/NativeWindowHost.h>

using namespace YY::MegaUI;

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

    intptr_t Cooike;
    YY::MegaUI::Element::Create(0, nullptr, &Cooike, &p);
    {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, p, &Cooike, &p2);
        p->Add(p2);

        p2->SetX(100);
        p2->SetY(100);

        p2->SetHeight(400);
        p2->SetWidth(300);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(128, 255, 255, 0)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));
        p2->EndDefer(Cooike);
    }

     {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, p, &Cooike, &p2);
        p->Add(p2);

        p2->SetX(300);
        p2->SetY(300);

        p2->SetHeight(400);
        p2->SetWidth(300);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(32, 255, 255, 0)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));
        p2->EndDefer(Cooike);
    }

    p->SetValue(YY::MegaUI::Element::g_ClassInfoData.BackgroundProp, YY::MegaUI::PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeRGB(255, 0, 0)));
    


    _pWindows->SetHost(p);
    p->EndDefer(Cooike);

    _pWindows->ShowWindow(SW_SHOW);

    MSG _Msg;
    while (GetMessageW(&_Msg, nullptr, 0, 0))
    {
        TranslateMessage(&_Msg);
        DispatchMessage(&_Msg);
    }

    return (int)_Msg.wParam;
}