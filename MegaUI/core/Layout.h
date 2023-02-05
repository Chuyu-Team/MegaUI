#pragma once
#include "..\base\MegaUITypeInt.h"
#include "Element.h"

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Layout
        {
        private:
            // 4
            Element* pClient;
            // 8
            ElementList IgnoreList;
            // 0xC
            bool bCacheDirty;
        public:
            // 0x10
            Layout();
            // 8
            virtual ~Layout();
            
            Layout(const Layout&) = delete;
            Layout& operator=(const Layout&) = delete;

            static HRESULT __YYAPI Create(Layout** ppLayout);
            static void __YYAPI UpdateLayoutRect(Element*, int, int, Element*, int, int, int, int);

            void __YYAPI Destroy();
            Element* __YYAPI GetChildFromLayoutIndex(Element* pElem, int nIndex, Array<Element*>* pArray);
            unsigned int __YYAPI GetLayoutChildCount(Element*);
            int __YYAPI GetLayoutIndexFromChild(Element*, Element*);
            void __YYAPI Initialize();
            // 0
            virtual void __YYAPI DoLayout(Element*, Size);

            // 1
            virtual SIZE __YYAPI UpdateDesiredSize(Element*, int, int);
            // 2
            virtual void __YYAPI OnAdd(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 3
            virtual void __YYAPI OnRemove(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 4
            virtual void __YYAPI OnLayoutPosChanged(Element* pElem1, Element* pElem2, int nLayoutPos1, int nLayoutPos2);

            // 5
            virtual void __YYAPI Attach(Element* pElem);
            // 6
            virtual void __YYAPI Detach(Element*);
            // 7
            virtual Element* __YYAPI GetAdjacent(Element*, Element*, int, NavReference const*, unsigned long);

        protected:
            void __YYAPI ClearCacheDirty();
            bool __YYAPI IsCacheDirty();
            void __YYAPI SetCacheDirty();
        };
    }
} // namespace YY

#pragma pack(pop)
