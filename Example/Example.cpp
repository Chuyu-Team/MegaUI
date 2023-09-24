// Example.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Example.h"

#include <vector>
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <memory>

#include <MegaUI/Window/Window.h>
#include <MegaUI/Parser/UIParser.h>
#include <MegaUI/Control/Button.h>
#include <Base/Threading/TaskRunner.h>

using namespace YY;
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

#pragma comment(lib, "Synchronization.lib")

struct Task
{
    struct promise_type
    {
        Task get_return_object() noexcept
        {
            return {};
        }
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_never final_suspend() noexcept
        {
            return {};
        }

        void return_void() noexcept
        {
        }
        void unhandled_exception()
        {
        }
    };
};


Task TestCoroutine()
{
    std::cout << "TID : " << GetCurrentThreadId() << " 我是 TestAsync，开始模拟耗时任务\n"; 
    aString _szOutString;
    auto _pTaskRunner = YY::Base::Threading::SequencedTaskRunner::Create();
    co_await _pTaskRunner->AsyncTask(
        [&_szOutString]()
        {
            for (int i = 0; i != 5; ++i)
            {
                std::cout << ">>>>>>>> TID : " << GetCurrentThreadId() << " 我正在处理数据，处理了" << i << "秒，\n";
                Sleep(1000);
            }     
            std::cout << ">>>>>>>> TID : " << GetCurrentThreadId() << " 假装处理数据结束，向主线程摊牌。\n";

            _szOutString = "给主线程的字符串。";
        });
    std::cout << "!!!!!!! TID : " << GetCurrentThreadId() << " TestAsync 结束, _szOutString：" << _szOutString << "\n";
    co_return;
}

int wmain()
{
    return YY::Base::Threading::ThreadTaskRunner::RunUIMessageLoop(
        [&]()
        {
            std::cout << std::endl;
            std::cout << "TID : " << GetCurrentThreadId() << " 测试进程启动，我是主线程。\n";

            TestCoroutine();

            std::cout << "TID : " << GetCurrentThreadId() << " 开始创建窗口。\n";

            // YY::MegaUI::NativeWindowHost::Create(L"YY Mega DirectUI Test", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, &_pWindows);
            WindowElement::Register();
            Button::Register();

            StyleSheet::AddGlobalStyleSheet(GetUiResource(IDR_UI_COMMON));

            Window* _pTestWindow = HNew<Window>();
            Window* _pTestWindow2 = HNew<Window>();

            UIParser _Parser;

            _Parser.ParserByXmlString(GetUiResource(IDR_UI1));

            UIParserPlayContext _Context;
            _Context.iDPI = 96;
            {
                YY::MegaUI::WindowElement* pWindowElement;
                intptr_t Cooike;
                _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

                _pTestWindow->SetHost(pWindowElement);

                _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
                pWindowElement->EndDefer(Cooike);

                _pTestWindow->ShowWindow(SW_SHOWNORMAL);
            }

            {
                YY::MegaUI::WindowElement* pWindowElement;
                intptr_t Cooike;
                _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

                _pTestWindow2->SetHost(pWindowElement);

                _pTestWindow2->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
                pWindowElement->EndDefer(Cooike);

                _pTestWindow2->ShowWindow(SW_SHOWNORMAL);
            }

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
        });
}
