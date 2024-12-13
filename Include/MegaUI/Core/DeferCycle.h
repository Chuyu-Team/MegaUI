#pragma once

#include <Base/Containers/Array.h>
#include <Base/Containers/HashSet.h>

#include <Base/Memory/Alloc.h>
#include <Base/Memory/RefPtr.h>

#pragma pack(push, __YY_PACKING)

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
            int32_t iDepPos = -1;
            int32_t cDepCnt = 0;
        };

        struct PCRecord
        {
            PropertyIndicies iIndex = PropertyIndicies::PI_MAX;
            Element* pElement = nullptr;
            // ppi
            const PropertyInfo* pProp = nullptr;
            //bool bVoid;
            uint8_t iRefCount = 0;
            bool vA = false;
            bool vB = false;

            // 0xC
            long vC = 0;
            Value pOldValue;
            Value pNewValue;
            DepRecs dr;
            int32_t iPrevElRec = 0;
        };

        // Group notifications: deferred until EndDefer and coalesced
        struct GCRecord
        {
            Element* pElement;
            uint32_t fGroups;
        };

        class DeferCycle : public RefValue
        {
        public:
            // pdaGC
            Array<GCRecord, AllocPolicy::SOO> vecGroupChangeNormalPriority;
            // pdaGCLP
            Array<GCRecord, AllocPolicy::SOO> vecGroupChangeLowPriority;
            // pdaPC
            Array<PCRecord, AllocPolicy::SOO> vecPropertyChanged;

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

            DeferCycle()
                : uEnter(0u)
                , bFiring(false)
                , uGroupChangeNormalPriorityFireCount(0u)
                , uGroupChangeLowPriorityFireCount(0u)
                , uPropertyChangedFireCount(0u)
                , uPropertyChangedPostSourceCount(0u)
                , uPropertyChangeEnter(0u)
            {
            }
        };
    }
}

#pragma pack(pop)
