// Example.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Example.h"

#include <vector>
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <memory>

#if defined(_WIN32)
#include <MegaUI/Window/Window.h>
#include <MegaUI/Parser/UIParser.h>
#include <MegaUI/Control/Button.h>
#include <MegaUI/Control/TextBox.h>
#endif
#include <YY/Base/Strings/String.h>
#include <YY/Base/Threading/TaskRunner.h>
#include <YY/Base/Sync/InterlockedQueue.h>
#include <YY/Base/Threading/ProcessThreads.h>

#include <YY/Base/IO/File.h>

#include <functional>

#include <YY/Base/Threading/Coroutine.h>

#include <YY/Base/Utils/ComObjectImpl.h>
#include <MegaUI/Core/TextScaleManger.h>

using namespace YY;

#if defined(_WIN32)
using namespace YY::MegaUI;

static u8String GetUiResource(DWORD _uResourceId)
{
    auto _hRes = FindResourceW(NULL, MAKEINTRESOURCEW(_uResourceId), L"UI");
    if (_hRes)
    {
        return u8String((u8char_t*)LockResource(LoadResource(NULL, _hRes)), SizeofResource(NULL, _hRes));
    }
    return u8String();
}
#endif

int wmain()
{
    auto _pMainThreadRunner = YY::Base::Threading::ThreadTaskRunner::BindCurrentThread();

#if 1
    WindowElement::Register();
    Button::Register();
    TextBox::Register();

    StyleSheet::AddGlobalStyleSheet(GetUiResource(IDR_UI_COMMON));

    UIParser _Parser;

    _Parser.ParserByXmlString(GetUiResource(IDR_UI1));

    UIParserPlayContext _Context;
    if (auto _pD2D1_1Factory = DrawContextFactory::GetD2D1_1DrawContextFactory())
    {
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, _pD2D1_1Factory);
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.1");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    if (auto _pD2D1_0Factory = DrawContextFactory::GetD2D1_0DrawContextFactory())
    {
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, _pD2D1_0Factory);
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.0");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    if (auto _pGdiPlusFactory = DrawContextFactory::GetGdiPlusDrawContextFactory())
    {
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, _pGdiPlusFactory);
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"GDIPlus");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }
#endif
// YY::MegaUI::Element* p;
// YY::MegaUI::Element::Create(0, _pWindows, &Cooike, &p);
#if 0
    {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, _pWindows, &Cooike, &p2);
        _pWindows->Add(p2);

        p2->SetX(100);
        p2->SetY(100);

        p2->SetHeight(400);
        p2->SetWidth(300);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(128, 255, 255, 0)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));
        p2->EndDefer(Cooike);
    }
#if 1
    {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, _pWindows, &Cooike, &p2);
        _pWindows->Add(p2);

        p2->SetX(300);
        p2->SetY(300);

        p2->SetHeight(400);
        p2->SetWidth(300);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(32, 255, 255, 0)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));

        p2->SetValue(Element::g_ClassInfoData.BorderThicknessProp, PropertyIndicies::PI_Local, Value::CreateRect(2, 4, 6, 8));
        p2->SetValue(Element::g_ClassInfoData.BorderColorProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(255u, 163u, 73u, 164u)));
        p2->SetValue(Element::g_ClassInfoData.BorderStyleProp, PropertyIndicies::PI_Local, Value::CreateInt32(BDS_Raised));

        p2->EndDefer(Cooike);
    }
    {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, _pWindows, &Cooike, &p2);
        _pWindows->Add(p2);

        p2->SetX(150);
        p2->SetY(150);

        p2->SetHeight(50);
        p2->SetWidth(40);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(255, 0, 255, 0)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));

        p2->SetValue(Element::g_ClassInfoData.BorderThicknessProp, PropertyIndicies::PI_Local, Value::CreateRect(2, 4, 6, 8));
        p2->SetValue(Element::g_ClassInfoData.BorderColorProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(255u, 255, 128, 192)));
        p2->SetValue(Element::g_ClassInfoData.BorderStyleProp, PropertyIndicies::PI_Local, Value::CreateInt32(BDS_Sunken));

        p2->EndDefer(Cooike);
    }
    {
        intptr_t Cooike;
        YY::MegaUI::Element* p2;
        YY::MegaUI::Element::Create(0, _pWindows, &Cooike, &p2);
        _pWindows->Add(p2);

        p2->SetX(450);
        p2->SetY(200);

        p2->SetHeight(50);
        p2->SetWidth(40);

        p2->SetValue(Element::g_ClassInfoData.BackgroundProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(255, 0, 128, 192)));
        // p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.MinSizeProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateSize(500, 60));
        p2->SetValue(YY::MegaUI::Element::g_ClassInfoData.LayoutPosProp, YY::MegaUI::PropertyIndicies::PI_Local, YY::MegaUI::Value::CreateInt32(LP_Absolute));

        p2->SetValue(Element::g_ClassInfoData.BorderThicknessProp, PropertyIndicies::PI_Local, Value::CreateRect(2, 4, 6, 8));
        p2->SetValue(Element::g_ClassInfoData.BorderColorProp, PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeARGB(255u, 255, 128, 192)));
        p2->SetValue(Element::g_ClassInfoData.BorderStyleProp, PropertyIndicies::PI_Local, Value::CreateInt32(BDS_Solid));

        p2->EndDefer(Cooike);
    }
#endif

    _pWindows->SetValue(YY::MegaUI::Element::g_ClassInfoData.BackgroundProp, YY::MegaUI::PropertyIndicies::PI_Local, Value::CreateColor(Color::MakeRGB(255, 0, 0)));
    
    //_pWindows->SetHost(p);
    _pWindows->EndDefer(Cooike);

    _pWindows->InitializeWindow(L"YY Mega DirectUI Test", nullptr, nullptr, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);

    //_pWindows->ShowWindow(SW_SHOW);
#endif

    return YY::Base::Threading::ThreadTaskRunner::RunUIMessageLoop();
}
