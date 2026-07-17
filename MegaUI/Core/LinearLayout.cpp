#include "pch.h"
#include <MegaUI/Core/LinearLayout.h>
#include <MegaUI/Core/Element.h>

#include <algorithm>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

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

Element* __YYAPI LinearLayout::GetAdjacent(Element* pElem, Element* _pFrom, NavigatingType _eNavigate, NavReference const* pnr, bool _bKeyableOnly)
{
    if (HasFlags(_eNavigate, NavigatingStyle::Logical))
        return nullptr;

    if (_pFrom == nullptr || _pFrom == pElem)
        return nullptr;

    size_t _cChildren = GetLayoutChildCount(pElem);

    ElementList _oChildrenList = pElem->GetChildren();

    for (size_t i = 0; i < _cChildren; i++)
    {
        auto _pChild = GetChildFromLayoutIndex(pElem, i, &_oChildrenList);
        if (!_pChild)
            continue;

        Element* _pTo = _pChild->GetAdjacent(nullptr, _eNavigate, pnr, _bKeyableOnly);
        if (_pTo)
        {
            return _pTo;
        }
    }

    return nullptr;
}

Size __YYAPI LinearLayout::UpdateDesiredSizeVertical(Element* pElem, Size _ConstraintSize)
{
    float widthDesired = 0;
    float heightDesired = 0;
    Rect prevMargin;

    auto cChildren = GetLayoutChildCount(pElem);
    for (size_t i = 0; i < cChildren; ++i)
    {
        auto pChild = GetChildFromLayoutIndex(pElem, static_cast<int>(i), nullptr);
        if (pChild == nullptr)
            continue;

        auto childDesired = UpdateChildDesiredSize(pChild, Size(_ConstraintSize.Width, -1));
        const auto& margin = GetChildMargin(pChild);

        // 交叉轴宽度 = max(childDesired.Width + margin.Left + margin.Right)
        auto childCrossW = childDesired.Width + margin.Left + margin.Right;
        if (childCrossW > widthDesired)
            widthDesired = childCrossW;

        // 主轴高度累加
        if (i == 0)
            heightDesired += margin.Top;
        else
            heightDesired += (std::max)(prevMargin.Bottom, margin.Top);

        heightDesired += childDesired.Height;
        prevMargin = margin;
    }

    // 末元素 Bottom margin
    if (cChildren > 0)
        heightDesired += prevMargin.Bottom;

    return Size(widthDesired, heightDesired);
}

Size __YYAPI LinearLayout::UpdateDesiredSizeHorizontal(Element* pElem, Size _ConstraintSize)
{
    float widthDesired = 0;
    float heightDesired = 0;
    Rect prevMargin;

    auto cChildren = GetLayoutChildCount(pElem);
    for (size_t i = 0; i < cChildren; ++i)
    {
        auto pChild = GetChildFromLayoutIndex(pElem, static_cast<int>(i), nullptr);
        if (pChild == nullptr)
            continue;

        auto childDesired = UpdateChildDesiredSize(pChild, Size(-1, _ConstraintSize.Height));
        const auto& margin = GetChildMargin(pChild);

        // 交叉轴高度
        auto childCrossH = childDesired.Height + margin.Top + margin.Bottom;
        if (childCrossH > heightDesired)
            heightDesired = childCrossH;

        // 主轴线宽度累加
        if (i == 0)
            widthDesired += margin.Left;
        else
            widthDesired += (std::max)(prevMargin.Right, margin.Left);

        widthDesired += childDesired.Width;
        prevMargin = margin;
    }

    // 末元素 Right margin
    if (cChildren > 0)
        widthDesired += prevMargin.Right;

    return Size(widthDesired, heightDesired);
}

/// <summary>
/// 沿垂直方向排列子元素，为每个子元素设置其在布局中的位置与尺寸。
/// </summary>
void __YYAPI LinearLayout::DoLayoutVertical(Element* pElem, Size _oSize)
{
    float yCurr = 0;
    Rect prevMargin;

    auto cChildren = GetLayoutChildCount(pElem);
    for (size_t i = 0; i < cChildren; ++i)
    {
        auto pChild = GetChildFromLayoutIndex(pElem, static_cast<int>(i), nullptr);
        if (pChild == nullptr)
            continue;

        const auto& margin = GetChildMargin(pChild);

        // 主轴 Y 位置
        if (i == 0)
            yCurr += margin.Top;
        else
            yCurr += (std::max)(prevMargin.Bottom, margin.Top);

        auto childDesired = GetChildDesiredSize(pChild);
        float childW = childDesired.Width;
        float childH = childDesired.Height;
        float childX = 0;

        // 交叉轴对齐
        switch (eCrossAlign)
        {
        case CrossAxisAlignment::Start:
            childX = margin.Left;
            break;
        case CrossAxisAlignment::Center:
        {
            childX = (_oSize.Width - childW) * 0.5f;
            if (childX < margin.Left)
                childX = margin.Left;
            if (childX + childW > _oSize.Width - margin.Right)
                childX = _oSize.Width - margin.Right - childW;
            if (childX < margin.Left)
                childX = margin.Left;
            break;
        }
        case CrossAxisAlignment::End:
            childX = _oSize.Width - childW - margin.Right;
            break;
        case CrossAxisAlignment::Stretch:
        {
            childX = margin.Left;
            childW = _oSize.Width - margin.Left - margin.Right;
            if (childW < 0)
                childW = 0;
            break;
        }
        }

        UpdateLayoutRect(pElem, _oSize, pChild, Point(childX, yCurr), Size(childW, childH));
        yCurr += childH;
        prevMargin = margin;
    }
}

/// <summary>
/// 沿水平方向排列子元素，为每个子元素设置其在布局中的位置与尺寸。
/// </summary>
void __YYAPI LinearLayout::DoLayoutHorizontal(Element* pElem, Size _oSize)
{
    float xCurr = 0;
    Rect prevMargin;

    auto cChildren = GetLayoutChildCount(pElem);
    for (size_t i = 0; i < cChildren; ++i)
    {
        auto pChild = GetChildFromLayoutIndex(pElem, static_cast<int>(i), nullptr);
        if (pChild == nullptr)
            continue;

        const auto& margin = GetChildMargin(pChild);

        // 主轴 X 位置
        if (i == 0)
            xCurr += margin.Left;
        else
            xCurr += (std::max)(prevMargin.Right, margin.Left);

        auto childDesired = GetChildDesiredSize(pChild);
        float childW = childDesired.Width;
        float childH = childDesired.Height;
        float childY = 0;

        // 交叉轴对齐
        switch (eCrossAlign)
        {
        case CrossAxisAlignment::Start:
            childY = margin.Top;
            break;
        case CrossAxisAlignment::Center:
        {
            childY = (_oSize.Height - childH) * 0.5f;
            if (childY < margin.Top)
                childY = margin.Top;
            if (childY + childH > _oSize.Height - margin.Bottom)
                childY = _oSize.Height - margin.Bottom - childH;
            if (childY < margin.Top)
                childY = margin.Top;
            break;
        }
        case CrossAxisAlignment::End:
            childY = _oSize.Height - childH - margin.Bottom;
            break;
        case CrossAxisAlignment::Stretch:
        {
            childY = margin.Top;
            childH = _oSize.Height - margin.Top - margin.Bottom;
            if (childH < 0)
                childH = 0;
            break;
        }
        }

        UpdateLayoutRect(pElem, _oSize, pChild, Point(xCurr, childY), Size(childW, childH));
        xCurr += childW;
        prevMargin = margin;
    }
}

} // namespace MegaUI
} // namespace YY
