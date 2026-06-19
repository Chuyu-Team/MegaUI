#include "pch.h"
#include <MegaUI/Control/TextBox.h>

#include <MegaUI/Window/Window.h>
#include <MegaUI/Core/ControlInfoImp.h>

namespace YY
{
    namespace MegaUI
    {
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
            const auto _uLength = uint32_t(_szText.GetSize());

            if (uCaretPosition == UINT32_MAX || uCaretPosition > _uLength)
                uCaretPosition = _uLength;

            if (uCaretAnchor == UINT32_MAX || uCaretAnchor > _uLength)
                uCaretAnchor = uCaretPosition;
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
            InvalidateTextBox();
            return true;
        }

        uint32_t TextBox::GetDeleteBackwardLength(const uString& _szText) const
        {
            if (uCaretPosition == 0 || uCaretPosition > _szText.GetSize())
                return 0;

            if (uCaretPosition >= 2)
            {
                const auto _uLow = uint32_t(_szText[uCaretPosition - 1]);
                const auto _uHigh = uint32_t(_szText[uCaretPosition - 2]);
                if (_uHigh >= 0xD800 && _uHigh <= 0xDBFF && _uLow >= 0xDC00 && _uLow <= 0xDFFF)
                    return 2;
            }

            return 1;
        }

        uint32_t TextBox::GetDeleteForwardLength(const uString& _szText) const
        {
            if (uCaretPosition >= _szText.GetSize())
                return 0;

            if (uCaretPosition + 1 < _szText.GetSize())
            {
                const auto _uHigh = uint32_t(_szText[uCaretPosition]);
                const auto _uLow = uint32_t(_szText[uCaretPosition + 1]);
                if (_uHigh >= 0xD800 && _uHigh <= 0xDBFF && _uLow >= 0xDC00 && _uLow <= 0xDFFF)
                    return 2;
            }

            return 1;
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

            if (_bMoveAnchor)
                uCaretAnchor = uCaretPosition;

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
                // _NavigateType = NavigatingType::Down;
                break;
            case VK_UP:
                // _NavigateType = NavigatingType::Up;
                break;
            case VK_LEFT:
                if (uCaretPosition == UINT32_MAX)
                {
                    uCaretAnchor = 0;
                    uCaretPosition = 0;
                }
                else if (uCaretAnchor != uCaretPosition)
                {
                    if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                    {
                        if (uCaretAnchor != 0)
                        {
                            FLOAT nPointX, nPointY;
                            DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                            if (SUCCEEDED(pTextLayout->HitTestTextPosition(uCaretAnchor - 1, TRUE, &nPointX, &nPointY, &_HitTestMetrics)))
                            {
                                uCaretAnchor = _HitTestMetrics.textPosition;
                            }
                        }
                    }
                    else
                    {
                        uCaretPosition = uCaretAnchor;
                    }
                }
                else
                {
                    if (uCaretAnchor != 0)
                    {
                        FLOAT nPointX, nPointY;
                        DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                        if (SUCCEEDED(pTextLayout->HitTestTextPosition(uCaretAnchor - 1, FALSE, &nPointX, &nPointY, &_HitTestMetrics)))
                        {
                            if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                            {
                                uCaretAnchor = _HitTestMetrics.textPosition;
                            }
                            else
                            {
                                uCaretAnchor = uCaretPosition = _HitTestMetrics.textPosition;
                            }
                        }
                    }
                }
                GetWindow()->InvalidateRect(nullptr);
                break;
            case VK_RIGHT:
                if (uCaretPosition == UINT32_MAX)
                {
                    uCaretAnchor = 0;
                    uCaretPosition = 0;
                }
                else if (uCaretAnchor != uCaretPosition)
                {
                    if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                    {
                        FLOAT nPointX, nPointY;
                        DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                        if (SUCCEEDED(pTextLayout->HitTestTextPosition(uCaretAnchor, TRUE, &nPointX, &nPointY, &_HitTestMetrics)))
                        {
                            uCaretAnchor = _HitTestMetrics.textPosition + _HitTestMetrics.length;
                        }
                    }
                    else
                    {
                        uCaretAnchor = uCaretPosition;
                    }
                }
                else
                {
                    FLOAT nPointX, nPointY;
                    DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                    if (SUCCEEDED(pTextLayout->HitTestTextPosition(uCaretPosition, TRUE, &nPointX, &nPointY, &_HitTestMetrics)))
                    {
                        if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                        {
                            uCaretAnchor = _HitTestMetrics.textPosition + _HitTestMetrics.length;
                        }
                        else
                        {
                            uCaretAnchor = uCaretPosition = _HitTestMetrics.textPosition + _HitTestMetrics.length;
                        }
                    }
                }
                GetWindow()->InvalidateRect(nullptr);
                break;
            case VK_HOME:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);

                if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                {
                    if (uCaretAnchor == UINT32_MAX)
                        uCaretAnchor = uCaretPosition;

                    uCaretPosition = 0;
                }
                else
                {
                    uCaretAnchor = uCaretPosition = 0;
                }

                InvalidateTextBox();
                break;
            }
            case VK_END:
            {
                auto _szText = GetText();
                NormalizeCaret(_szText);

                const auto _uTextLength = uint32_t(_szText.GetSize());
                if (HasFlags(_KeyEvent.fModifiers, EventModifier::Shift))
                {
                    if (uCaretAnchor == UINT32_MAX)
                        uCaretAnchor = uCaretPosition;

                    uCaretPosition = _uTextLength;
                }
                else
                {
                    uCaretAnchor = uCaretPosition = _uTextLength;
                }

                InvalidateTextBox();
                break;
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
                return Element::OnChar(_KeyEvent);

            if (uSuppressNextCharCount)
            {
                --uSuppressNextCharCount;
                return true;
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
