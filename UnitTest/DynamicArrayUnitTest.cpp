#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <Base/Containers/Array.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY::Base;
using namespace YY::Base::Containers;

namespace UnitTest
{
    TEST_CLASS(DynamicArrayUnitTest)
    {
    public:
        TEST_METHOD(列表初始化)
        {
            Array<int> _Data = {0, 1, 2, 3};
            Assert::AreEqual(_Data.GetSize(), size_t(4));

            for (int i = 0; i != 4; ++i)
                Assert::AreEqual(*_Data.GetItemPtr(i), i);
        }
        TEST_METHOD(Add)
        {
            Array<int> _Data;

            for (int i = 0; i != 100; ++i)
                Assert::IsTrue(SUCCEEDED(_Data.Add(i)));

            Assert::AreEqual(_Data.GetSize(), size_t(100));

            for (int i = 0; i != 100; ++i)
                Assert::AreEqual(*_Data.GetItemPtr(i), i);
        }

        TEST_METHOD(Insert)
        {
            Array<int> _Data;

            Assert::IsTrue(SUCCEEDED(_Data.Insert(0, 1)));
            Assert::AreEqual(_Data.GetSize(), size_t(1));
            Assert::AreEqual(*_Data.GetItemPtr(0), 1);
            
            Assert::IsTrue(SUCCEEDED(_Data.Insert(0, 2)));
            Assert::AreEqual(_Data.GetSize(), size_t(2));
            Assert::AreEqual(*_Data.GetItemPtr(0), 2);
            Assert::AreEqual(*_Data.GetItemPtr(1), 1);
            
            Assert::IsTrue(SUCCEEDED(_Data.Insert(2, 3)));
            Assert::AreEqual(_Data.GetSize(), size_t(3));
            Assert::AreEqual(*_Data.GetItemPtr(0), 2);
            Assert::AreEqual(*_Data.GetItemPtr(1), 1);
            Assert::AreEqual(*_Data.GetItemPtr(2), 3);


            Assert::IsTrue(FAILED(_Data.Insert(4, 4)));
            Assert::AreEqual(_Data.GetSize(), size_t(3));
            Assert::AreEqual(*_Data.GetItemPtr(0), 2);
            Assert::AreEqual(*_Data.GetItemPtr(1), 1);
            Assert::AreEqual(*_Data.GetItemPtr(2), 3);
        }

        TEST_METHOD(Remove)
        {
            {
                Array<int> _Data = {0, 1, 2, 3, 4, 5, 6};
                Assert::AreEqual(_Data.GetSize(), size_t(7));


                {
                    _Data.Remove(6);
                    const int Data[] = {0, 1, 2, 3, 4, 5};

                    Assert::AreEqual(_Data.GetSize(), _countof(Data));
                    for (int i = 0; i != _countof(Data); ++i)
                        Assert::AreEqual(*_Data.GetItemPtr(i), Data[i]);
                }

                {
                    _Data.Remove(1, 2);
                    const int Data[] = {0, 3, 4, 5};

                    Assert::AreEqual(_Data.GetSize(), _countof(Data));
                    for (int i = 0; i != _countof(Data); ++i)
                        Assert::AreEqual(*_Data.GetItemPtr(i), Data[i]);
                }

                {
                    _Data.Remove(0, 2);
                    const int Data[] = {4, 5};

                    Assert::AreEqual(_Data.GetSize(), _countof(Data));
                    for (int i = 0; i != _countof(Data); ++i)
                        Assert::AreEqual(*_Data.GetItemPtr(i), Data[i]);
                }
            }
        }

        TEST_METHOD(SetItem)
        {
            Array<int> _Data;
            Assert::IsTrue(FAILED(_Data.SetItem(0, 1)));
            Assert::AreEqual(_Data.GetSize(), size_t(0));

            Assert::IsTrue(SUCCEEDED(_Data.Insert(0, 1)));
            Assert::IsTrue(SUCCEEDED(_Data.SetItem(0, 2)));
            
            Assert::AreEqual(_Data.GetSize(), size_t(1));
            Assert::AreEqual(*_Data.GetItemPtr(0), 2);
        }

        TEST_METHOD(Clear)
        {
            Array<int> _Data;
            for (int i = 0; i != 100; ++i)
                Assert::IsTrue(SUCCEEDED(_Data.Add(i)));

            Assert::AreEqual(_Data.GetSize(), size_t(100));

            _Data.Clear();

            Assert::AreEqual(_Data.GetSize(), size_t(0));
        }

        TEST_METHOD(Resize)
        {
            Array<int> _Data;

            Assert::IsTrue(SUCCEEDED(_Data.Resize(0)));
            Assert::AreEqual(_Data.GetSize(), size_t(0));

            Assert::IsTrue(SUCCEEDED(_Data.Resize(3)));
            Assert::AreEqual(_Data.GetSize(), size_t(3));
            Assert::AreEqual(*_Data.GetItemPtr(0), 0);
            Assert::AreEqual(*_Data.GetItemPtr(1), 0);
            Assert::AreEqual(*_Data.GetItemPtr(2), 0);
        }

        TEST_METHOD(SetArray)
        {
            static const int _Src[] = {0, 1, 2, 3};
            Array<int> _Data;

            Assert::IsTrue(SUCCEEDED(_Data.SetArray(_Src, _countof(_Src))));
            
            Assert::AreEqual(_Data.GetSize(), _countof(_Src));

            for (int i = 0; i != _countof(_Src); ++i)
            {
                Assert::AreEqual(*_Data.GetItemPtr(i), _Src[i]);
            }
        }

        TEST_METHOD(写时复制机制测试)
        {
            // 写覆盖模式
            {
                Array<int> _Data;
                _Data.Add(1);

                auto _pDateBuffer = _Data.GetData();

                auto _Data2 = _Data;

                Assert::AreEqual(_Data.GetData(), _pDateBuffer);
                Assert::AreEqual(_Data2.GetData(), _pDateBuffer);

                _Data2.Add(2);

                Assert::AreEqual(_Data.GetData(), _pDateBuffer);
                Assert::AreNotEqual(_Data2.GetData(), _pDateBuffer);
            }

            // 关闭写覆盖模式
            {
                Array<int, AllocPolicy::SOO> _Data;
                _Data.Add(1);

                auto _pDateBuffer = _Data.GetData();

                auto _Data2 = _Data;

                Assert::AreEqual(_Data.GetData(), _pDateBuffer);
                Assert::AreNotEqual(_Data2.GetData(), _pDateBuffer);
            }
        }
    };
}