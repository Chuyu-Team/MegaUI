#pragma once

#include <dwrite.h>

#include <MegaUI/Core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY::MegaUI
{
    // clang-format off
    //     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_TEXT_BOX_PROPERTY_TABLE(_APPLY) 

    // clang-format on

    typedef Array<DWRITE_HIT_TEST_METRICS, AllocPolicy::SOO, 128> TextBoxSelectedItems;

    class TextBox : public Element
    {
        _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(TextBox, Element, ControlInfoImp<TextBox>, 0u, _MEGA_UI_TEXT_BOX_PROPERTY_TABLE);

    private:
        uint32_t uCaretAnchor = UINT32_MAX;
        uint32_t uCaretPosition = UINT32_MAX;

    public:
        TextBox();

        // void __YYAPI Paint(_In_ DrawContext* _pDrawContext, _In_ const Rect& _Bounds) override;
        void __YYAPI PaintContent(
            _In_ DrawContext* _pDrawContext,
            _In_ const Value& _Content,
            _In_ const Font& _FontInfo,
            _In_ Color _ForegroundColor,
            _In_ const Rect& _Bounds,
            _In_ ContentAlignStyle _fContentAlign
            ) override;

    protected:
        void UpdateCaret(const Point& _Position, bool _bMoveAnchor);

        void GetCaret(/*const Point& _oTextLayoutOrigin, */Rect* _poCaretRect, TextBoxSelectedItems* _poSecectedRangeRect);

        void PaintCaret(_In_ DrawContext* _pDrawContext, _In_ const Point& _Origin);

        // -----------------------------
        //	Element
        // -----------------------------
        bool __YYAPI OnLeftButtonDown(const MouseEvent& _Event) override;

        bool __YYAPI OnLeftButtonUp(const MouseEvent& _Event) override;

        bool __YYAPI OnMouseMove(const MouseEvent& _Event) override;

        bool __YYAPI OnKeyboardFocusedPropChanged(_In_ OnPropertyChangedHandleData* _pHandleData) override;

        bool __YYAPI OnKeyDown(const KeyboardEvent& _KeyEvent) override;
    };
}

#pragma pack(pop)
