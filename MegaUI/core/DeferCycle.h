#pragma once

#include "..\base\DynamicArray.h"
#include "..\base\HashSet.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Element;
        struct PropertyInfo;
        class Value;

        // Track dependency records in PC list
        struct DepRecs
        {
            int32_t iDepPos = 0;
            int32_t cDepCnt = 0;
        };

        struct PCRecord
        {
            PropertyIndicies iIndex;
            Element* pElement;
            // ppi
            const PropertyInfo* pProp;
            //bool bVoid;
            uint8_t iRefCount;
            bool vA;
            bool vB;

            // 0xC
            long vC;
            Value pOldValue;
            Value pNewValue;
            DepRecs dr;
            int32_t iPrevElRec;
        };

        // Group notifications: deferred until EndDefer and coalesced
        struct GCRecord
        {
            Element* pElement;
            uint32_t fGroups;
        };

        class DeferCycle
        {
        public:
            // pdaGC
            DynamicArray<GCRecord, false, false> vecGroupChangeNormalPriority;
            // pdaGCLP
            DynamicArray<GCRecord, false, false> vecGroupChangeLowPriority;
            // pdaPC
            DynamicArray<PCRecord, false, true> vecPropertyChanged;

            HashSet<Element*, 12> LayoutRootPendingSet;
            HashSet<Element*, 12> UpdateDesiredSizeRootPendingSet;

            // StartDefer的进入次数
            uint32_t uEnter;
            // 已经进入 EndDefer？
            bool bFiring;
            // iGCPtr
            uint32_t uGroupChangeNormalPriorityFireCount;
            //int32_t iGCLPPtr;
            uint32_t uGroupChangeLowPriorityFireCount;
            //int32_t iPCPtr;
            uint32_t uPropertyChangedFireCount;
            //int32_t iPCSSUpdate;
            uint32_t uPropertyChangedPostSourceCount;
            //int32_t cPCEnter;
            uint32_t uPropertyChangeEnter;
            // 当前内存区域的引用计数，如果为 0，则销毁此对象
            uint32_t uRef;

            DeferCycle()
                : uEnter(0u)
                , bFiring(false)
                , uGroupChangeNormalPriorityFireCount(0u)
                , uGroupChangeLowPriorityFireCount(0u)
                , uPropertyChangedFireCount(0u)
                , uPropertyChangedPostSourceCount(0u)
                , uPropertyChangeEnter(0u)
                , uRef(0)
            {
            }


            uint32_t __fastcall AddRef()
            {
                return ++uRef;
            }

            uint32_t __fastcall Release()
            {
                const auto _uRefNew = --uRef;

                if (_uRefNew == 0)
                {
                    HDelete(this);
                }

                return _uRefNew;
            }
        };
    }
}

#pragma pack(pop)
