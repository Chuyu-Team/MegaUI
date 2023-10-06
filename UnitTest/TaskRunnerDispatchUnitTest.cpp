#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <Base/Threading/TaskRunnerDispatchImpl.h>
#include <Base/Time/TickCount.h>
#include <Base/Strings/String.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY::Base;
using namespace YY::Base::Threading;
using namespace YY::Base::Memory;
using namespace YY::Base::Time;

namespace UnitTest
{
    TEST_CLASS(TaskRunnerDispatchUnitTest)
    {
    public:
        TEST_METHOD(时间间隔检测)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();
            HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

            {
                auto _uStartTick = TickCount<TimePrecise::Millisecond>::GetCurrent();


                RefPtr<DelayDispatchEntry> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(500, [_uStartTick, &_pTimer, _hEvent]()
                    {
                        auto _nSpan = TickCount<TimePrecise::Millisecond>::GetCurrent() - _uStartTick;

                        Strings::uString _szTmp;
                        _szTmp.Format(L"Run 延迟 %I64d\n", _nSpan.GetMilliseconds());

                        OutputDebugStringW(_szTmp);

                        Assert::IsTrue(_nSpan.GetMilliseconds() >= 500 - 100);
                        Assert::IsTrue(_nSpan.GetMilliseconds() <= 500 + 100);

                        _pTimer->bCancel = true;
                        SetEvent(_hEvent);
                    });

                Assert::AreEqual(WaitForSingleObject(_hEvent, 5000), (DWORD)WAIT_OBJECT_0);
            }

            {
                auto _uStartTick = TickCount<TimePrecise::Millisecond>::GetCurrent();


                RefPtr<DelayDispatchEntry> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(5000, [_uStartTick, &_pTimer, _hEvent]()
                    {
                        auto _nSpan = TickCount<TimePrecise::Millisecond>::GetCurrent() - _uStartTick;

                        Strings::uString _szTmp;
                        _szTmp.Format(L"Run 延迟 %I64d\n", _nSpan.GetMilliseconds());

                        OutputDebugStringW(_szTmp);

                        Assert::IsTrue(_nSpan.GetMilliseconds() >= 5000 - 200);
                        Assert::IsTrue(_nSpan.GetMilliseconds() <= 5000 + 200);

                        _pTimer->bCancel = true;
                        SetEvent(_hEvent);
                    });


                Assert::AreEqual(WaitForSingleObject(_hEvent, 10000), (DWORD)WAIT_OBJECT_0);
            }
        }

        TEST_METHOD(周期性唤醒检查)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();


            HANDLE _hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

            auto _uStartTick = TickCount<TimePrecise::Millisecond>::GetCurrent();

            int nCount = 0;
            RefPtr<DelayDispatchEntry> _pTimer;
            _pTimer = _pTaskRunner->CreateTimer(500, [&nCount, _uStartTick, &_pTimer, _hEvent]()
                {
                    ++nCount;
                    auto _nSpan = TickCount<TimePrecise::Millisecond>::GetCurrent() - _uStartTick;

                    Strings::uString _szTmp;
                    _szTmp.Format(L"Run 延迟 %I64d\n", _nSpan.GetMilliseconds());

                    OutputDebugStringW(_szTmp);
                    auto _uArg = _nSpan.GetMilliseconds() / nCount;

                    Assert::IsTrue(_uArg >= 500 - 100);
                    Assert::IsTrue(_uArg <= 500 + 100);

                    if (nCount == 5)
                    {
                        _pTimer->bCancel = true;

                        SetEvent(_hEvent);
                    }
                });


            WaitForSingleObject(_hEvent, 5000);

            Assert::IsTrue(nCount  == 5);
            _pTimer = nullptr;
            _pTaskRunner = nullptr;

        }
    };
}
