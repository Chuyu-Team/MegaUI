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
#include <Base/Strings/String.h>
#include <Base/Threading/TaskRunner.h>
#include <Base/Sync/InterlockedQueue.h>
#include <Base/Threading/ProcessThreads.h>

#include <Media/Graphics/GDIPlus/GDIPlusHelper.h>

#ifdef _WIN32
#include <Media/Graphics/D2D/D2D1_0DrawContext.h>
#include <Media/Graphics/D2D/D2D1_1DrawContext.h>
#include <Media/Graphics/GDIPlus/GDIPlusDrawContext.h>
#endif

#include <functional>


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

#if defined(_WIN32)
Task TestCoroutine()
{
    std::cout << "TID : " << YY::Base::Threading::GetCurrentThreadId() << " 我是 TestAsync，开始模拟耗时任务\n";
    aString _szOutString;
    auto _pTaskRunner = YY::Base::Threading::SequencedTaskRunner::Create();
    co_await _pTaskRunner->AsyncTask(
        [&_szOutString]()
        {
            for (int i = 0; i != 5; ++i)
            {
                std::cout << ">>>>>>>> TID : " << ::GetCurrentThreadId() << " 我正在处理数据，处理了" << i << "秒，\n";
                // Sleep(1000);
            }     
            std::cout << ">>>>>>>> TID : " << ::GetCurrentThreadId() << " 假装处理数据结束，向主线程摊牌。\n";

            _szOutString = "给主线程的字符串。";
        });
    std::cout << "!!!!!!! TID : " << ::GetCurrentThreadId() << " TestAsync 结束, _szOutString：" << _szOutString << "\n";
    co_return;
}
#endif

int wmain()
{

    auto _pMainThreadRunner = YY::Base::Threading::ThreadTaskRunner::BindCurrentThread();

    _pMainThreadRunner->PostTask(
        []()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << "Tick " << GetTickCount() << " 投递的任务需要等 RunUIMessageLoop 开始。\n";
        });


    auto _pTimer = _pMainThreadRunner->CreateTimer(TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(1000),
        []()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << " Tick " << GetTickCount() << " 5秒后定时任务。\n";
        });


    //_pMainThreadRunner->PostDelayTask(
    //    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(5000),
    //    );

    // 注意保留生命周期，如果TaskRunner生命周期结束则自动取消内部所有任务！
    auto _pAsynTaskRunner = Threading::SequencedTaskRunner::Create();

    _pAsynTaskRunner->PostTask(
        [_pMainThreadRunner]()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << "我是另外一个异步线程。\n";

            std::vector<int> DDD = {1, 2, 3};

            // 将结果回传主线程
            _pMainThreadRunner->PostTask(
                [DDD = std::move(DDD)]()
                {
                    // 主线程
                    for (auto _nData : DDD)
                    {
                        std::cout << "TID : " << ::GetCurrentThreadId() << "_nData = " << _nData << "\n";
                    }
                });
        });

    std::cout << std::endl;
    std::cout << "TID : " << ::GetCurrentThreadId() << " 测试进程启动，我是主线程。\n";

    // AsyncReadCoroutineTest(LR"(E:\软件程序\WDK 10.0.10586.0.iso)");

#if 1
    // YY::MegaUI::NativeWindowHost::Create(L"YY Mega DirectUI Test", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, &_pWindows);
    WindowElement::Register();
    Button::Register();
    TextBox::Register();

    StyleSheet::AddGlobalStyleSheet(GetUiResource(IDR_UI_COMMON));

    UIParser _Parser;

    _Parser.ParserByXmlString(GetUiResource(IDR_UI1));

    UIParserPlayContext _Context;
    _Context.iDPI = 96;
    {
        Window* _pTestWindow = HNew<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, D2D1_1DrawContext::GetDrawContextFactory());
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.1");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    {
        Window* _pTestWindow = HNew<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, D2D1_0DrawContext::GetDrawContextFactory());
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.0");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    if (1)
    {
        Window* _pTestWindow = HNew<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, GDIPlusDrawContext::GetDrawContextFactory());
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
