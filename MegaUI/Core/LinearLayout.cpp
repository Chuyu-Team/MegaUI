#include "pch.h"
#include <MegaUI/Core/LinearLayout.h>

namespace YY {
namespace MegaUI {

HRESULT __YYAPI LinearLayout::Create(
    LinearLayoutOrientation _eOrientation,
    CrossAxisAlignment _eCrossAlign,
    Layout** _ppLayout)
{
    if (_ppLayout == nullptr)
        return E_POINTER;

    auto pLayout = new (std::nothrow) LinearLayout();
    if (pLayout == nullptr)
        return E_OUTOFMEMORY;

    pLayout->eOrientation = _eOrientation;
    pLayout->eCrossAlign = _eCrossAlign;

    *_ppLayout = pLayout;
    return S_OK;
}

LinearLayoutOrientation __YYAPI LinearLayout::GetOrientation() const
{
    return eOrientation;
}

CrossAxisAlignment __YYAPI LinearLayout::GetCrossAxisAlignment() const
{
    return eCrossAlign;
}

void __YYAPI LinearLayout::DoLayout(Element* pElem, Size _oSize)
{
    if (pElem == nullptr)
        return;

    if (eOrientation == LinearLayoutOrientation::Vertical)
        DoLayoutVertical(pElem, _oSize);
    else
        DoLayoutHorizontal(pElem, _oSize);
}

Size __YYAPI LinearLayout::UpdateDesiredSize(Element* pElem, Size _ConstraintSize)
{
    if (pElem == nullptr)
        return Size(0, 0);

    if (eOrientation == LinearLayoutOrientation::Vertical)
        return UpdateDesiredSizeVertical(pElem, _ConstraintSize);
    else
        return UpdateDesiredSizeHorizontal(pElem, _ConstraintSize);
}

Size __YYAPI LinearLayout::UpdateDesiredSizeVertical(Element* pElem, Size _ConstraintSize)
{
    // 骨架：待 T6 实现
    return Size(0, 0);
}

Size __YYAPI LinearLayout::UpdateDesiredSizeHorizontal(Element* pElem, Size _ConstraintSize)
{
    // 骨架：待 T6 实现
    return Size(0, 0);
}

void __YYAPI LinearLayout::DoLayoutVertical(Element* pElem, Size _oSize)
{
    // 骨架：待 T7 实现
}

void __YYAPI LinearLayout::DoLayoutHorizontal(Element* pElem, Size _oSize)
{
    // 骨架：待 T7 实现
}
}
} // namespace YY
