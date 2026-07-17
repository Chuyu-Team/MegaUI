#pragma once
#include <YY/Base/YY.h>
#include <YY/Base/Memory/RefPtr.h>
#include <MegaUI/Core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Layout : public RefValue
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

            static void __YYAPI UpdateLayoutRect(Element* pElem, Size _oContainerSize, Element* peChild, Point _oElementPoint, Size _oElementSize);

            Element* __YYAPI GetChildFromLayoutIndex(Element* pElem, size_t _uLayoutIndex, Array<Element*>* pArray);

            size_t __YYAPI GetLayoutChildCount(Element* pElem);

            int __YYAPI GetLayoutIndexFromChild(Element* pElem, Element* pChild);

            // 0
            virtual void __YYAPI DoLayout(Element* pElem, Size _oSize) = 0;

            // 1
            virtual Size __YYAPI UpdateDesiredSize(Element* pElem, Size _ConstraintSize) = 0;
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
            virtual _Ret_maybenull_ Element* __YYAPI GetAdjacent(Element* pElem, Element* _pFrom, NavigatingType _eNavigate, NavReference const* pnr, bool _bKeyableOnly) = 0;

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

            /// <summary>
            /// 调用子元素的 UpdateDesiredSize（桥接 Element protected 方法，供 Layout 子类使用）。
            /// </summary>
            static Size __YYAPI UpdateChildDesiredSize(_In_ Element* _pChild, Size _ConstraintSize)
            {
                return _pChild->UpdateDesiredSize(_ConstraintSize);
            }

            /// <summary>
            /// 获取子元素的计算后外边距（桥接 Element protected 成员，供 Layout 子类使用）。
            /// </summary>
            static const Rect& __YYAPI GetChildMargin(_In_ Element* _pChild)
            {
                return _pChild->CmpMargin;
            }

            /// <summary>
            /// 获取子元素的期望尺寸（便捷访问）。
            /// </summary>
            static Size __YYAPI GetChildDesiredSize(_In_ Element* _pChild)
            {
                return _pChild->GetDesiredSize();
            }
        };
    }
} // namespace YY

#pragma pack(pop)
