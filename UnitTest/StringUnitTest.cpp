#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/base/String.h>
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
	TEST_CLASS(StringUnitTest)
	{
	public:
		
		TEST_METHOD(从原生指针构造)
		{
			{
#define _TEST_TEXT "从原生指针构造"

				YY::MegaUI::aString Tmp(_TEST_TEXT);
				Assert::AreEqual(Tmp.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(memcmp(Tmp.GetConstString(), _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);



				YY::MegaUI::String Tmp2(_TEST_TEXT);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(Tmp2.GetEncoding() == YY::MegaUI::Encoding::ANSI_DEFAULT);
				Assert::IsTrue(memcmp(*(void**)&Tmp2, _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);
#undef _TEST_TEXT
			}

			{
#define _TEST_TEXT _U8S("从原生指针构造")

				YY::MegaUI::u8String Tmp(_TEST_TEXT);
				Assert::AreEqual(Tmp.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(memcmp(Tmp.GetConstString(), _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);


				YY::MegaUI::String Tmp2(_TEST_TEXT);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(Tmp2.GetEncoding() == YY::MegaUI::Encoding::UTF8);
				Assert::IsTrue(memcmp(*(void**)&Tmp2, _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);
#undef _TEST_TEXT
			}

			{
#define _TEST_TEXT _U16S("从原生指针构造")

				YY::MegaUI::u16String Tmp(_TEST_TEXT);
				Assert::AreEqual(Tmp.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(memcmp(Tmp.GetConstString(), _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);


				YY::MegaUI::String Tmp2(_TEST_TEXT);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(Tmp2.GetEncoding() == YY::MegaUI::Encoding::UTF16);
				Assert::IsTrue(memcmp(*(void**)&Tmp2, _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);
#undef _TEST_TEXT
			}

			{
#define _TEST_TEXT _U32S("从原生指针构造")

				YY::MegaUI::u32String Tmp(_TEST_TEXT);
				Assert::AreEqual(Tmp.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(memcmp(Tmp.GetConstString(), _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);


				YY::MegaUI::String Tmp2(_TEST_TEXT);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_TEST_TEXT) - 1);
				Assert::IsTrue(Tmp2.GetEncoding() == YY::MegaUI::Encoding::UTF32);
				Assert::IsTrue(memcmp(*(void**)&Tmp2, _TEST_TEXT, sizeof(_TEST_TEXT)) == 0);
#undef _TEST_TEXT
			}
		}


		TEST_METHOD(引用计数能力验证)
		{
			YY::MegaUI::u16String Tmp(_U16S("一段测试文本"));

			auto p1 = Tmp.GetConstString();

			YY::MegaUI::u16String Tmp2 = Tmp;

			auto p2 = Tmp.GetConstString();
			auto p3 = Tmp2.GetConstString();


			Assert::IsTrue(p1 == p2);
			Assert::IsTrue(p1 == p3);
		}

		TEST_METHOD(LockBuffer复制验证)
		{
			// 这个缓冲区只共享一份，所以 LockBuffer，前后指针不变
			{
				YY::MegaUI::u16String Tmp(_U16S("一段测试文本"));
				auto p1 = Tmp.GetConstString();

				auto p2 = Tmp.LockBuffer(Tmp.GetSize());
				Tmp.UnlockBuffer(Tmp.GetSize());

				Assert::AreEqual(Tmp.GetSize(), _countof(_U16S("一段测试文本")) - 1);
				Assert::IsTrue(memcmp(Tmp.GetConstString(), _U16S("一段测试文本"), sizeof(_U16S("一段测试文本"))) == 0);

				Assert::IsTrue(p1 == p2);
			}


			// 缓冲区存在，共享，所以 LockBuffer 后指针会重新开辟
			{
				YY::MegaUI::u16String Tmp(_U16S("一段测试文本"));
				auto p1 = Tmp.GetConstString();

				YY::MegaUI::u16String Tmp2 = Tmp;
				auto p2 = Tmp2.LockBuffer(Tmp.GetSize());
				Tmp2.UnlockBuffer(Tmp.GetSize());

				Assert::AreEqual(Tmp.GetSize(), _countof(_U16S("一段测试文本")) - 1);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_U16S("一段测试文本")) - 1);

				Assert::IsTrue(memcmp(Tmp.GetConstString(), _U16S("一段测试文本"), sizeof(_U16S("一段测试文本"))) == 0);
				Assert::IsTrue(memcmp(Tmp2.GetConstString(), _U16S("一段测试文本"), sizeof(_U16S("一段测试文本"))) == 0);

				Assert::IsTrue(p1 != p2);
			}

		}

		TEST_METHOD(写复制能力验证)
		{
			{
				YY::MegaUI::u16String Tmp(_U16S("一段测试文本"));
				auto p1 = Tmp.GetConstString();

				YY::MegaUI::u16String Tmp2 = Tmp;
				Tmp2 += _U16S("2");
				auto p2 = Tmp.GetConstString();
				auto p3 = Tmp2.GetConstString();

				Assert::AreEqual(Tmp.GetSize(), _countof(_U16S("一段测试文本")) - 1);
				Assert::AreEqual(Tmp2.GetSize(), _countof(_U16S("一段测试文本2")) - 1);

				Assert::IsTrue(memcmp(Tmp.GetConstString(), _U16S("一段测试文本"), sizeof(_U16S("一段测试文本"))) == 0);
				Assert::IsTrue(memcmp(Tmp2.GetConstString(), _U16S("一段测试文本2"), sizeof(_U16S("一段测试文本2"))) == 0);

				Assert::IsTrue(p1 == p2);
				Assert::IsTrue(p1 != p3);
			}
		}

		
	};
}
