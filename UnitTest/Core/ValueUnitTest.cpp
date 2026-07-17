#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/Core/Layout.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace YY::MegaUI;

namespace Core {

class TestLayout : public Layout
{
public:
    void __YYAPI DoLayout(Element* pElem, YY::Size _oSize) override
    {
    }

    // 1
    YY::Size __YYAPI UpdateDesiredSize(Element* pElem, YY::Size _ConstraintSize) override
    {
        return YY::Size(0, 0);
    }

    Element* __YYAPI GetAdjacent(Element* pElem, Element* _pFrom, NavigatingType _eNavigate, NavReference const* pnr, bool _bKeyableOnly) override
    {
        return nullptr;
    }
};

TEST_CLASS(ValueUnitTest)
{
public:

    TEST_METHOD(CreateLayout_非nullptr输入_Type为Layout且指针可恢复)
    {
        auto _pLayout = YY::RefPtr<TestLayout>::Create();

        Value v = Value::CreateLayout(_pLayout);
        Assert::IsTrue(v.GetType() == ValueType::Layout);

        Assert::AreEqual((intptr_t)_pLayout.Get(), (intptr_t)v.GetLayout().Get());
    }

    TEST_METHOD(CreateLayout_nullptr输入_Type为Layout指针为nullptr)
    {
        Value v = Value::CreateLayout(nullptr);
        Assert::IsTrue(v.GetType() == ValueType::Layout);
        Assert::IsNull(v.GetLayout().Get());
    }
};
} // namespace Core
