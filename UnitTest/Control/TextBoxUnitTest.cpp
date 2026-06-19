#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/Control/TextBox.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace YY::MegaUI;
using namespace YY::Base;
using namespace YY;

namespace Control
{
    namespace
    {
        class TestTextBox : public TextBox
        {
        public:
            bool SendKeyDown(u16char_t _uVKey, EventModifier _fModifiers = EventModifier::None)
            {
                return OnKeyDown(KeyboardEvent(this, PlatformEvent{ nullptr, WM_KEYDOWN, _uVKey, 0 }, _fModifiers));
            }

            bool __YYAPI InsertText(const uString& _szText)
            {
                return TextBox::InsertText(_szText);
            }

            bool __YYAPI DeleteBackward()
            {
                return TextBox::DeleteBackward();
            }

            bool __YYAPI DeleteForward()
            {
                return TextBox::DeleteForward();
            }
        };

        static uString MakeText(_In_z_ const wchar_t* _szText)
        {
            return uString(reinterpret_cast<const u16char_t*>(_szText));
        }

        static uString GetTextBoxContent(TextBox& _TextBox)
        {
            auto _Value = _TextBox.GetValue(Element::g_ControlInfoData.ContentProp);
            return _Value.GetType() == ValueType::String ? _Value.GetString() : uString();
        }

        static bool IsTextEqual(TextBox& _TextBox, _In_z_ const wchar_t* _szText)
        {
            return GetTextBoxContent(_TextBox) == MakeText(_szText);
        }
    }

    TEST_CLASS(TextBoxUnitTest)
    {
    public:
        TEST_METHOD(连续Backspace应删除到空)
        {
            Assert::IsTrue(SUCCEEDED(TextBox::Register()));

            TestTextBox _TextBox;
            Assert::IsTrue(_TextBox.InsertText(MakeText(L"abcd")));
            Assert::AreEqual(size_t(4), GetTextBoxContent(_TextBox).GetSize());

            Assert::IsTrue(_TextBox.DeleteBackward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"abc"));

            Assert::IsTrue(_TextBox.DeleteBackward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"ab"));

            Assert::IsTrue(_TextBox.DeleteBackward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"a"));

            Assert::IsTrue(_TextBox.DeleteBackward());
            Assert::IsTrue(GetTextBoxContent(_TextBox).IsEmpty());

            Assert::IsFalse(_TextBox.DeleteBackward());
            Assert::IsTrue(GetTextBoxContent(_TextBox).IsEmpty());
        }

        TEST_METHOD(代理对Backspace应一次删除完整字符)
        {
            Assert::IsTrue(SUCCEEDED(TextBox::Register()));

            TestTextBox _TextBox;
            Assert::IsTrue(_TextBox.InsertText(MakeText(L"a𤭢")));

            Assert::IsTrue(_TextBox.DeleteBackward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"a"));
        }

        TEST_METHOD(DeleteForward应删除下一个字符)
        {
            Assert::IsTrue(SUCCEEDED(TextBox::Register()));

            TestTextBox _TextBox;
            Assert::IsTrue(_TextBox.InsertText(MakeText(L"abcd")));
            Assert::IsTrue(_TextBox.SendKeyDown(VK_HOME));

            Assert::IsTrue(_TextBox.DeleteForward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"bcd"));
        }

        TEST_METHOD(DeleteForward遇到代理对应一次删除完整字符)
        {
            Assert::IsTrue(SUCCEEDED(TextBox::Register()));

            TestTextBox _TextBox;
            Assert::IsTrue(_TextBox.InsertText(MakeText(L"𤭢b")));
            Assert::IsTrue(_TextBox.SendKeyDown(VK_HOME));

            Assert::IsTrue(_TextBox.DeleteForward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"b"));
        }

        TEST_METHOD(只读模式下Backspace不应修改文本)
        {
            Assert::IsTrue(SUCCEEDED(TextBox::Register()));

            TestTextBox _TextBox;
            Assert::IsTrue(_TextBox.InsertText(MakeText(L"abcd")));
            Assert::IsTrue(SUCCEEDED(_TextBox.SetReadOnly(true)));
            Assert::IsTrue(_TextBox.IsReadOnly());

            Assert::IsFalse(_TextBox.DeleteBackward());
            Assert::IsTrue(IsTextEqual(_TextBox, L"abcd"));
        }
    };
} // namespace UnitTest
