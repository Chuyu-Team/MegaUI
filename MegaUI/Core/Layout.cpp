#include <MegaUI/Core/Layout.h>
#include <MegaUI/Core/Element.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY {
namespace MegaUI {

void __YYAPI Layout::OnAdd(Element* pElem, Element** ppElemAdd, size_t uCountOfElementAdd)
{
    bool fReSort = false;
    for (size_t i = 0; i != uCountOfElementAdd; ++i)
    {
        const auto _iLayoutPos = ppElemAdd[i]->GetLayoutPos();
        if (_iLayoutPos == LP_Absolute || _iLayoutPos == LP_None)
        {
            if (IgnoreList.FindItemIndex(ppElemAdd[i]) == ElementList::uInvalidIndex)
            {
                IgnoreList.Add(ppElemAdd[i]);
                fReSort = true;
            }
        }
    }

    if (fReSort)
    {
        ReSortIgnoreList();
    }
}

void __YYAPI Layout::OnRemove(Element* pElem, Element** ppElemRemove, size_t uCountOfElementRemove)
{
    bool fReSort = false;

    for (UINT i = 0; i < uCountOfElementRemove; i++)
    {
        const auto dLayoutPos = ppElemRemove[i]->GetLayoutPos();

        // Check if was ignored
        if (dLayoutPos == LP_Absolute || dLayoutPos == LP_None)
        {
            auto _iItemIndex = IgnoreList.FindItemIndex(ppElemRemove[i]);
            if (_iItemIndex != ElementList::uInvalidIndex)
            {
                IgnoreList.Remove(_iItemIndex);
                fReSort = true;
            }
        }
    }

    if (fReSort)
    {
        ReSortIgnoreList();
    }
}

void __YYAPI Layout::OnLayoutPosChanged(Element* pElem1, Element* pElemChanged, int nOldLayoutPos, int nNewLayoutPos)
{
    // 没变你进来作甚？
    assert(nOldLayoutPos != nNewLayoutPos);

    bool fReSort = false;

    if (nOldLayoutPos == LP_Absolute || nOldLayoutPos == LP_None)
    {
        const auto _iItemIndex = IgnoreList.FindItemIndex(pElemChanged);
        if (_iItemIndex != ElementList::uInvalidIndex)
        {
            IgnoreList.Remove(_iItemIndex);
            fReSort = true;
        }
    }

    if (nNewLayoutPos == LP_Absolute || nNewLayoutPos == LP_None)
    {
        if (IgnoreList.FindItemIndex(pElemChanged) == ElementList::uInvalidIndex)
        {
            IgnoreList.Add(pElemChanged);
            fReSort = true;
        }
    }

    if (fReSort)
    {
        ReSortIgnoreList();
    }
}

void __YYAPI Layout::Attach(Element* pElem)
{
    // Layout已经绑定了一个Element，不能重复绑定！
    assert(pClient == nullptr);
    pClient = pElem;
    SetCacheDirty();
}

void __YYAPI Layout::Detach(Element* pElem)
{
    // Layout未绑定该Element，无法解绑！
    assert(pClient == pElem);
    pClient = nullptr;
    IgnoreList.Clear();
    SetCacheDirty();
}

void __YYAPI Layout::ReSortIgnoreList()
{
    IgnoreList.Sort(
        [](Element* const* _ppElem1, Element* const* _ppElem2) -> int
        {
            return (*_ppElem1)->iIndex - (*_ppElem2)->iIndex;
        });

    SetCacheDirty();
}

Size __YYAPI Layout::UpdateChildDesiredSize(Element* _pChild, Size _ConstraintSize)
{
    return _pChild->UpdateDesiredSize(_ConstraintSize);
}

const Rect& __YYAPI Layout::GetChildMargin(Element* _pChild)
{
    return _pChild->CmpMargin;
}

Size __YYAPI Layout::GetChildDesiredSize(Element* _pChild)
{
    return _pChild->GetDesiredSize();
}

void __YYAPI Layout::UpdateLayoutRect(Element* pElem, Size _oContainerSize, Element* peChild, Point _oElementPoint, Size _oElementSize)
{
    if (pElem->GetFlowDirection() == FlowDirection::RightToLeft)
        _oElementPoint.X = _oContainerSize.Width - (_oElementPoint.X + _oElementSize.Width);

    peChild->UpdateLayoutPosition(_oElementPoint);
    peChild->UpdateLayoutSize(_oElementSize);
}

Element* __YYAPI Layout::GetChildFromLayoutIndex(Element* pElem, size_t _uLayoutIndex, Array<Element*>* pArray)
{
    ElementList oList = pArray ? *pArray : pElem->GetChildren();

    size_t _uIndex = _uLayoutIndex;

    if (IgnoreList.GetSize())
    {
        size_t i = 0;
        while (i < IgnoreList.GetSize() && IgnoreList[i]->iIndex <= _uIndex)
        {
            _uIndex++;
            i++;
        }
    }

    if (_uIndex >= oList.GetSize())
        return nullptr;

    return oList[_uIndex];
}

size_t __YYAPI Layout::GetLayoutChildCount(Element* pElem)
{
    ElementList oList = pElem->GetChildren();
    return oList.GetSize() - IgnoreList.GetSize();
}

int __YYAPI Layout::GetLayoutIndexFromChild(Element* pElem, Element* pChild)
{
    // pChild不是pElem的子元素！
    assert(pChild->GetParent() == pElem);
    int iLayoutIdx = pChild->iIndex;

    if (IgnoreList.GetSize())
    {
        UINT i = 0;
        while (i < IgnoreList.GetSize() && IgnoreList[i]->iIndex <= iLayoutIdx)
        {
            i++;
        }

        iLayoutIdx -= i;
    }

    return iLayoutIdx;
}

} // namespace MegaUI
} // namespace YY
