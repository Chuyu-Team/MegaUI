#include "pch.h"
#include "CppUnitTest.h"
#include <MegaUI/Core/Layout.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Control {

using namespace YY::MegaUI;

TEST_CLASS(ElementUntiTest)
{
public:
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

        pElem->LocDesiredSize = YY::Size(100, 50);
        Assert::AreEqual(100.0f, pElem->GetDesiredSize().Width);
        Assert::AreEqual(50.0f, pElem->GetDesiredSize().Height);

        pElem->Destroy(false);
    }
};

} // namespace Control
