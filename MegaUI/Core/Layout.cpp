#include <MegaUI/Core/Layout.h>

namespace YY {
namespace MegaUI {

HRESULT __YYAPI Layout::Create(Layout** ppLayout)
{
    if (!ppLayout)
        return E_POINTER;

    *ppLayout = new Layout();
    if (!*ppLayout)
        return E_OUTOFMEMORY;

    return S_OK;
}

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

void __YYAPI Layout::UpdateLayoutRect(Element* pElem, Size _oContainerSize, Element* peChild, Point _oElementPoint, Size _oElementSize)
{
    if (pElem->GetFlowDirection() == FlowDirection::RightToLeft)
        _oElementPoint.X = _oContainerSize.Width - (_oElementPoint.X + _oElementSize.Width);

    peChild->UpdateLayoutPosition(_oElementPoint);
    peChild->UpdateLayoutSize(_oElementSize);
}

void __YYAPI Layout::Destroy()
{
    delete this;
}

void __YYAPI Layout::DoLayout(Element* pElem, Size _oSize)
{
    // 基类默认空实现
}

Size __YYAPI Layout::UpdateDesiredSize(Element* pElem, Size _ConstraintSize)
{
    // 基类默认空实现
    return Size(0, 0);
}

Element* __YYAPI Layout::GetAdjacent(Element* pElem, Element* _pFrom, int iNavDir, NavReference const* pnr, bool _bKeyableOnly)
{
    // 基类默认空实现
    return nullptr;
}

Element* __YYAPI Layout::GetChildFromLayoutIndex(Element* pElem, int iLayoutIndex, Array<Element*>* pArray)
{
    ElementList oList = pArray ? *pArray : pElem->GetChildren();

    int _iIndex = iLayoutIndex;

    if (IgnoreList.GetSize())
    {
        UINT i = 0;
        while (i < IgnoreList.GetSize() && IgnoreList[i]->iIndex <= _iIndex)
        {
            _iIndex++;
            i++;
        }
    }

    if ((UINT)_iIndex >= oList.GetSize())
        return nullptr;

    return oList[_iIndex];
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
