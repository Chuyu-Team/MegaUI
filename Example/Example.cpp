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

#include <Base/IO/File.h>

#include <functional>

#include <Base/Threading/Coroutine.h>

#include <Base/Utils/ComObjectImpl.h>

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

#if defined(_WIN32)
Coroutine<void> WriteMessage(AsyncPipe& _hPipe, const void* _pData, DWORD _cbData)
{
    std::string _oMessage;
    _oMessage.append((char*) & _cbData, sizeof(_cbData));
    _oMessage.append((char*)_pData, _cbData);

    co_await _hPipe.AsyncWrite(0, _oMessage.c_str(), _oMessage.size());
    co_return;
}

Coroutine<void> TestCoroutine()
{
    //std::cout << "TID : " << YY::Base::Threading::GetCurrentThreadId() << " 我是 TestAsync，开始模拟耗时任务\n";
    //aString _szOutString;
    //auto _pTaskRunner = YY::Base::Threading::SequencedTaskRunner::Create();
    //co_await _pTaskRunner->AsyncTask(
    //    [&_szOutString]()
    //    {
    //        for (int i = 0; i != 5; ++i)
    //        {
    //            std::cout << ">>>>>>>> TID : " << ::GetCurrentThreadId() << " 我正在处理数据，处理了" << i << "秒，\n";
    //            // Sleep(1000);
    //        }     
    //        std::cout << ">>>>>>>> TID : " << ::GetCurrentThreadId() << " 假装处理数据结束，向主线程摊牌。\n";

    //        _szOutString = "给主线程的字符串。";
    //    });
    //std::cout << "!!!!!!! TID : " << ::GetCurrentThreadId() << " TestAsync 结束, _szOutString：" << _szOutString << "\n";

    //auto _File = AsyncFile::Open(LR"(C:\Windows\explorer.exe)", Access::Read, ShareMode::Read | ShareMode::Write | ShareMode::Delete);

    //constexpr auto kcbData = 10 * 1024 * 1024;
    //auto _pData = new byte[kcbData];
    //auto _cbData = co_await _File.AsyncRead(0, _pData, kcbData);
    //_cbData = co_await _File.AsyncRead(0, _pData, kcbData);
    //_cbData = co_await _File.AsyncRead(0, _pData, kcbData);
    //std::cout << "!!!!!!! TID : " << ::GetCurrentThreadId() << " AsyncRead 结束, 成功读取：" << _cbData << "\n";

    auto _hPipe = AsyncPipe::Create(
        L"\\\\.\\pipe\\mynamedpipe",
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4 * 1024 * 1024,
        4 * 1024 * 1024,
        5000,
        nullptr);

    auto _pTaskRunner = YY::TaskRunner::GetCurrent();

    struct Message
    {
        DWORD cbData;
    };

    Message _oMessage;
    std::string _szMessageData;

    for (;;)
    {
        auto _lStatus = co_await _hPipe.AsyncConnect();
        if (_lStatus != ERROR_SUCCESS)
        {
            printf("AsyncConnect 失败，错误代码：%u\n", _lStatus);
            co_return;
        }
        for (;;)
        {
            // 读取消息长度
            auto _cbData = co_await _hPipe.AsyncRead(0, &_oMessage.cbData, sizeof(_oMessage.cbData));
            if (_cbData == 0)
            {
                _lStatus = GetLastError();
                if (_lStatus == ERROR_BROKEN_PIPE)
                {
                    // 重新连接管道
                    break;
                }
                else
                {
                    printf("AsyncRead 失败，错误代码：%u\n", _lStatus);
                    co_return;
                }
            }

            if (_cbData != sizeof(Message::cbData))
            {
                // 消息错误
                co_return;
            }
            _szMessageData.resize(_oMessage.cbData);
            _cbData = co_await _hPipe.AsyncRead(0, _szMessageData.data(), _oMessage.cbData);
            _szMessageData.resize(_cbData);

            printf("Message 按预期 %u 字节，成功读取 %u 字节，内容：%s\n", _oMessage.cbData, _cbData, _szMessageData.c_str());

            std::string _szRespond = "Ok:" + _szMessageData;
            DWORD _cbRespand = _szRespond.size();
            co_await _hPipe.AsyncWrite(0, &_cbRespand, sizeof(_cbRespand));
            co_await _hPipe.AsyncWrite(0, _szRespond.c_str(), _szRespond.size());
        }
    }
    co_return;
}
#endif

#include <UIAutomation.h>
#include <type_traits>
//class TTTTT
//    : public YY::ComObjectImpl<TTTTT, YY::ComObjectInheritChain<ITextProvider2, ITextProvider, ITextRangeProvider>, YY::ComObjectInheritChain<ITextRangeProvider, IUnknown>>
//{
//public:
//    virtual HRESULT STDMETHODCALLTYPE GetSelection(
//        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetVisibleRanges(
//        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE RangeFromChild(
//        /* [in] */ __RPC__in_opt IRawElementProviderSimple* childElement,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE RangeFromPoint(
//        /* [in] */ struct UiaPoint point,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DocumentRange(
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(
//        /* [retval][out] */ __RPC__out enum SupportedTextSelection* pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE RangeFromAnnotation(
//        /* [in] */ __RPC__in_opt IRawElementProviderSimple* annotationElement,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetCaretRange(
//        /* [out] */ __RPC__out BOOL* isActive,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal)
//    {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE Clone(
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE Compare(
//        /* [in] */ __RPC__in_opt ITextRangeProvider* range,
//        /* [retval][out] */ __RPC__out BOOL* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE CompareEndpoints(
//        /* [in] */ enum TextPatternRangeEndpoint endpoint,
//        /* [in] */ __RPC__in_opt ITextRangeProvider* targetRange,
//        /* [in] */ enum TextPatternRangeEndpoint targetEndpoint,
//        /* [retval][out] */ __RPC__out int* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(
//        /* [in] */ enum TextUnit unit) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE FindAttribute(
//        /* [in] */ TEXTATTRIBUTEID attributeId,
//        /* [in] */ VARIANT val,
//        /* [in] */ BOOL backward,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE FindText(
//        /* [in] */ __RPC__in BSTR text,
//        /* [in] */ BOOL backward,
//        /* [in] */ BOOL ignoreCase,
//        /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetAttributeValue(
//        /* [in] */ TEXTATTRIBUTEID attributeId,
//        /* [retval][out] */ __RPC__out VARIANT* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetBoundingRectangles(
//        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetEnclosingElement(
//        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderSimple** pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetText(
//        /* [in] */ int maxLength,
//        /* [retval][out] */ __RPC__deref_out_opt BSTR* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE Move(
//        /* [in] */ enum TextUnit unit,
//        /* [in] */ int count,
//        /* [retval][out] */ __RPC__out int* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(
//        /* [in] */ enum TextPatternRangeEndpoint endpoint,
//        /* [in] */ enum TextUnit unit,
//        /* [in] */ int count,
//        /* [retval][out] */ __RPC__out int* pRetVal) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE MoveEndpointByRange(
//        /* [in] */ enum TextPatternRangeEndpoint endpoint,
//        /* [in] */ __RPC__in_opt ITextRangeProvider* targetRange,
//        /* [in] */ enum TextPatternRangeEndpoint targetEndpoint) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE Select(void) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE AddToSelection(void) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE RemoveFromSelection(void) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE ScrollIntoView(
//        /* [in] */ BOOL alignToTop) {
//        return 0;
//    }
//
//    virtual HRESULT STDMETHODCALLTYPE GetChildren(
//        /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) {
//        return 0;
//    }
//};


#include <Base/Utils/SystemInfo.h>

int wmain()
{
    auto _Version = GetOperatingSystemVersion();
    std::cout << " " << _Version.uMajor << "." << _Version.uMinor << "." << _Version.uBuild << "." << _Version.uRevision << "\n";

    //TTTTT TTTT;

    //CComPtr<ITextProvider2> T1;
    //CComPtr<ITextProvider> T2;
    //CComPtr<IUnknown> T3;
    //CComPtr<ITextRangeProvider> T4;

    //TTTT.QueryInterface(__uuidof(ITextProvider2), (void**)&T1);
    //TTTT.QueryInterface(__uuidof(ITextProvider), (void**)&T2);
    //TTTT.QueryInterface(__uuidof(IUnknown), (void**)&T3);
    //TTTT.QueryInterface(__uuidof(ITextRangeProvider), (void**)&T4);

    auto _pMainThreadRunner = YY::Base::Threading::ThreadTaskRunner::BindCurrentThread();

    auto _pAsynTaskRunner = Threading::SequencedTaskRunner::Create();

    auto _hEevent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    _pAsynTaskRunner->CreateWait(_hEevent,
        [_pAsynTaskRunner, _hEevent](DWORD)
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << "TickXXXXXXXX " << GetTickCount() << " 投递的任务需要等 RunUIMessageLoop 开始。\n";

            _pAsynTaskRunner->PostDelayTask(
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(5000),
                []()
                {
                    std::cout << "TID : " << ::GetCurrentThreadId() << "TickBBBBBBBBB " << GetTickCount() << " 投递的任务需要等 RunUIMessageLoop 开始。\n";
                });

            _pAsynTaskRunner->CreateWait(_hEevent,
                [](DWORD)
                {
                    std::cout << "TID : " << ::GetCurrentThreadId() << "TickVVVVVVVVVV " << GetTickCount() << " 投递的任务需要等 RunUIMessageLoop 开始。\n";
                    return false;
                });

            return false;
        });

    /*_pMainThreadRunner->PostDelayTask(
        TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(10000),
        [_hEevent, _pMainThreadRunner]()
        {
            SetEvent(_hEevent);
            _pMainThreadRunner->PostDelayTask(
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(10000),
                [_hEevent]()
                {
                    SetEvent(_hEevent);
                });
        });*/

//    _pMainThreadRunner->PostDelayTask(
//        TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(15000),
//        [_hEevent, _pAsynTaskRunner]()
//        {
//            _pAsynTaskRunner->PostDelayTask(TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(15000), []()
//                {
//                    int j = 0;
//                });
//
//             SetEvent(_hEevent);
//            _pMainThreadRunner->PostDelayTask(
//                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(10000),
//                [_hEevent]()
//                {
//                    SetEvent(_hEevent);
//                });
//        });
#if 1
    // auto TTT = TestCoroutine();

    _pMainThreadRunner->PostTask(
        []()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << "Tick " << GetTickCount() << " 投递的任务需要等 RunUIMessageLoop 开始。\n";
        });


    /*auto _pTimer = _pMainThreadRunner->CreateTimer(TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(1000),
        []()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << " Tick " << GetTickCount() << " 5秒后定时任务。\n";
        });*/


    //_pMainThreadRunner->PostDelayTask(
    //    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(5000),
    //    );

    // 注意保留生命周期，如果TaskRunner生命周期结束则自动取消内部所有任务！



    //HANDLE _hEvents[300] = {};

    //for (auto& _hEvent : _hEvents)
    //{
    //    _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    //    _pMainThreadRunner->CreateWait(
    //        _hEvent,
    //        [_hEvent](DWORD _Uesulr)
    //        {
    //            static int Count = 1;
    //            std::cout << "TID : " << ::GetCurrentThreadId() << "Wait Tick " << GetTickCount() << "  " << Count << " " << _hEvent << " result :" << _Uesulr << "\n";
    //            ++Count;
    //        });

    //}

    //_pAsynTaskRunner->PostDelayTask(TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(5000),
    //    [&_hEvents]()
    //    {

    //        std::cout << "TID : " << ::GetCurrentThreadId() << "SetEvent Tick " << GetTickCount() << "  " << "\n";
    //        for (auto _hEvent : _hEvents)
    //        {
    //            SetEvent(_hEvent);
    //        }
    //        // CloseHandle(_hEvent);
    //    });
    // SetEvent(_hEvent);

    


    

   auto _pTimer2 = _pAsynTaskRunner->CreateTimer(
       TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(5000),
        []()
        {
            std::cout << "TID : " << ::GetCurrentThreadId() << " Tick " << GetTickCount() << " 5秒后定时任务XXXXX。\n";

            return true;
        });

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
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, DrawContextFactory::GetD2D1_1DrawContextFactory());
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.1");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    {
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, DrawContextFactory::GetD2D1_0DrawContextFactory());
        pWindowElement->EndDefer(Cooike);
        SetWindowTextW(_pTestWindow->GetWnd(), L"D2D1.0");
        _pTestWindow->ShowWindow(SW_SHOWNORMAL);
    }

    if (1)
    {
        Window* _pTestWindow = New<Window>();
        YY::MegaUI::WindowElement* pWindowElement;
        intptr_t Cooike;
        _Parser.Play(u8"测试窗口", &_Context, &Cooike, &pWindowElement);

        _pTestWindow->SetHost(pWindowElement);

        _pTestWindow->Initialize(NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WS_EX_WINDOWEDGE, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, DrawContextFactory::GetGdiPlusDrawContextFactory());
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
#endif

    return YY::Base::Threading::ThreadTaskRunner::RunUIMessageLoop();
}
