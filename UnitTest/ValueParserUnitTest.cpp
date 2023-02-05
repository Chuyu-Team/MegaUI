#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/Parser/ValueParser.h>

#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <atltypes.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace YY::MegaUI;
using namespace YY::Base;
using namespace YY;

namespace UnitTest
{
    TEST_CLASS(ValueParserUnitTest)
    {
    public:
        TEST_METHOD(Parser整形)
        {
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"123"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::BaseIdentifier);
                Assert::IsTrue(_ExprNode.ChildExprNode[0].szValue == u8"123");
                
            }

        }

        TEST_METHOD(ParserOr行为)
        {
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"123 | 46 | 789"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");
                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::Or);

                auto& ChildExprNode = _ExprNode.ChildExprNode[0].ChildExprNode;

                Assert::AreEqual(ChildExprNode.GetSize(), uint_t(3), L"必须相等");

                Assert::IsTrue(ChildExprNode[0].szValue == u8"123");
                Assert::IsTrue(ChildExprNode[0].Type == ExprNodeType::BaseIdentifier);

                Assert::IsTrue(ChildExprNode[1].szValue == u8"46");
                Assert::IsTrue(ChildExprNode[1].Type == ExprNodeType::BaseIdentifier);

                Assert::IsTrue(ChildExprNode[2].szValue == u8"789");
                Assert::IsTrue(ChildExprNode[2].Type == ExprNodeType::BaseIdentifier);

            }
        }

        TEST_METHOD(Parser函数)
        {
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"Test()"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::Funcall);
                Assert::IsTrue(_ExprNode.ChildExprNode[0].szValue == u8"Test");
            }
            
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"Test(123, 456)"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::Funcall);
                Assert::IsTrue(_ExprNode.ChildExprNode[0].szValue == u8"Test");


                auto& ChildExprNode = _ExprNode.ChildExprNode[0].ChildExprNode;

                Assert::AreEqual(ChildExprNode.GetSize(), uint_t(2), L"必须相等");

                Assert::IsTrue(ChildExprNode[0].szValue == u8"123");
                Assert::IsTrue(ChildExprNode[0].Type == ExprNodeType::BaseIdentifier);

                Assert::IsTrue(ChildExprNode[1].szValue == u8"456");
                Assert::IsTrue(ChildExprNode[1].Type == ExprNodeType::BaseIdentifier);
            }
        }

        TEST_METHOD(Parser函数嵌套)
        {
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"Test(Test2())"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::Funcall);
                Assert::IsTrue(_ExprNode.ChildExprNode[0].szValue == u8"Test");

                auto& ChildExprNode = _ExprNode.ChildExprNode[0].ChildExprNode;

                Assert::AreEqual(ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(ChildExprNode[0].szValue == u8"Test2");
                Assert::IsTrue(ChildExprNode[0].Type == ExprNodeType::Funcall);
            }
            
            {
                ValueParser _ValueParser;
                ExprNode _ExprNode;

                auto _hr = _ValueParser.Parse(
                    u8StringView(u8"Test(123, Test2())"),
                    &_ExprNode);

                Assert::IsTrue(_hr >= 0);
                Assert::AreEqual(_ExprNode.ChildExprNode.GetSize(), uint_t(1), L"必须相等");

                Assert::IsTrue(_ExprNode.ChildExprNode[0].Type == ExprNodeType::Funcall);
                Assert::IsTrue(_ExprNode.ChildExprNode[0].szValue == u8"Test");

                auto& ChildExprNode = _ExprNode.ChildExprNode[0].ChildExprNode;

                Assert::AreEqual(ChildExprNode.GetSize(), uint_t(2), L"必须相等");
                
                Assert::IsTrue(ChildExprNode[0].szValue == u8"123");
                Assert::IsTrue(ChildExprNode[0].Type == ExprNodeType::BaseIdentifier);

                Assert::IsTrue(ChildExprNode[1].szValue == u8"Test2");
                Assert::IsTrue(ChildExprNode[1].Type == ExprNodeType::Funcall);
            }
        }
    };
} // namespace UnitTest