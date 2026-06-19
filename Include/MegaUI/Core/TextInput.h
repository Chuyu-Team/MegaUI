#pragma once

#include <YY/Base/YY.h>
#include <YY/Base/Strings/String.h>
#include <Media/Rect.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class ITextInputClient
        {
        public:
            virtual ~ITextInputClient() = default;

            virtual bool __YYAPI IsTextInputEnabled() = 0;

            virtual bool __YYAPI IsReadOnly() = 0;

            // 返回相对元素本地坐标系的 caret rect。
            // 平台桥接层负责将其转换为窗口坐标或屏幕坐标，
            // 用于候选窗和组合窗定位。
            virtual Rect __YYAPI GetTextCaretRect() = 0;

            virtual bool __YYAPI InsertText(_In_ const uString& _Text) = 0;

            virtual bool __YYAPI DeleteBackward() = 0;

            virtual bool __YYAPI DeleteForward() = 0;

            virtual bool __YYAPI StartComposition() = 0;

            virtual bool __YYAPI UpdateComposition(_In_ const uString& _Text, uint32_t _uCaretPos) = 0;

            virtual bool __YYAPI CommitComposition(_In_ const uString& _Text) = 0;

            virtual bool __YYAPI CancelComposition() = 0;

            virtual void __YYAPI EndComposition() = 0;
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
