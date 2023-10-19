#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <Base/Threading/TaskRunner.h>
#include <Base/Threading/ProcessThreads.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY::Base;
using namespace YY::Base::Threading;

namespace UnitTest
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
        TEST_METHOD(后台模式检测)
        {
            auto _pTaskRunner = ThreadTaskRunner::Create(true);

            volatile uint32_t _uCount2 = 0;

            _pTaskRunner->PostTask([&_uCount2]()
                {
                    _uCount2 = 1;
                });

            Sleep(100);

            Assert::AreEqual((uint32_t)_uCount2, 1u);

            _pTaskRunner->PostTask([&_uCount2]()
                {
                    _uCount2 = 2;
                });

            Sleep(100);

            Assert::AreEqual((uint32_t)_uCount2, 2u);

            _pTaskRunner->PostTask([&_uCount2]()
                {
                    _uCount2 = 3;
                });

            Sleep(100);

            Assert::AreEqual((uint32_t)_uCount2, 3u);
        }


        TEST_METHOD(线程Id获取)
        {
            auto _pTaskRunner = ThreadTaskRunner::Create(true);

            auto _oId = _pTaskRunner->GetThreadId();
            decltype(_oId) _oId2 = 0xCC;


            _pTaskRunner->PostTask([&_oId2]()
                {
                    _oId2 = Threading::GetCurrentThreadId();
                });

            Sleep(10);

            Assert::AreEqual(_oId, _oId2);
        }
    };
} // namespace UnitTest
