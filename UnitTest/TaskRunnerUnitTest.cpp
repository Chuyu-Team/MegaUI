#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <YY/Base/Threading/TaskRunner.h>
#include <YY/Base/Threading/ProcessThreads.h>
#include <YY/Base/Time/TickCount.h>
#include <YY/Base/Strings/String.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY::Base;
using namespace YY::Base::Threading;
using namespace YY::Base::Memory;
using namespace YY::Base::Time;

namespace TaskRunnerUnitTest
{
    TEST_CLASS(SequencedTaskRunnerUnitTest)
    {
    public:
        TEST_METHOD(任务序列化保证)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            uint32_t _uCount = 0;
            volatile uint32_t _uCount2 = 0;

            for (auto i = 0; i != 1000; ++i)
            {
                _pTaskRunner->PostTask(
                    [&_uCount, &_uCount2]()
                    {
                        Assert::AreEqual(Sync::Increment(&_uCount), 1u);

                        Sleep(5);

                        Assert::AreEqual(Sync::Decrement(&_uCount), 0u);

                        Sync::Increment(&_uCount2);
                    });
            }


            for (int i= 0; _uCount2 != 1000;++i)
            {
                Assert::IsTrue(i < 100);

                Sleep(1000);
            }
        }

        TEST_METHOD(TaskRunner销毁后任务全部自动取消)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            uint32_t _uCount = 0;
            volatile uint32_t _uCount2 = 0;

            for (auto i = 0; i != 1000; ++i)
            {
                _pTaskRunner->PostTask(
                    [&_uCount, &_uCount2]()
                    {
                        Assert::AreEqual(Sync::Increment(&_uCount), 1u);


                        if (Sync::Increment(&_uCount2) == 1)
                        {
                            Sleep(500);
                        }

                        Assert::AreEqual(Sync::Decrement(&_uCount), 0u);
                    });
            }

            _pTaskRunner = nullptr;

            Sleep(5000);


            Assert::AreEqual((uint32_t)_uCount2, 1u);
        }

        TEST_METHOD(TaskRunner内部可以正确取到TaskRunner自身指针)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            YY::RefPtr<SequencedTaskRunner> _pOutTaskRunner;

            _pTaskRunner->PostTask(
                [&_pOutTaskRunner]()
                {
                    _pOutTaskRunner = SequencedTaskRunner::GetCurrent();

                    Assert::AreEqual((void*)TaskRunner::GetCurrent().Get(), (void*)_pOutTaskRunner.Get());
                    Assert::AreEqual((void*)ThreadTaskRunner::GetCurrent().Get(), (void*)nullptr);
                    Assert::AreEqual((void*)ParallelTaskRunner::GetCurrent().Get(), (void*)nullptr);
                });

            Sleep(500);
            
            Assert::AreEqual((void*)_pOutTaskRunner.Get(), (void*)_pTaskRunner.Get());
        }

        TEST_METHOD(时间间隔检测)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            for(int i = 0;i!=500;++i)
            {
                constexpr auto kDelay = 10ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(10),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait(kDelay * 5));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }


            for (int i = 0; i != 250; ++i)
            {
                constexpr auto kDelay = 100ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation) * 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }

            for (int i = 0; i != 250; ++i)
            {
                constexpr auto kDelay = 1000ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation )* 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }


            for (int i = 0; i != 100; ++i)
            {
                constexpr auto kDelay = 1300ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation) * 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }
        }

        TEST_METHOD(周期性唤醒检查)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();


            HANDLE _hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

            auto _uStartTick = TickCount<TimePrecise::Millisecond>::GetCurrent();

            int nCount = 0;
            RefPtr<Timer> _pTimer;
            _pTimer = _pTaskRunner->CreateTimer(
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                [&nCount, _uStartTick, _hEvent]()
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
                        SetEvent(_hEvent);
                        return false;
                    }
                    else
                    {
                        return true;
                    }
                });


            WaitForSingleObject(_hEvent, 5000);

            Assert::IsTrue(nCount == 5);
            _pTimer = nullptr;
            _pTaskRunner = nullptr;

        }

        TEST_METHOD(Wait句柄测试)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            for (int i = 0; i < 3; ++i)
            {
                HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                volatile uint32_t _uWaitResultCount = 0;
                volatile UINT64 _uTickCount = 0;

                _pTaskRunner->CreateWait(_hEvent, [&](DWORD _uWaitResultT)
                    {
                        if (_uWaitResultT == WAIT_OBJECT_0)
                        {
                            YY::Increment(&_uWaitResultCount);
                            _uTickCount = GetTickCount64();
                        }

                        return true;
                    });

                Sleep(600);
                SetEvent(_hEvent);
                auto _uTickCountEnd = GetTickCount64();
                Sleep(10);

                Assert::IsTrue(abs((long long)(_uTickCountEnd - _uTickCount)) < 50);
                CloseHandle(_hEvent);
            }


            // 超多句柄等待情况测试
            {
                HANDLE _hEvents[300];
                volatile uint32_t _uWaitResultCount = 0;

                for (auto& _hEvent : _hEvents)
                {
                    _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                    _pTaskRunner->CreateWait(_hEvent, [&](DWORD _uWaitResultT)
                        {
                            if (_uWaitResultT == WAIT_OBJECT_0)
                            {
                                YY::Increment(&_uWaitResultCount);
                            }
                            
                            return true;
                        });
                }

                Sleep(100);
                Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(0));

                for (auto _hEvent : _hEvents)
                {
                    SetEvent(_hEvent);
                }

                Sleep(100);
                Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(std::size(_hEvents)));

                for (auto _hEvent : _hEvents)
                {
                    SetEvent(_hEvent);
                }

                Sleep(100);
                Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(std::size(_hEvents)));

                for (auto _hEvent : _hEvents)
                {
                    CloseHandle(_hEvent);
                }
            }
        }

        TEST_METHOD(Wait句柄超时测试)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            volatile DWORD _uWaitResult = -1;
            auto _pWait = _pTaskRunner->CreateWait(
                _hEvent,
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                [&_uWaitResult](DWORD _uWaitResultT)
                {
                    _uWaitResult = _uWaitResultT;
                    return true;
                });

            Assert::IsTrue(((TaskEntry*)_pWait.Get())->Wait(600ul));
            Assert::AreEqual(DWORD(_uWaitResult), DWORD(WAIT_TIMEOUT));

            _pWait = _pTaskRunner->CreateWait(
                _hEvent,
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                [&_uWaitResult](DWORD _uWaitResultT)
                {
                    _uWaitResult = _uWaitResultT;
                    return true;
                });
            SetEvent(_hEvent);
            Assert::IsTrue(((TaskEntry*)_pWait.Get())->Wait(100ul));
            CloseHandle(_hEvent);
        }
    };


    TEST_CLASS(ParallelTaskRunnerUnitTest)
    {
    public :
        TEST_METHOD(并行数量保证)
        {
            {
                // 只允许一个并行
                uint32_t _uCount = 0;
                volatile uint32_t _uCount2 = 0;
                auto _pTaskRunner = ParallelTaskRunner::Create(1);

                for (auto i = 0; i != 1000; ++i)
                {
                    _pTaskRunner->PostTask(
                        [&_uCount, &_uCount2]()
                        {
                            Assert::AreEqual(Sync::Increment(&_uCount), 1u);

                            Sleep(5);

                            Assert::AreEqual(Sync::Decrement(&_uCount), 0u);

                            Sync::Increment(&_uCount2);
                        });
                }

                for (int i = 0; _uCount2 != 1000; ++i)
                {
                    Assert::IsTrue(i < 100);

                    Sleep(1000);
                }
            }

            {
                // 允许4个并行
                uint32_t _uCount = 0;
                volatile uint32_t _uMaxCount = 0;
                volatile uint32_t _uCount2 = 0;
                auto _pTaskRunner = ParallelTaskRunner::Create(4);

                for (auto i = 0; i != 1000; ++i)
                {
                    _pTaskRunner->PostTask(
                        [&_uCount, &_uCount2, &_uMaxCount]()
                        {
                            auto _uNew = Sync::Increment(&_uCount);
                            Assert::IsTrue(_uNew <= 4u);
                            auto _uOldMaxCount = _uMaxCount;
                            for (; _uOldMaxCount < _uNew;)
                            {
                                auto _uLast = Sync::CompareExchange(&_uMaxCount, _uNew, _uOldMaxCount);
                                if (_uLast == _uOldMaxCount)
                                {
                                    break;
                                }

                                _uOldMaxCount = _uLast;
                            }

                            Sleep(5);

                            Assert::IsTrue(Sync::Decrement(&_uCount) < 4u);

                            Sync::Increment(&_uCount2);
                        });
                }

                for (int i = 0; _uCount2 != 1000; ++i)
                {
                    Assert::IsTrue(i < 100);

                    Sleep(1000);
                }

                Assert::AreEqual((uint32_t)_uMaxCount, 4u);
            }
        }

        
        TEST_METHOD(TaskRunner销毁后任务全部自动取消)
        {
            auto _pTaskRunner = ParallelTaskRunner::Create(4);

            uint32_t _uCount = 0;
            volatile uint32_t _uCount2 = 0;

            for (auto i = 0; i != 1000; ++i)
            {
                _pTaskRunner->PostTask(
                    [&_uCount, &_uCount2]()
                    {
                        Assert::IsTrue(Sync::Increment(&_uCount) <= 4u);

                        Sleep(500);

                        Sync::Increment(&_uCount2);
                        Assert::IsTrue(Sync::Decrement(&_uCount) < 4u);
                    });
            }

            _pTaskRunner = nullptr;

            Sleep(5000);


            Assert::IsTrue((uint32_t)_uCount2 < 100u);
        }

        TEST_METHOD(TaskRunner内部可以正确取到TaskRunner自身指针)
        {
            auto _pTaskRunner = ParallelTaskRunner::Create();

            YY::RefPtr<ParallelTaskRunner> _pOutTaskRunner;

            _pTaskRunner->PostTask(
                [&_pOutTaskRunner]()
                {
                    _pOutTaskRunner = ParallelTaskRunner::GetCurrent();

                    Assert::AreEqual((void*)TaskRunner::GetCurrent().Get(), (void*)_pOutTaskRunner.Get());
                    Assert::AreEqual((void*)SequencedTaskRunner::GetCurrent().Get(), (void*)nullptr);
                    Assert::AreEqual((void*)ThreadTaskRunner::GetCurrent().Get(), (void*)nullptr);
                });

            Sleep(500);
            
            Assert::AreEqual((void*)_pOutTaskRunner.Get(), (void*)_pTaskRunner.Get());
        }
    };

    TEST_CLASS(ThreadTaskRunnerUnitTest)
    {
    public:
        TEST_METHOD(PostTask可用性检测)
        {
            auto _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

            RefPtr<ThreadTaskRunner> _pTaskRunners[] = { ThreadTaskRunner::Create(false), ThreadTaskRunner::Create(true) };
            for (auto& _pTaskRunner : _pTaskRunners)
            {
                volatile uint32_t _uCount2 = 0;

                _pTaskRunner->PostTask([&_uCount2, _hEvent]()
                    {
                        YY::Increment(&_uCount2);
                        SetEvent(_hEvent);
                    });

                WaitForSingleObject(_hEvent, 100);

                Assert::AreEqual((uint32_t)_uCount2, 1u);

                _pTaskRunner->PostTask([&_uCount2, _hEvent]()
                    {
                        YY::Increment(&_uCount2);
                        SetEvent(_hEvent);
                    });

                WaitForSingleObject(_hEvent, 100);

                Assert::AreEqual((uint32_t)_uCount2, 2u);

                _pTaskRunner->PostTask([&_uCount2, _hEvent]()
                    {
                        YY::Increment(&_uCount2);
                        SetEvent(_hEvent);
                    });

                WaitForSingleObject(_hEvent, 100);

                Assert::AreEqual((uint32_t)_uCount2, 3u);
            }

            CloseHandle(_hEvent);
        }


        TEST_METHOD(线程Id获取)
        {
            auto _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

            auto _pTaskRunner = ThreadTaskRunner::Create(true);

            auto _oId = _pTaskRunner->GetThreadId();
            volatile decltype(_oId) _oId2 = 0xCC;


            _pTaskRunner->PostTask([&_oId2, _hEvent]()
                {
                    _oId2 = Threading::GetCurrentThreadId();
                    SetEvent(_hEvent);
                });

            WaitForSingleObject(_hEvent, 100);

            Assert::AreEqual(_oId, decltype(_oId)(_oId2));
            CloseHandle(_hEvent);
        }

        TEST_METHOD(时间间隔检测)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            for (int i = 0; i != 500; ++i)
            {
                constexpr auto kDelay = 10ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(10),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait(kDelay * 5));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }


            for (int i = 0; i != 250; ++i)
            {
                constexpr auto kDelay = 100ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation) * 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }

            for (int i = 0; i != 250; ++i)
            {
                constexpr auto kDelay = 1000ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation) * 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }


            for (int i = 0; i != 100; ++i)
            {
                constexpr auto kDelay = 1300ul;
                constexpr auto kDeviation = 35ul;

                auto _uStartTick = TickCount<TimePrecise::Microsecond>::GetCurrent();
                TickCount<TimePrecise::Microsecond> _uEnd;

                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(kDelay),
                    [&_uEnd]()
                    {
                        _uEnd = TickCount<TimePrecise::Microsecond>::GetCurrent();
                        return false;
                    });

                Assert::IsTrue(_pTimer->Wait((kDelay + kDeviation) * 2));

                auto nMilliseconds = (_uEnd - _uStartTick).GetMilliseconds();
                Strings::uString _szTmp;
                _szTmp.Format(L"第 %d次，预期延迟 %u ，实际延迟 %I64d，\n", i, kDelay, nMilliseconds);

                Assert::IsTrue(nMilliseconds <= (kDelay + kDeviation) && nMilliseconds >= (kDelay - 5), _szTmp);
            }
        }

        TEST_METHOD(周期性唤醒检查)
        {
            HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

            RefPtr<ThreadTaskRunner> _pTaskRunners[] = { ThreadTaskRunner::Create(false), ThreadTaskRunner::Create(true)};
            for (auto& _pTaskRunner : _pTaskRunners)
            {
                auto _uStartTick = TickCount<TimePrecise::Millisecond>::GetCurrent();

                volatile int nCount = 0;
                RefPtr<Timer> _pTimer;
                _pTimer = _pTaskRunner->CreateTimer(
                    TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                    [&nCount, _uStartTick, _hEvent]()
                    {
                        ++nCount;
                        if (nCount == 5)
                        {
                            SetEvent(_hEvent);
                            return false;
                        }
                        else
                        {
                            return true;
                        }
                    });


                WaitForSingleObject(_hEvent, 6000);
                Sleep(1000);
                Assert::AreEqual((int)nCount, 5);
                _pTimer = nullptr;
                _pTaskRunner = nullptr;
            }

            CloseHandle(_hEvent);
        }

        TEST_METHOD(Wait句柄测试)
        {
            RefPtr<ThreadTaskRunner> _pTaskRunners[] = { ThreadTaskRunner::Create(false), ThreadTaskRunner::Create(true) };
            for (auto& _pTaskRunner : _pTaskRunners)
            {
                HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

                for (int i = 0; i < 10; ++i)
                {
                    volatile uint32_t _uWaitResultCount = 0;
                    volatile UINT64 _uTickCount = 0;

                    _pTaskRunner->CreateWait(_hEvent, [&](DWORD _uWaitResultT)
                        {
                            if (_uWaitResultT == WAIT_OBJECT_0)
                            {
                                YY::Increment(&_uWaitResultCount);
                                _uTickCount = GetTickCount64();
                            }
                            return true;
                        });

                    Sleep(600);
                    SetEvent(_hEvent);
                    auto _uTickCountEnd = GetTickCount64();
                    Sleep(10);

                    Assert::IsTrue(abs((long long)(_uTickCountEnd - _uTickCount)) < 50);
                }

                CloseHandle(_hEvent);

                // 超多句柄等待情况测试
                {
                    HANDLE _hEvents[300];
                    volatile uint32_t _uWaitResultCount = 0;

                    for (auto& _hEvent : _hEvents)
                    {
                        _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                        _pTaskRunner->CreateWait(_hEvent, [&](DWORD _uWaitResultT)
                            {
                                if (_uWaitResultT == WAIT_OBJECT_0)
                                {
                                    YY::Increment(&_uWaitResultCount);
                                }
                                return true;
                            });
                    }

                    Sleep(100);
                    Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(0));

                    for (auto _hEvent : _hEvents)
                    {
                        SetEvent(_hEvent);
                    }

                    Sleep(100);
                    Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(std::size(_hEvents)));

                    for (auto _hEvent : _hEvents)
                    {
                        SetEvent(_hEvent);
                    }

                    Sleep(100);
                    Assert::AreEqual((uint32_t)_uWaitResultCount, uint32_t(std::size(_hEvents)));

                    for (auto _hEvent : _hEvents)
                    {
                        CloseHandle(_hEvent);
                    }
                }
            }
        }

        TEST_METHOD(Wait句柄超时测试)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            HANDLE _hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            volatile DWORD _uWaitResult = -1;
            auto _pWait = _pTaskRunner->CreateWait(
                _hEvent,
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                [&_uWaitResult](DWORD _uWaitResultT)
                {
                    _uWaitResult = _uWaitResultT;
                    return true;
                });

            Assert::IsTrue(((TaskEntry*)_pWait.Get())->Wait(600ul));
            Assert::AreEqual(DWORD(_uWaitResult), DWORD(WAIT_TIMEOUT));

            _pWait = _pTaskRunner->CreateWait(
                _hEvent,
                TimeSpan<TimePrecise::Millisecond>::FromMilliseconds(500),
                [&_uWaitResult](DWORD _uWaitResultT)
                {
                    _uWaitResult = _uWaitResultT;
                    return true;
                });
            SetEvent(_hEvent);
            Assert::IsTrue(((TaskEntry*)_pWait.Get())->Wait(100ul));
            CloseHandle(_hEvent);
        }
    };
} // namespace UnitTest
