#pragma once
#include <MegaUI/Core/Element.h>
#include <MegaUI/Control/TreeViewItem.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // clang-format off
        //     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_TREE_VIEW_PROPERTY_TABLE(_APPLY)

        // clang-format on

        class TreeView : public Element
        {
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(TreeView, Element, ControlInfoImp<TreeView>, 0u, _MEGA_UI_TREE_VIEW_PROPERTY_TABLE);

        private:
            
        public:
            TreeView();
        };
    }
} // namespace YY

#pragma pack(pop)
