#pragma once
#include "..\base\MegaUITypeInt.h"
#include "Element.h"

#pragma pack(push, __MEGA_UI_PACKING)

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

            static HRESULT WINAPI Create(Layout** ppLayout);
            static void WINAPI UpdateLayoutRect(Element*, int, int, Element*, int, int, int, int);

            void Destroy();
            Element* GetChildFromLayoutIndex(Element* pElem, int nIndex, DynamicArray<Element*, 0>* pArray);
            unsigned int GetLayoutChildCount(Element*);
            int GetLayoutIndexFromChild(Element*, Element*);
            void Initialize();
            // 0
            virtual void DoLayout(Element*, int, int);

            // 1
            virtual SIZE UpdateDesiredSize(Element*, int, int);
            // 2
            virtual void OnAdd(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 3
            virtual void OnRemove(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 4
            virtual void OnLayoutPosChanged(Element* pElem1, Element* pElem2, int nLayoutPos1, int nLayoutPos2);

            // 5
            virtual void Attach(Element* pElem);
            // 6
            virtual void Detach(Element*);
            // 7
            virtual Element* GetAdjacent(Element*, Element*, int, NavReference const*, unsigned long);

        protected:
            void ClearCacheDirty();
            bool IsCacheDirty();
            void SetCacheDirty();
        };
    }
} // namespace YY