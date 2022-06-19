#pragma once

#include "..\base\DynamicArray.h"

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
            int32_t iDepPos;
            int32_t cDepCnt;
        };

        struct PCRecord
        {
            bool bVoid;
            int32_t iIndex;
            Element* pElement;
            PropertyInfo* pProp;
            
            Value* pvOld;
            Value* pvNew;
            DepRecs dr;
            int32_t iPrevElRec;
        };

        // Group notifications: deferred until EndDefer and coalesced
        struct GCRecord
        {
            Element* pe;
            uint32_t fGroups;
        };

        class DeferCycle
        {
        public:
            DynamicArray<GCRecord> vecGroupChangeNormalPriority;
            DynamicArray<GCRecord> vecGroupChangeLowPriority;
            DynamicArray<PCRecord> vecPropertyChanged;

            // StartDefer的进入次数
            uint32_t uEnter;
            // 已经进入 EndDefer？
            bool bFiring;
            // 
            uint32_t uGroupChangeNormalPriorityFireIndex;
            //int32_t iGCLPPtr;
            uint32_t uGroupChangeLowPriorityFireIndex;
            int32_t iPCPtr;
            int32_t iPCSSUpdate;
            int32_t cPCEnter;
            // 当前内存区域的引用计数，如果为 0，则销毁此对象
            uint32_t uRef;


            uint32_t __fastcall AddRef()
            {
                return ++uRef;
            }

            uint32_t __fastcall Release()
            {
                const auto uRefNew = --uRef;

                if (uRefNew == 0)
                {
                    HDelete(this);
                }

                return uRefNew;
            }
        };
    }
}

#pragma pack(pop)
