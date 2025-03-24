#pragma once
#include <MegaUI/Core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // clang-format off
        //     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_TREE_VIEW_ITEM_PROPERTY_TABLE(_APPLY)

        // clang-format on

        class TreeViewItem : public Element
        {
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(TreeViewItem, Element, ControlInfoImp<TreeViewItem>, 0u, _MEGA_UI_TREE_VIEW_ITEM_PROPERTY_TABLE);

        private:

        public:
            TreeViewItem();
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
