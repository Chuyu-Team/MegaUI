#pragma once

#include <dwrite.h>

#include <MegaUI/Core/Element.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        // clang-format off
        //     属性名称             属性Flags                                        属性组FLAGS                       DefaultValue函数                         CustomPropertyHandle                      pEnumMaps              BindCache                                                                    ValidValueType
#define _MEGA_UI_TEXT_BOX_PROPERTY_TABLE(_APPLY) \
    _APPLY(ReadOnly, PF_Normal, 0, &Value::CreateBoolFalse, nullptr, nullptr, nullptr, _MEGA_UI_PROP_BIND_NONE(), ValueType::boolean)

    // clang-format on

        typedef Array<DWRITE_HIT_TEST_METRICS, AllocPolicy::SOO, 128> TextBoxSelectedItems;

        class TextBox
            : public Element
            , protected ITextInputClient
        {
            _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(TextBox, Element, ControlInfoImp<TextBox>, 0u, _MEGA_UI_TEXT_BOX_PROPERTY_TABLE);

        private:
            uint32_t uCaretAnchor = UINT32_MAX;
            uint32_t uCaretPosition = UINT32_MAX;
            bool bCompositionActive = false;
            uint32_t uCompositionRangeStart = UINT32_MAX;
            uint32_t uCompositionRangeLength = 0;
            uint32_t uCompositionCaret = 0;
            uint32_t uSuppressNextCharCount = 0;
            uString szCompositionText;

        public:
            TextBox();

            bool __YYAPI IsReadOnly();

            HRESULT __YYAPI SetReadOnly(bool _bReadOnly);

            uString __YYAPI GetText();
            
            bool __YYAPI SetText(uString _szText);

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

            bool __YYAPI OnChar(const KeyboardEvent& _KeyEvent) override;

            ITextInputClient* __YYAPI GetTextInputClient() override;
            
            // -----------------------------
            //	ITextInputClient
            // -----------------------------
            bool __YYAPI IsTextInputEnabled() override;

            Rect __YYAPI GetTextCaretRect() override;

            bool __YYAPI InsertText(_In_ const uString& _Text) override;

            bool __YYAPI DeleteBackward() override;

            bool __YYAPI DeleteForward() override;

            bool __YYAPI StartComposition() override;

            bool __YYAPI UpdateComposition(_In_ const uString& _Text, uint32_t _uCaretPos) override;

            bool __YYAPI CommitComposition(_In_ const uString& _Text) override;

            bool __YYAPI CancelComposition() override;

            void __YYAPI EndComposition() override;

        private:

            void NormalizeCaret(const uString& _Text);

            bool TryGetSelectionRange(uint32_t* _puStart, uint32_t* _puEnd);

            bool ReplaceSelection(const uString& _Text);

            bool InsertTextCore(const uString& _Text);

            bool DeleteBackwardCore();

            bool DeleteForwardCore();

            uint32_t GetDeleteBackwardLength(const uString& _Text) const;

            uint32_t GetDeleteForwardLength(const uString& _Text) const;

            void InvalidateTextBox();

            uString BuildDisplayText();

            uint32_t ToDisplayPosition(uint32_t _uCommittedPosition);

            uint32_t ToCommittedPosition(uint32_t _uDisplayPosition);

            RefPtr<IDWriteTextLayout> CreateActiveTextLayout(Size _ConstraintSize);
        };
    }
}

#pragma pack(pop)
