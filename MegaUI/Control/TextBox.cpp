#include "pch.h"
#include <MegaUI/Control/TextBox.h>

#include <MegaUI/Window/Window.h>
#include <MegaUI/Core/ControlInfoImp.h>

namespace YY
{
    namespace MegaUI
    {
        namespace
        {
            inline bool IsHighSurrogate(u16char_t _ch)
            {
                const auto _uValue = uint32_t(_ch);
                return _uValue >= 0xD800u && _uValue <= 0xDBFFu;
            }

            inline bool IsLowSurrogate(u16char_t _ch)
            {
                const auto _uValue = uint32_t(_ch);
                return _uValue >= 0xDC00u && _uValue <= 0xDFFFu;
            }
        }

        _APPLY_MEGA_UI_STATIC_CONTROL_INFO(TextBox, _MEGA_UI_TEXT_BOX_PROPERTY_TABLE);

        TextBox::TextBox() = default;

        bool __YYAPI TextBox::IsReadOnly()
        {
            auto _Value = GetValue(TextBox::g_ControlInfoData.ReadOnlyProp);
            return _Value.GetType() == ValueType::boolean && _Value.GetBool();
        }

        HRESULT __YYAPI TextBox::SetReadOnly(bool _bReadOnly)
        {
            return SetValue(TextBox::g_ControlInfoData.ReadOnlyProp, Value::CreateBool(_bReadOnly));
        }

        bool __YYAPI TextBox::IsAcceptsReturn()
        {
            auto _Value = GetValue(TextBox::g_ControlInfoData.AcceptsReturnProp);
            return _Value.GetType() == ValueType::boolean && _Value.GetBool();
        }

        HRESULT __YYAPI TextBox::SetAcceptsReturn(bool _bAcceptsReturn)
        {
            return SetValue(TextBox::g_ControlInfoData.AcceptsReturnProp, Value::CreateBool(_bAcceptsReturn));
        }

        bool __YYAPI TextBox::IsAcceptsTab()
        {
            auto _Value = GetValue(TextBox::g_ControlInfoData.AcceptsTabProp);
            return _Value.GetType() == ValueType::boolean && _Value.GetBool();
        }

        HRESULT __YYAPI TextBox::SetAcceptsTab(bool _bAcceptsTab)
        {
            return SetValue(TextBox::g_ControlInfoData.AcceptsTabProp, Value::CreateBool(_bAcceptsTab));
        }

        bool __YYAPI TextBox::IsTextInputEnabled()
        {
            return true;
        }

        Rect __YYAPI TextBox::GetTextCaretRect()
        {
            Rect _CaretRect;
            TextBoxSelectedItems _SelectedItems;
            GetCaret(&_CaretRect, &_SelectedItems);
            return _CaretRect;
        }

        bool __YYAPI TextBox::InsertText(const uString& _szText)
        {
            return InsertTextCore(_szText);
        }

        bool __YYAPI TextBox::DeleteBackward()
        {
            return DeleteBackwardCore();
        }

        bool __YYAPI TextBox::DeleteForward()
        {
            return DeleteForwardCore();
        }

        bool __YYAPI TextBox::StartComposition()
        {
            if (IsReadOnly())
                return false;

            auto _szText = GetText();
            NormalizeCaret(_szText);

            uint32_t _uSelectionStart = uCaretPosition;
            uint32_t _uSelectionEnd = uCaretPosition;
            TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd);

            bCompositionActive = true;
            uCompositionRangeStart = _uSelectionStart;
            uCompositionRangeLength = _uSelectionEnd - _uSelectionStart;
            uCompositionCaret = 0;
            szCompositionText.Clear();
            InvalidateTextBox();
            return true;
        }

        bool __YYAPI TextBox::UpdateComposition(const uString& _szText, uint32_t _uCaretPos)
        {
            if (IsReadOnly())
                return false;

            if (!bCompositionActive)
            {
                if (!StartComposition())
                    return false;
            }

            szCompositionText = _szText;
            uCompositionRangeLength = uint32_t(_szText.GetSize());
            uCompositionCaret = _uCaretPos > uCompositionRangeLength ? uCompositionRangeLength : _uCaretPos;
            InvalidateTextBox();
            return true;
        }

        bool __YYAPI TextBox::CommitComposition(const uString& _szText)
        {
            if (IsReadOnly())
                return false;

            auto _bResult = InsertTextCore(_szText);
            if (_bResult)
            {
                uSuppressNextCharCount += uint32_t(_szText.GetSize());
                bCompositionActive = false;
                uCompositionRangeStart = UINT32_MAX;
                uCompositionRangeLength = 0;
                uCompositionCaret = 0;
                szCompositionText.Clear();
                InvalidateTextBox();
            }

            return _bResult;
        }

        bool __YYAPI TextBox::CancelComposition()
        {
            if (!bCompositionActive)
                return false;

            bCompositionActive = false;
            uCompositionRangeStart = UINT32_MAX;
            uCompositionRangeLength = 0;
            uCompositionCaret = 0;
            szCompositionText.Clear();
            InvalidateTextBox();
            return true;
        }

        void __YYAPI TextBox::EndComposition()
        {
            CancelComposition();
        }

        uString __YYAPI TextBox::GetText()
        {
            auto _ContentValue = GetValue(Element::g_ControlInfoData.ContentProp);
            if (_ContentValue.GetType() == ValueType::String)
                return _ContentValue.GetString();

            return uString();
        }

        bool __YYAPI TextBox::SetText(uString _szText)
        {
            auto _TextValue = Value::CreateString(_szText);
            if (_TextValue == nullptr)
                return false;

            return SUCCEEDED(SetValue(Element::g_ControlInfoData.ContentProp, _TextValue));
        }

        void TextBox::NormalizeCaret(const uString& _szText)
        {
            if (uCaretPosition == UINT32_MAX)
                uCaretPosition = uint32_t(_szText.GetSize());
            else
                uCaretPosition = NormalizeCaretPosition(_szText, uCaretPosition);

            if (uCaretAnchor == UINT32_MAX)
                uCaretAnchor = uCaretPosition;
            else
                uCaretAnchor = NormalizeCaretPosition(_szText, uCaretAnchor);
        }

        uint32_t TextBox::NormalizeCaretPosition(const uString& _szText, uint32_t _uPosition) const
        {
            const auto _uLength = uint32_t(_szText.GetSize());
            if (_uPosition > _uLength)
                _uPosition = _uLength;

            if (_uPosition > 0 && _uPosition < _uLength)
            {
                if (_szText[_uPosition - 1] == u16char_t('\r') && _szText[_uPosition] == u16char_t('\n'))
                    --_uPosition;
                else if (IsHighSurrogate(_szText[_uPosition - 1]) && IsLowSurrogate(_szText[_uPosition]))
                    --_uPosition;
            }

            return _uPosition;
        }

        uint32_t TextBox::GetPreviousCaretPosition(const uString& _szText, uint32_t _uPosition) const
        {
            _uPosition = NormalizeCaretPosition(_szText, _uPosition);
            if (_uPosition == 0)
                return 0;

            if (_uPosition >= 2)
            {
                if (_szText[_uPosition - 2] == u16char_t('\r') && _szText[_uPosition - 1] == u16char_t('\n'))
                    return _uPosition - 2;

                if (IsHighSurrogate(_szText[_uPosition - 2]) && IsLowSurrogate(_szText[_uPosition - 1]))
                    return _uPosition - 2;
            }

            return _uPosition - 1;
        }

        uint32_t TextBox::GetNextCaretPosition(const uString& _szText, uint32_t _uPosition) const
        {
            _uPosition = NormalizeCaretPosition(_szText, _uPosition);

            const auto _uLength = uint32_t(_szText.GetSize());
            if (_uPosition >= _uLength)
                return _uLength;

            if (_uPosition + 1 < _uLength)
            {
                if (_szText[_uPosition] == u16char_t('\r') && _szText[_uPosition + 1] == u16char_t('\n'))
                    return _uPosition + 2;

                if (IsHighSurrogate(_szText[_uPosition]) && IsLowSurrogate(_szText[_uPosition + 1]))
                    return _uPosition + 2;
            }

            return _uPosition + 1;
        }

        void TextBox::ResetPreferredCaretX()
        {
            bPreferredCaretXValid = false;
        }

        bool TextBox::TryGetSelectionRange(uint32_t* _puStart, uint32_t* _puEnd)
        {
            if (!_puStart || !_puEnd)
                return false;

            if (uCaretAnchor == UINT32_MAX || uCaretPosition == UINT32_MAX)
                return false;

            if (uCaretAnchor == uCaretPosition)
                return false;

            if (uCaretAnchor < uCaretPosition)
            {
                *_puStart = uCaretAnchor;
                *_puEnd = uCaretPosition;
            }
            else
            {
                *_puStart = uCaretPosition;
                *_puEnd = uCaretAnchor;
            }

            return true;
        }

        bool TextBox::ReplaceSelection(const uString& _szText)
        {
            if (IsReadOnly())
                return false;

            auto _CommittedText = GetText();
            NormalizeCaret(_CommittedText);

            uint32_t _uSelectionStart = uCaretPosition;
            uint32_t _uSelectionEnd = uCaretPosition;
            TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd);

            uString _NewText;

            if (_uSelectionStart)
                _NewText += _CommittedText.Substring(0, _uSelectionStart);

            if (_szText.GetSize())
                _NewText += _szText;

            if (_uSelectionEnd < _CommittedText.GetSize())
                _NewText += _CommittedText.Substring(_uSelectionEnd);

            if (!SetText(_NewText))
                return false;

            uCaretAnchor = uCaretPosition = _uSelectionStart + uint32_t(_szText.GetSize());
            ResetPreferredCaretX();
            InvalidateTextBox();
            return true;
        }

        bool TextBox::InsertTextCore(const uString& _szText)
        {
            if (_szText.GetSize() == 0)
                return true;

            return ReplaceSelection(_szText);
        }

        bool TextBox::DeleteBackwardCore()
        {
            if (IsReadOnly())
                return false;

            auto _CommittedText = GetText();
            NormalizeCaret(_CommittedText);

            uint32_t _uSelectionStart = 0;
            uint32_t _uSelectionEnd = 0;
            if (TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd))
                return ReplaceSelection(uString());

            if (uCaretPosition == 0)
                return false;

            auto _NewText = _CommittedText;
            const auto _uDeleteLength = GetDeleteBackwardLength(_CommittedText);
            _NewText.Remove(uCaretPosition - _uDeleteLength, _uDeleteLength);
            if (!SetText(_NewText))
                return false;

            uCaretAnchor = uCaretPosition = uCaretPosition - _uDeleteLength;
            ResetPreferredCaretX();
            InvalidateTextBox();
            return true;
        }

        bool TextBox::DeleteForwardCore()
        {
            if (IsReadOnly())
                return false;

            auto _CommittedText = GetText();
            NormalizeCaret(_CommittedText);

            uint32_t _uSelectionStart = 0;
            uint32_t _uSelectionEnd = 0;
            if (TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd))
                return ReplaceSelection(uString());

            if (uCaretPosition >= _CommittedText.GetSize())
                return false;

            auto _NewText = _CommittedText;
            _NewText.Remove(uCaretPosition, GetDeleteForwardLength(_CommittedText));
            if (!SetText(_NewText))
                return false;

            uCaretAnchor = uCaretPosition;
            ResetPreferredCaretX();
            InvalidateTextBox();
            return true;
        }

        uint32_t TextBox::GetDeleteBackwardLength(const uString& _szText) const
        {
            if (uCaretPosition == 0 || uCaretPosition > _szText.GetSize())
                return 0;

            const auto _uPrevious = GetPreviousCaretPosition(_szText, uCaretPosition);
            return uCaretPosition - _uPrevious;
        }

        uint32_t TextBox::GetDeleteForwardLength(const uString& _szText) const
        {
            if (uCaretPosition >= _szText.GetSize())
                return 0;

            const auto _uNext = GetNextCaretPosition(_szText, uCaretPosition);
            return _uNext - uCaretPosition;
        }

        void TextBox::InvalidateTextBox()
        {
            auto _pWindow = GetWindow();
            if (_pWindow)
                _pWindow->InvalidateRect(nullptr);
        }

        uString TextBox::BuildDisplayText()
        {
            auto _CommittedText = GetText();
            if (!bCompositionActive || uCompositionRangeStart == UINT32_MAX)
                return _CommittedText;

            auto _uStart = uCompositionRangeStart;
            if (_uStart > _CommittedText.GetSize())
                _uStart = uint32_t(_CommittedText.GetSize());

            auto _uEnd = uCompositionRangeStart + uCompositionRangeLength;
            if (_uEnd > _CommittedText.GetSize())
                _uEnd = uint32_t(_CommittedText.GetSize());

            uString _szDisplayText;
            if (_uStart)
                _szDisplayText += _CommittedText.Substring(0, _uStart);

            _szDisplayText += szCompositionText;

            if (_uEnd < _CommittedText.GetSize())
                _szDisplayText += _CommittedText.Substring(_uEnd);

            return _szDisplayText;
        }

        uint32_t TextBox::ToDisplayPosition(uint32_t _uCommittedPosition)
        {
            if (!bCompositionActive || uCompositionRangeStart == UINT32_MAX)
                return _uCommittedPosition;

            const auto _uCompStart = uCompositionRangeStart;
            const auto _uCompEnd = uCompositionRangeStart + uCompositionRangeLength;
            const auto _uCompTextLength = uint32_t(szCompositionText.GetSize());

            if (_uCommittedPosition <= _uCompStart)
                return _uCommittedPosition;

            if (_uCommittedPosition >= _uCompEnd)
                return _uCommittedPosition - uCompositionRangeLength + _uCompTextLength;

            return _uCompStart + _uCompTextLength;
        }

        uint32_t TextBox::ToCommittedPosition(uint32_t _uDisplayPosition)
        {
            if (!bCompositionActive || uCompositionRangeStart == UINT32_MAX)
                return _uDisplayPosition;

            const auto _uCompStart = uCompositionRangeStart;
            const auto _uCompEnd = uCompositionRangeStart + uCompositionRangeLength;
            const auto _uCompTextLength = uint32_t(szCompositionText.GetSize());
            const auto _uDisplayCompEnd = _uCompStart + _uCompTextLength;

            if (_uDisplayPosition <= _uCompStart)
                return _uDisplayPosition;

            if (_uDisplayPosition >= _uDisplayCompEnd)
                return _uDisplayPosition - _uCompTextLength + uCompositionRangeLength;

            const auto _uRelative = _uDisplayPosition - _uCompStart;
            return _uCompStart + (_uRelative > uCompositionRangeLength ? uCompositionRangeLength : _uRelative);
        }

        RefPtr<IDWriteTextLayout> TextBox::CreateActiveTextLayout(Size _ConstraintSize)
        {
            if (!bCompositionActive)
                return GetTextLayout(_ConstraintSize);

            auto _pWindow = GetWindow();
            if (!_pWindow)
                return nullptr;

            auto _pDrawContext = _pWindow->GetDrawContext();
            if (!_pDrawContext)
                return nullptr;

            auto _szDisplayText = BuildDisplayText();
            return _pDrawContext->CreateTextLayout(_szDisplayText, SpecFont, _ConstraintSize, fSpecContentAlign);
        }

        void __YYAPI TextBox::PaintContent(DrawContext* _pDrawContext, const Value& _Content, const Font& _FontInfo, Color _ForegroundColor, const Rect& _Bounds, ContentAlignStyle _fContentAlign)
        {
            auto _pTextLayout = CreateActiveTextLayout(_Bounds.GetSize());
            if (!_pTextLayout)
                return;

            Rect _CaretRect;
            TextBoxSelectedItems _SecectedItems;
            GetCaret(&_CaretRect, &_SecectedItems);

            for (auto& _Item : _SecectedItems)
            {
                _pDrawContext->DrawRectangle(
                    Rect(Point(_Item.left + _Bounds.Left, _Item.top + _Bounds.Top), Size(_Item.width, _Item.height)),
                    nullptr,
                    SolidColorBrush(Color::MakeRGB(255, 255, 0)));
            }

            _pDrawContext->DrawString2(_Bounds.GetPoint(), _pTextLayout, SolidColorBrush(_ForegroundColor));

            if (!_CaretRect.IsEmpty())
            {
                _CaretRect += Point(_Bounds.Left, _Bounds.Top);
                _pDrawContext->DrawRectangle(_CaretRect, nullptr, SolidColorBrush(Color::MakeRGB(255, 255, 0)));
            }
            // PaintCaret(_pDrawContext, _Bounds.GetPoint());
        }

        void TextBox::UpdateCaret(const Point& _Position, bool _bMoveAnchor)
        {
            Rect _Bounds(Point(0, 0), GetExtent());
            _Bounds.DeflateRect(ApplyFlowDirection(CmpBorderThickness));
            _Bounds.DeflateRect(ApplyFlowDirection(CmpPadding));

            auto _pTextLayout = CreateActiveTextLayout(_Bounds.GetSize());
            if (!_pTextLayout)
                return;

            BOOL _bTrailingHit;
            BOOL _bInside;
            DWRITE_HIT_TEST_METRICS _HitTestMetrics;

            if (FAILED(_pTextLayout->HitTestPoint(_Position.X - _Bounds.Left, _Position.Y - _Bounds.Top, &_bTrailingHit, &_bInside, &_HitTestMetrics)))
                return;

            uint32_t _uDisplayPos;
            if (_bTrailingHit)
            {
                _uDisplayPos = _HitTestMetrics.textPosition + _HitTestMetrics.length;
            }
            else
            {
                _uDisplayPos = _HitTestMetrics.textPosition;
            }

            uCaretPosition = ToCommittedPosition(_uDisplayPos);
            uCaretPosition = NormalizeCaretPosition(GetText(), uCaretPosition);

            if (_bMoveAnchor)
                uCaretAnchor = uCaretPosition;

            ResetPreferredCaretX();

            GetWindow()->InvalidateRect(nullptr);
            // PaintCaret();
            return;
        }

        void TextBox::GetCaret(/*const Point& _oTextLayoutOrigin, */Rect* _pCaretRect, TextBoxSelectedItems* _pSecectedItems)
        {
            _pCaretRect->Clear();
            _pSecectedItems->Clear();

            if (uCaretPosition == UINT32_MAX)
                return;

            Rect _Bounds(Point(0, 0), GetExtent());
            _Bounds.DeflateRect(ApplyFlowDirection(CmpBorderThickness));
            _Bounds.DeflateRect(ApplyFlowDirection(CmpPadding));

            auto _pTextLayout = CreateActiveTextLayout(_Bounds.GetSize());
            if (!_pTextLayout)
                return;

            const auto _uDisplayCaretPosition = ToDisplayPosition(uCaretPosition);
            const auto _uDisplayCaretAnchor = ToDisplayPosition(uCaretAnchor);

            if (_uDisplayCaretAnchor == _uDisplayCaretPosition)
            {
                Rect _Caret;
                DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                if (FAILED(_pTextLayout->HitTestTextPosition(_uDisplayCaretPosition, false, &_Caret.Left, &_Caret.Top, &_HitTestMetrics)))
                    return;

                DWORD _uCaretThickness = 2;
                SystemParametersInfo(SPI_GETCARETWIDTH, 0, &_uCaretThickness, FALSE);

                _Caret.Left = _Caret.Left - _uCaretThickness / 2.0f;
                _Caret.Right = _Caret.Left + _uCaretThickness;
                // _Caret.Top += _oTextLayoutOrigin.Y;
                _Caret.Bottom = _Caret.Top + _HitTestMetrics.height;
                *_pCaretRect = _Caret;

                // _pSecectedItems->EmplacePtr(_HitTestMetrics);
            }
            else if (_uDisplayCaretAnchor < _uDisplayCaretPosition)
            {
                UINT32 _uActualHitTestMetricsCount = _pSecectedItems->GetCapacity();
                _pSecectedItems->Resize(_uActualHitTestMetricsCount);

                auto _hr = _pTextLayout->HitTestTextRange(_uDisplayCaretAnchor, _uDisplayCaretPosition - _uDisplayCaretAnchor, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount);
                if (SUCCEEDED(_hr))
                {
                    _pSecectedItems->Resize(_uActualHitTestMetricsCount);
                    return;
                }

                if (_hr != E_NOT_SUFFICIENT_BUFFER)
                {
                    _pSecectedItems->Clear();
                    return;
                }

                _pSecectedItems->Resize(_uActualHitTestMetricsCount);
                if (FAILED(_pTextLayout->HitTestTextRange(_uDisplayCaretAnchor, _uDisplayCaretPosition - _uDisplayCaretAnchor, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount)))
                {
                    _pSecectedItems->Clear();
                    return;
                }
            }
            else
            {
                UINT32 _uActualHitTestMetricsCount = _pSecectedItems->GetCapacity();
                _pSecectedItems->Resize(_uActualHitTestMetricsCount);

                auto _hr = _pTextLayout->HitTestTextRange(_uDisplayCaretPosition, _uDisplayCaretAnchor - _uDisplayCaretPosition, 0, 0, nullptr, 0, &_uActualHitTestMetricsCount);
                if (SUCCEEDED(_hr))
                {
                    _pSecectedItems->Resize(_uActualHitTestMetricsCount);
                    return;
                }

                if (_hr != E_NOT_SUFFICIENT_BUFFER)
                {
                    _pSecectedItems->Clear();
                    return;
                }

                _pSecectedItems->Resize(_uActualHitTestMetricsCount);
                if (FAILED(_pTextLayout->HitTestTextRange(_uDisplayCaretPosition, _uDisplayCaretAnchor - _uDisplayCaretPosition, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount)))
                {
                    _pSecectedItems->Clear();
                    return;
                }
            }

        }

        bool TextBox::TryGetCaretMetrics(FLOAT* _pnX, FLOAT* _pnY, FLOAT* _pnHeight, Rect* _pBounds, RefPtr<IDWriteTextLayout>* _ppTextLayout)
        {
            if (!_pnX || !_pnY || !_pnHeight)
                return false;

            auto _szText = GetText();
            NormalizeCaret(_szText);

            Rect _Bounds(Point(0, 0), GetExtent());
            _Bounds.DeflateRect(ApplyFlowDirection(CmpBorderThickness));
            _Bounds.DeflateRect(ApplyFlowDirection(CmpPadding));

            auto _pTextLayout = CreateActiveTextLayout(_Bounds.GetSize());
            if (!_pTextLayout)
                return false;

            DWRITE_HIT_TEST_METRICS _HitTestMetrics;
            const auto _uDisplayCaretPosition = ToDisplayPosition(uCaretPosition);
            if (FAILED(_pTextLayout->HitTestTextPosition(_uDisplayCaretPosition, FALSE, _pnX, _pnY, &_HitTestMetrics)))
                return false;

            *_pnHeight = _HitTestMetrics.height;

            if (_pBounds)
                *_pBounds = _Bounds;

            if (_ppTextLayout)
                *_ppTextLayout = _pTextLayout;

            return true;
        }

        bool TextBox::MoveCaretVertically(bool _bMoveDown, bool _bExtendSelection)
        {
            FLOAT _nCurrentX = 0;
            FLOAT _nCurrentY = 0;
            FLOAT _nCurrentHeight = 0;
            RefPtr<IDWriteTextLayout> _pTextLayout;
            if (!TryGetCaretMetrics(&_nCurrentX, &_nCurrentY, &_nCurrentHeight, nullptr, &_pTextLayout))
                return false;

            if (!bPreferredCaretXValid)
            {
                nPreferredCaretX = _nCurrentX;
                bPreferredCaretXValid = true;
            }

            const auto _nTargetY = _bMoveDown ? (_nCurrentY + _nCurrentHeight + 1.0f) : (_nCurrentY - 1.0f);

            BOOL _bTrailingHit = FALSE;
            BOOL _bInside = FALSE;
            DWRITE_HIT_TEST_METRICS _HitTestMetrics;
            if (FAILED(_pTextLayout->HitTestPoint(nPreferredCaretX, _nTargetY, &_bTrailingHit, &_bInside, &_HitTestMetrics)))
                return false;

            uint32_t _uDisplayPosition = _HitTestMetrics.textPosition;
            if (_bTrailingHit)
                _uDisplayPosition += _HitTestMetrics.length;

            auto _szText = GetText();
            const auto _uCommittedPosition = NormalizeCaretPosition(_szText, ToCommittedPosition(_uDisplayPosition));

            if (_bExtendSelection)
            {
                uCaretPosition = _uCommittedPosition;
            }
            else
            {
                uCaretAnchor = uCaretPosition = _uCommittedPosition;
            }

            InvalidateTextBox();
            return true;
        }

        bool TextBox::MoveCaretToVisualLineBoundary(bool _bMoveToEnd, bool _bExtendSelection)
        {
            FLOAT _nCurrentX = 0;
            FLOAT _nCurrentY = 0;
            FLOAT _nCurrentHeight = 0;
            Rect _Bounds;
            RefPtr<IDWriteTextLayout> _pTextLayout;
            if (!TryGetCaretMetrics(&_nCurrentX, &_nCurrentY, &_nCurrentHeight, &_Bounds, &_pTextLayout))
                return false;

            BOOL _bTrailingHit = FALSE;
            BOOL _bInside = FALSE;
            DWRITE_HIT_TEST_METRICS _HitTestMetrics;
            const auto _nTargetX = _bMoveToEnd ? (_Bounds.GetSize().Width + 1.0f) : -1.0f;
            const auto _nTargetY = _nCurrentY + _nCurrentHeight * 0.5f;
            if (FAILED(_pTextLayout->HitTestPoint(_nTargetX, _nTargetY, &_bTrailingHit, &_bInside, &_HitTestMetrics)))
                return false;

            uint32_t _uDisplayPosition = _HitTestMetrics.textPosition;
            if (_bTrailingHit)
                _uDisplayPosition += _HitTestMetrics.length;

            auto _szText = GetText();
            const auto _uCommittedPosition = NormalizeCaretPosition(_szText, ToCommittedPosition(_uDisplayPosition));

            if (_bExtendSelection)
            {
                uCaretPosition = _uCommittedPosition;
            }
            else
            {
                uCaretAnchor = uCaretPosition = _uCommittedPosition;
            }

            ResetPreferredCaretX();
            InvalidateTextBox();
            return true;
        }

        void TextBox::PaintCaret(_In_ DrawContext* _pDrawContext, _In_ const Point& _Origin)
        {
            if (uCaretPosition == UINT32_MAX)
                return;

            if (!pTextLayout)
                return;

            Rect _Caret;
            DWRITE_HIT_TEST_METRICS _HitTestMetrics;
            if (FAILED(pTextLayout->HitTestTextPosition(uCaretPosition, false, &_Caret.Left, &_Caret.Top, &_HitTestMetrics)))
                return;

            DWORD _uCaretThickness = 2;
            SystemParametersInfo(SPI_GETCARETWIDTH, 0, &_uCaretThickness, FALSE);

            _Caret.Left = _Caret.Left + _Origin.X - _uCaretThickness / 2.0f;
            _Caret.Right = _Caret.Left + _uCaretThickness;
            _Caret.Top += _Origin.Y;
            _Caret.Bottom = _Caret.Top + _HitTestMetrics.height;

            _pDrawContext->DrawRectangle(_Caret, nullptr, SolidColorBrush(Color::MakeRGB(255, 255, 0)));
            // SetCaretPos(_CaretPos.X + _Location.X, _CaretPos.Y + _Location.Y);


        }

        bool __YYAPI TextBox::OnLeftButtonDown(const MouseEvent& _Event)
        {
            UpdateCaret(_Event.pt, true);
            return false;
        }

        bool __YYAPI TextBox::OnLeftButtonUp(const MouseEvent& _Event)
        {
            LRESULT _Result = 0;
            return false;
        }

        bool __YYAPI TextBox::OnMouseMove(const MouseEvent& _Event)
        {
            if (HasFlags(_Event.fModifiers, EventModifier::LeftButton))
            {
                UpdateCaret(_Event.pt, false);
            }
            return false;
        }

        bool __YYAPI TextBox::OnKeyboardFocusedPropChanged(OnPropertyChangedHandleData* _pHandleData)
        {
            const auto _bRet = Element::OnKeyboardFocusedPropChanged(_pHandleData);
            LRESULT _Result = 0;

            if (_pHandleData->NewValue.GetType() == ValueType::boolean && _pHandleData->NewValue.GetBool())
            {
                GetWindow()->InvalidateRect(nullptr);
                // ShowCaret(true);
                //pTextServices->TxSendMessage(WM_SETFOCUS, 0, 0, &_Result);
            }
            else
            {
                // ShowCaret(false);
                //pTextServices->TxSendMessage(WM_KILLFOCUS, 0, 0, &_Result);
            }
            return _bRet;
        }

        bool __YYAPI TextBox::OnKeyDown(const KeyboardEvent& _KeyEvent)
        {
            switch (_KeyEvent.vKey)
            {
            case VK_BACK:
                if (DeleteBackwardCore())
                    return true;

                return IsReadOnly();
            case VK_DELETE:
                if (DeleteForwardCore())
                    return true;

                return IsReadOnly();
            case VK_DOWN:
            {
                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                return MoveCaretVertically(true, _bShift);
            }
            case VK_UP:
            {
                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                return MoveCaretVertically(false, _bShift);
            }
            case VK_LEFT:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);
                ResetPreferredCaretX();

                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                uint32_t _uSelectionStart = 0;
                uint32_t _uSelectionEnd = 0;

                if (!_bShift && TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd))
                {
                    uCaretAnchor = uCaretPosition = _uSelectionStart;
                }
                else
                {
                    const auto _uNewPosition = GetPreviousCaretPosition(_szText, uCaretPosition);
                    if (_bShift)
                    {
                        uCaretPosition = _uNewPosition;
                    }
                    else
                    {
                        uCaretAnchor = uCaretPosition = _uNewPosition;
                    }
                }

                InvalidateTextBox();
                break;
            }
            case VK_RIGHT:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);
                ResetPreferredCaretX();

                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                uint32_t _uSelectionStart = 0;
                uint32_t _uSelectionEnd = 0;

                if (!_bShift && TryGetSelectionRange(&_uSelectionStart, &_uSelectionEnd))
                {
                    uCaretAnchor = uCaretPosition = _uSelectionEnd;
                }
                else
                {
                    const auto _uNewPosition = GetNextCaretPosition(_szText, uCaretPosition);
                    if (_bShift)
                    {
                        uCaretPosition = _uNewPosition;
                    }
                    else
                    {
                        uCaretAnchor = uCaretPosition = _uNewPosition;
                    }
                }

                InvalidateTextBox();
                break;
            }
            case VK_HOME:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);

                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                const auto _bCtrl = HasFlags(_KeyEvent.fModifiers, EventModifier::Control);

                if (_bCtrl)
                {
                    if (_bShift)
                    {
                        uCaretPosition = 0;
                    }
                    else
                    {
                        uCaretAnchor = uCaretPosition = 0;
                    }

                    ResetPreferredCaretX();
                    InvalidateTextBox();
                    break;
                }

                if (MoveCaretToVisualLineBoundary(false, _bShift))
                    return true;

                if (_bShift)
                    uCaretPosition = 0;
                else
                    uCaretAnchor = uCaretPosition = 0;

                ResetPreferredCaretX();
                InvalidateTextBox();
                return true;
            }
            case VK_END:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);

                const auto _uTextLength = uint32_t(_szText.GetSize());
                const auto _bShift = HasFlags(_KeyEvent.fModifiers, EventModifier::Shift);
                const auto _bCtrl = HasFlags(_KeyEvent.fModifiers, EventModifier::Control);

                if (_bCtrl)
                {
                    if (_bShift)
                    {
                        uCaretPosition = _uTextLength;
                    }
                    else
                    {
                        uCaretAnchor = uCaretPosition = _uTextLength;
                    }

                    ResetPreferredCaretX();
                    InvalidateTextBox();
                    break;
                }

                if (MoveCaretToVisualLineBoundary(true, _bShift))
                    return true;

                if (_bShift)
                    uCaretPosition = _uTextLength;
                else
                    uCaretAnchor = uCaretPosition = _uTextLength;

                ResetPreferredCaretX();
                InvalidateTextBox();
                return true;
            }
            default:
                return false;
                break;
            }
            return true;
        }

        bool __YYAPI TextBox::OnChar(const KeyboardEvent& _KeyEvent)
        {
            if (_KeyEvent.vKey == VK_TAB)
            {
                if (!IsAcceptsTab())
                    return Element::OnChar(_KeyEvent);

                if (IsReadOnly())
                    return true;

                uString _szTab;
                _szTab.AppendChar(u16char_t('\t'));
                return InsertTextCore(_szTab);
            }

            if (uSuppressNextCharCount)
            {
                --uSuppressNextCharCount;
                return true;
            }

            if (_KeyEvent.vKey == VK_RETURN)
            {
                if (!IsAcceptsReturn())
                    return false;

                if (IsReadOnly())
                    return true;

                uString _szNewLine;
                _szNewLine.AppendChar(u16char_t('\r'));
                _szNewLine.AppendChar(u16char_t('\n'));
                return InsertTextCore(_szNewLine);
            }

            if (_KeyEvent.vKey < 0x20)
                return false;

            if (IsReadOnly())
                return true;

            uString _szText;
            _szText.AppendChar(_KeyEvent.vKey);
            return InsertTextCore(_szText);
        }

        ITextInputClient* __YYAPI TextBox::GetTextInputClient()
        {
            return this;
        }

        //void __YYAPI TextBox::Paint(DrawContext* _pDrawContext, const Rect& _Bounds)
        //{
        //    _pDrawContext->DrawTextServices(pTextServices, _Bounds, 0);
        //}
    }
}
