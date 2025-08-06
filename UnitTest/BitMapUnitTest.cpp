#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <YY/Base/Containers/BitMap.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY::Base;
using namespace YY::Base::Containers;

namespace UnitTest
{
    TEST_CLASS(BitMapUnitTest)
    {
    public:
        TEST_METHOD(Bit数检测)
        {
            static_assert(sizeof(BitMap<65>::arrBits) >= sizeof(uint32_t) * 3);

            static_assert(sizeof(BitMap<64>::arrBits) == sizeof(uint32_t) * 2);
        }

        TEST_METHOD(Bit设置获取能力)
        {
            BitMap<128> TTT;
            Assert::AreEqual(TTT.GetItem(126), false);
            Assert::AreEqual(TTT.GetSize(), 0u);

            TTT.SetItem(126, true);
            Assert::AreEqual(TTT.GetItem(126), true);
            Assert::AreEqual(TTT.GetSize(), 1u);
            Assert::AreEqual(((uint32_t*)&TTT.arrBits)[3], (uint32_t)0x40000000u);

            TTT.SetItem(126, false);
            Assert::AreEqual(TTT.GetItem(126), false);
            Assert::AreEqual(TTT.GetSize(), 0u);
            Assert::AreEqual(((uint32_t*)&TTT.arrBits)[3], (uint32_t)0x00000000u);
        }

        TEST_METHOD(Find验证)
        {
            {
                BitMap<128> TTT;
                for (int32_t i = 127; i >= 0; --i)
                {
                    TTT.SetItem(i, true);

                    Assert::AreEqual(TTT.Find(0), i);
                }
            }

            {
                BitMap<128> TTT;
                TTT.SetItem(127, true);

                Assert::AreEqual(TTT.Find(120), 127);
            }
        }
    };
}
