#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/Core/LinearLayout.h>
#include <MegaUI/Core/Element.h>
#include <MegaUI/Core/Layout.h>
#include <MegaUI/Core/Value.h>
#include <MegaUI/Parser/UIParser.h>
#include "../MegaUI/Parser/ValueParser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace YY::MegaUI;
using namespace YY;

namespace LinearLayoutTest
{
    /// <summary>
    /// 辅助函数：创建 LinearLayout 实例并返回指针。
    /// </summary>
    static LinearLayout* CreateLinearLayoutHelper(LinearLayoutOrientation _eOri, CrossAxisAlignment _eCross)
    {
        Layout* pLayout = nullptr;
        HRESULT hr = LinearLayout::Create(_eOri, _eCross, &pLayout);
        Assert::AreEqual(S_OK, hr);
        Assert::IsNotNull(pLayout);
        return static_cast<LinearLayout*>(pLayout);
    }

    /// <summary>
    /// 辅助函数：创建并初始化一个 Element，设置指定 DesiredSize。
    /// </summary>
    static Element* CreateElementWithDesiredSize(float _w, float _h, Element* _pParent = nullptr)
    {
        UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
        intptr_t cookie = 0;
        Element* pElem = nullptr;
        HRESULT hr = Element::Create(oUnitMetrics, 0u, _pParent, &cookie, &pElem);
        Assert::AreEqual(S_OK, hr);
        Assert::IsNotNull(pElem);

        // 直接修改 LocDesiredSize（测试中绕过完整布局计算）
        pElem->LocDesiredSize = Size(_w, _h);
        return pElem;
    }

    TEST_CLASS(LinearLayoutUnitTest)
    {
    public:
        // ===========================================
        // Create 工厂 + Getter（TC-001 ~ TC-005）
        // ===========================================

        TEST_METHOD(Create_默认参数_创建成功且Getter返回值一致)
        {
            Layout* pLayout = nullptr;
            HRESULT hr = LinearLayout::Create(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch,
                &pLayout);
            Assert::AreEqual(S_OK, hr);
            Assert::IsNotNull(pLayout);

            auto pLL = static_cast<LinearLayout*>(pLayout);
            Assert::IsTrue(pLL->GetOrientation() == LinearLayoutOrientation::Vertical);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Stretch);

            pLayout->Destroy();
        }

        TEST_METHOD(Create_Horizontal方向_返回Horizontal)
        {
            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Horizontal,
                CrossAxisAlignment::Stretch);
            Assert::IsTrue(pLL->GetOrientation() == LinearLayoutOrientation::Horizontal);
            pLL->Destroy();
        }

        TEST_METHOD(Create_交叉轴Start对齐_返回Start)
        {
            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Start);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Start);
            pLL->Destroy();
        }

        TEST_METHOD(Create_交叉轴Center对齐_返回Center)
        {
            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Center);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Center);
            pLL->Destroy();
        }

        TEST_METHOD(Create_ppLayout为nullptr_返回E_POINTER)
        {
            HRESULT hr = LinearLayout::Create(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch,
                nullptr);
            Assert::AreEqual(E_POINTER, hr);
        }

        // ===========================================
        // GetDesiredSize（TC-011）
        // ===========================================

        TEST_METHOD(GetDesiredSize_返回LocDesiredSize)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pElem = nullptr;
            HRESULT hr = Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pElem);
            Assert::AreEqual(S_OK, hr);
            Assert::IsNotNull(pElem);

            pElem->LocDesiredSize = Size(100, 50);
            Assert::AreEqual(100.0f, pElem->GetDesiredSize().Width);
            Assert::AreEqual(50.0f, pElem->GetDesiredSize().Height);

            pElem->Destroy(false);
        }

        // ===========================================
        // UpdateDesiredSize - 垂直模式（TC-012 ~ TC-018）
        // ===========================================

        TEST_METHOD(UpdateDesiredSize_垂直空子元素_返回SizeZero)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            Size result = pLL->UpdateDesiredSize(pContainer, Size(400, -1));
            Assert::AreEqual(0.0f, result.Width);
            Assert::AreEqual(0.0f, result.Height);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(UpdateDesiredSize_垂直单子无Margin_返回子元素DesiredSize)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            // 创建子元素
            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(100, 50);
            pContainer->Add(pChild);

            Size result = pLL->UpdateDesiredSize(pContainer, Size(400, -1));
            Assert::AreEqual(100.0f, result.Width);
            Assert::AreEqual(50.0f, result.Height);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(UpdateDesiredSize_垂直单子带Margin_宽含Margin左右高含Margin上下)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(100, 50);
            pChild->CmpMargin = Rect(5, 10, 5, 10);
            pContainer->Add(pChild);

            Size result = pLL->UpdateDesiredSize(pContainer, Size(400, -1));
            // Width = 100 + 5 + 5 = 110
            Assert::AreEqual(110.0f, result.Width);
            // Height = 10 + 50 + 10 = 70
            Assert::AreEqual(70.0f, result.Height);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(UpdateDesiredSize_垂直三子带Margin_高度累加取max间距)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            // A: h=50, mb=5  (mt=10, 但首个取自己的 margin.top)
            // B: h=80, mt=15, mb=0
            // C: h=40, mt=0, mb=8

            Element* pA = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pA);
            pA->LocDesiredSize = Size(100, 50);
            pA->CmpMargin = Rect(0, 10, 0, 5);
            pContainer->Add(pA);

            Element* pB = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pB);
            pB->LocDesiredSize = Size(200, 80);
            pB->CmpMargin = Rect(0, 15, 0, 0);
            pContainer->Add(pB);

            Element* pC = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pC);
            pC->LocDesiredSize = Size(150, 40);
            pC->CmpMargin = Rect(0, 0, 0, 8);
            pContainer->Add(pC);

            Size result = pLL->UpdateDesiredSize(pContainer, Size(400, -1));
            // Height = 10(A.mt) + 50 + max(5,15) + 80 + max(0,0) + 40 + 8(C.mb) = 203
            Assert::AreEqual(203.0f, result.Height);
            // Width = max(100, 200, 150) = 200
            Assert::AreEqual(200.0f, result.Width);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(UpdateDesiredSize_垂直子元素LP_None全部跳过_返回SizeZero)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            // 添加 LP_None 子元素
            Element* pChild1 = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild1);
            pChild1->LocDesiredSize = Size(100, 50);
            pChild1->iSpecLayoutPos = LP_None;
            pContainer->Add(pChild1);

            Element* pChild2 = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild2);
            pChild2->LocDesiredSize = Size(200, 80);
            pChild2->iSpecLayoutPos = LP_None;
            pContainer->Add(pChild2);

            Size result = pLL->UpdateDesiredSize(pContainer, Size(400, -1));
            Assert::AreEqual(0.0f, result.Width);
            Assert::AreEqual(0.0f, result.Height);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        // ===========================================
        // DoLayout - 垂直模式（核心对齐测试）
        // ===========================================

        TEST_METHOD(DoLayout_垂直Stretch对齐_宽度拉伸填满Margin内空间)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 30);
            pChild->CmpMargin = Rect(10, 0, 5, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(400, 200));

            // Stretch: X = margin.Left = 10
            Assert::AreEqual(10.0f, pChild->LocPosInLayout.X);
            // W = container.W - margin.Left - margin.Right = 400 - 10 - 5 = 385
            Assert::AreEqual(385.0f, pChild->LocSizeInLayout.Width);
            Assert::AreEqual(30.0f, pChild->LocSizeInLayout.Height);
            Assert::AreEqual(0.0f, pChild->LocPosInLayout.Y); // margin.Top=0

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_垂直Start对齐_靠左贴MarginLeft)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Start);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 30);
            pChild->CmpMargin = Rect(10, 0, 5, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(400, 200));

            Assert::AreEqual(10.0f, pChild->LocPosInLayout.X);
            Assert::AreEqual(50.0f, pChild->LocSizeInLayout.Width); // 不拉伸

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_垂直Center对齐_X在容器中水平居中)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Center);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(100, 30);
            pChild->CmpMargin = Rect(0, 0, 0, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(400, 200));

            // Center: X = (400 - 100) / 2 = 150
            Assert::AreEqual(150.0f, pChild->LocPosInLayout.X);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_垂直End对齐_靠右贴MarginRight)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::End);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(100, 30);
            pChild->CmpMargin = Rect(0, 0, 20, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(400, 200));

            // End: X = 400 - 100 - 20 = 280
            Assert::AreEqual(280.0f, pChild->LocPosInLayout.X);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_垂直Stretch_容器不足以容纳Margin时宽度为0)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 30);
            pChild->CmpMargin = Rect(300, 0, 200, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(400, 200));

            // W = max(0, 400 - 300 - 200) = 0
            Assert::AreEqual(0.0f, pChild->LocSizeInLayout.Width);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_垂直三子Margin间距_位置正确累加)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Start);
            pLL->Attach(pContainer);

            // A: h=50, mb=5
            Element* pA = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pA);
            pA->LocDesiredSize = Size(100, 50);
            pA->CmpMargin = Rect(0, 0, 0, 5);
            pContainer->Add(pA);

            // B: h=80, mt=15
            Element* pB = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pB);
            pB->LocDesiredSize = Size(100, 80);
            pB->CmpMargin = Rect(0, 15, 0, 0);
            pContainer->Add(pB);

            // C: h=40, mt=0
            Element* pC = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pC);
            pC->LocDesiredSize = Size(100, 40);
            pC->CmpMargin = Rect(0, 0, 0, 0);
            pContainer->Add(pC);

            pLL->DoLayout(pContainer, Size(400, 300));

            // A.Y = A.mt = 0
            Assert::AreEqual(0.0f, pA->LocPosInLayout.Y);
            // B.Y = A.Y + A.h + max(A.mb, B.mt) = 0 + 50 + max(5, 15) = 65
            Assert::AreEqual(65.0f, pB->LocPosInLayout.Y);
            // C.Y = B.Y + B.h + max(B.mb, C.mt) = 65 + 80 + max(0, 0) = 145
            Assert::AreEqual(145.0f, pC->LocPosInLayout.Y);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        // ===========================================
        // DoLayout - 水平模式
        // ===========================================

        TEST_METHOD(DoLayout_水平Stretch对齐_高度拉伸填满Margin内空间)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Horizontal,
                CrossAxisAlignment::Stretch);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 30);
            pChild->CmpMargin = Rect(0, 10, 0, 5);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(200, 300));

            // Stretch: Y = margin.Top = 10
            Assert::AreEqual(10.0f, pChild->LocPosInLayout.Y);
            // H = container.H - margin.Top - margin.Bottom = 300 - 10 - 5 = 285
            Assert::AreEqual(285.0f, pChild->LocSizeInLayout.Height);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_水平Center对齐_Y在容器中垂直居中)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Horizontal,
                CrossAxisAlignment::Center);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 100);
            pChild->CmpMargin = Rect(0, 0, 0, 0);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(200, 300));

            // Center: Y = (300 - 100) / 2 = 100
            Assert::AreEqual(100.0f, pChild->LocPosInLayout.Y);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        TEST_METHOD(DoLayout_水平End对齐_靠底贴MarginBottom)
        {
            UnitMetrics oUnitMetrics = UnitMetrics::GetSystemMetrics();
            intptr_t cookie = 0;
            Element* pContainer = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pContainer);

            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Horizontal,
                CrossAxisAlignment::End);
            pLL->Attach(pContainer);

            Element* pChild = nullptr;
            Element::Create(oUnitMetrics, 0u, nullptr, &cookie, &pChild);
            pChild->LocDesiredSize = Size(50, 100);
            pChild->CmpMargin = Rect(0, 0, 0, 20);
            pContainer->Add(pChild);

            pLL->DoLayout(pContainer, Size(200, 300));

            // End: Y = 300 - 100 - 20 = 180
            Assert::AreEqual(180.0f, pChild->LocPosInLayout.Y);

            pLL->Detach(pContainer);
            pLL->Destroy();
            pContainer->Destroy(false);
        }

        // ===========================================
        // Value::CreateLayout
        // ===========================================

        TEST_METHOD(CreateLayout_非nullptr输入_Type为Layout且指针可恢复)
        {
            auto pLL = CreateLinearLayoutHelper(
                LinearLayoutOrientation::Vertical,
                CrossAxisAlignment::Stretch);

            Value v = Value::CreateLayout(pLL);
            Assert::IsTrue(v.GetType() == ValueType::Layout);

            auto pRecovered = static_cast<Layout*>(v.GetRawBuffer());
            Assert::IsTrue(pRecovered == static_cast<Layout*>(pLL));

            pLL->Destroy();
        }

        TEST_METHOD(CreateLayout_nullptr输入_Type为Layout指针为nullptr)
        {
            Value v = Value::CreateLayout(nullptr);
            Assert::IsTrue(v.GetType() == ValueType::Layout);
            Assert::IsNull(static_cast<Layout*>(v.GetRawBuffer()));
        }

        // ===========================================
        // UIParser ParserLayoutValue
        // ===========================================

        TEST_METHOD(ParserLayoutValue_无参默认_创建VerticalStretch实例)
        {
            // 构建 "LinearLayout" ExprNode
            ValueParser valueParser;
            ExprNode exprNode;
            HRESULT hr = valueParser.Parse(u8StringView(u8"LinearLayout"), &exprNode);
            Assert::AreEqual(S_OK, hr);

            Value v;
            hr = UIParser::ParserLayoutValue(exprNode.ChildExprNode.GetData(), &v);
            Assert::AreEqual(S_OK, hr);

            auto pLayout = static_cast<Layout*>(v.GetRawBuffer());
            Assert::IsNotNull(pLayout);

            auto pLL = static_cast<LinearLayout*>(pLayout);
            Assert::IsTrue(pLL->GetOrientation() == LinearLayoutOrientation::Vertical);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Stretch);

            pLL->Destroy();
        }

        TEST_METHOD(ParserLayoutValue_单参Horizontal_创建HorizontalStretch实例)
        {
            ValueParser valueParser;
            ExprNode exprNode;
            HRESULT hr = valueParser.Parse(u8StringView(u8"LinearLayout(Horizontal)"), &exprNode);
            Assert::AreEqual(S_OK, hr);

            Value v;
            hr = UIParser::ParserLayoutValue(exprNode.ChildExprNode.GetData(), &v);
            Assert::AreEqual(S_OK, hr);

            auto pLL = static_cast<LinearLayout*>(static_cast<Layout*>(v.GetRawBuffer()));
            Assert::IsTrue(pLL->GetOrientation() == LinearLayoutOrientation::Horizontal);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Stretch);

            pLL->Destroy();
        }

        TEST_METHOD(ParserLayoutValue_双参VerticalCenter_创建VerticalCenter实例)
        {
            ValueParser valueParser;
            ExprNode exprNode;
            HRESULT hr = valueParser.Parse(u8StringView(u8"LinearLayout(Vertical, Center)"), &exprNode);
            Assert::AreEqual(S_OK, hr);

            Value v;
            hr = UIParser::ParserLayoutValue(exprNode.ChildExprNode.GetData(), &v);
            Assert::AreEqual(S_OK, hr);

            auto pLL = static_cast<LinearLayout*>(static_cast<Layout*>(v.GetRawBuffer()));
            Assert::IsTrue(pLL->GetOrientation() == LinearLayoutOrientation::Vertical);
            Assert::IsTrue(pLL->GetCrossAxisAlignment() == CrossAxisAlignment::Center);

            pLL->Destroy();
        }

        TEST_METHOD(ParserLayoutValue_未知布局名_返回BAD_FORMAT)
        {
            ValueParser valueParser;
            ExprNode exprNode;
            HRESULT hr = valueParser.Parse(u8StringView(u8"UnknownLayout"), &exprNode);
            Assert::AreEqual(S_OK, hr);

            Value v;
            hr = UIParser::ParserLayoutValue(exprNode.ChildExprNode.GetData(), &v);
            Assert::AreEqual(HRESULT_From_LSTATUS(ERROR_BAD_FORMAT), hr);
        }
    };
}
