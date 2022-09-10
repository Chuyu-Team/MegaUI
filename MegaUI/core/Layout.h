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

            static HRESULT __MEGA_UI_API Create(Layout** ppLayout);
            static void __MEGA_UI_API UpdateLayoutRect(Element*, int, int, Element*, int, int, int, int);

            void __MEGA_UI_API Destroy();
            Element* __MEGA_UI_API GetChildFromLayoutIndex(Element* pElem, int nIndex, DynamicArray<Element*, 0>* pArray);
            unsigned int __MEGA_UI_API GetLayoutChildCount(Element*);
            int __MEGA_UI_API GetLayoutIndexFromChild(Element*, Element*);
            void __MEGA_UI_API Initialize();
            // 0
            virtual void __MEGA_UI_API DoLayout(Element*, int, int);

            // 1
            virtual SIZE __MEGA_UI_API UpdateDesiredSize(Element*, int, int);
            // 2
            virtual void __MEGA_UI_API OnAdd(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 3
            virtual void __MEGA_UI_API OnRemove(Element*, Element** ppElem, unsigned int uCountOfElement);
            // 4
            virtual void __MEGA_UI_API OnLayoutPosChanged(Element* pElem1, Element* pElem2, int nLayoutPos1, int nLayoutPos2);

            // 5
            virtual void __MEGA_UI_API Attach(Element* pElem);
            // 6
            virtual void __MEGA_UI_API Detach(Element*);
            // 7
            virtual Element* __MEGA_UI_API GetAdjacent(Element*, Element*, int, NavReference const*, unsigned long);

        protected:
            void __MEGA_UI_API ClearCacheDirty();
            bool __MEGA_UI_API IsCacheDirty();
            void __MEGA_UI_API SetCacheDirty();
        };
    }
} // namespace YY

#pragma pack(pop)
