#pragma once
#include <YY/Base/YY.h>
#include <MegaUI/Core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Layout
        {
        private:
            // 4
            Element* pClient = nullptr;
            // 8
            ElementList IgnoreList;
            // 0xC
            bool bCacheDirty = true;
        public:
            // 0x10
            constexpr Layout() = default;

            // 8
            virtual ~Layout() = default;
            
            Layout(const Layout&) = delete;

            Layout& operator=(const Layout&) = delete;

            static HRESULT __YYAPI Create(Layout** ppLayout);

            static void __YYAPI UpdateLayoutRect(Element* pElem, Size _oContainerSize, Element* peChild, Point _oElementPoint, Size _oElementSize);

            void __YYAPI Destroy();

            Element* __YYAPI GetChildFromLayoutIndex(Element* pElem, int iLayoutIndex, Array<Element*>* pArray);

            size_t __YYAPI GetLayoutChildCount(Element* pElem);

            int __YYAPI GetLayoutIndexFromChild(Element* pElem, Element* pChild);

            void __YYAPI Initialize();
            // 0
            virtual void __YYAPI DoLayout(Element* pElem, Size _oSize);

            // 1
            virtual Size __YYAPI UpdateDesiredSize(Element* pElem, Size _ConstraintSize);
            // 2
            virtual void __YYAPI OnAdd(Element* pElem, Element** ppElemAdd, size_t uCountOfElementAdd);
            // 3
            virtual void __YYAPI OnRemove(Element* pElem, Element** ppElemRemove, size_t uCountOfElementRemove);
            // 4
            virtual void __YYAPI OnLayoutPosChanged(Element* pElem1, Element* pElemChanged, int nOldLayoutPos, int nNewLayoutPos);

            // 5
            virtual void __YYAPI Attach(Element* pElem);
            // 6
            virtual void __YYAPI Detach(Element* pElem);
            // 7
            virtual Element* __YYAPI GetAdjacent(Element* pElem, Element* _pFrom, int iNavDir, NavReference const* pnr, bool _bKeyableOnly);

        protected:
            void __YYAPI ClearCacheDirty()
            {
                bCacheDirty = false;
            }

            bool __YYAPI IsCacheDirty()
            {
                return bCacheDirty;
            }

            void __YYAPI SetCacheDirty()
            {
                bCacheDirty = true;
            }

            void __YYAPI ReSortIgnoreList();
        };
    }
} // namespace YY

#pragma pack(pop)
