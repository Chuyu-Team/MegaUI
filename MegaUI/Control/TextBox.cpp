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

        void __YYAPI TextBox::PaintContent(DrawContext* _pDrawContext, const Value& _Content, const Font& _FontInfo, Color _ForegroundColor, const Rect& _Bounds, ContentAlignStyle _fContentAlign)
        {
            auto _pTextLayout = GetTextLayout(_Bounds.GetSize());
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
            if (!pTextLayout)
                return;

            Rect _Bounds(Point(0, 0), GetExtent());
            _Bounds.DeflateRect(ApplyFlowDirection(CmpBorderThickness));
            _Bounds.DeflateRect(ApplyFlowDirection(CmpPadding));

            BOOL _bTrailingHit;
            BOOL _bInside;
            DWRITE_HIT_TEST_METRICS _HitTestMetrics;

            if (FAILED(pTextLayout->HitTestPoint(_Position.X - _Bounds.Left, _Position.Y - _Bounds.Top, &_bTrailingHit, &_bInside, &_HitTestMetrics)))
                return;

            if (_bTrailingHit)
            {
                uCaretPosition = _HitTestMetrics.textPosition + _HitTestMetrics.length;
            }
            else
            {
                uCaretPosition = _HitTestMetrics.textPosition;
            }

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

            if (!pTextLayout)
                return;

            if (uCaretAnchor == uCaretPosition)
            {
                Rect _Caret;
                DWRITE_HIT_TEST_METRICS _HitTestMetrics;
                if (FAILED(pTextLayout->HitTestTextPosition(uCaretPosition, false, &_Caret.Left, &_Caret.Top, &_HitTestMetrics)))
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
            else if (uCaretAnchor < uCaretPosition)
            {
                UINT32 _uActualHitTestMetricsCount = _pSecectedItems->GetCapacity();
                _pSecectedItems->Resize(_uActualHitTestMetricsCount);

                auto _hr = pTextLayout->HitTestTextRange(uCaretAnchor, uCaretPosition - uCaretAnchor, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount);
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
                if (FAILED(pTextLayout->HitTestTextRange(uCaretAnchor, uCaretPosition - uCaretAnchor, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount)))
                {
                    _pSecectedItems->Clear();
                    return;
                }
            }
            else
            {
                UINT32 _uActualHitTestMetricsCount = _pSecectedItems->GetCapacity();
                _pSecectedItems->Resize(_uActualHitTestMetricsCount);

                auto _hr = pTextLayout->HitTestTextRange(uCaretPosition, uCaretAnchor - uCaretPosition, 0, 0, nullptr, 0, &_uActualHitTestMetricsCount);
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
                if (FAILED(pTextLayout->HitTestTextRange(uCaretPosition, uCaretAnchor - uCaretPosition, 0, 0, _pSecectedItems->GetData(), _uActualHitTestMetricsCount, &_uActualHitTestMetricsCount)))
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
                // _NavigateType = NavigatingType::First;
                break;
            case VK_END:
                // _NavigateType = NavigatingType::Last;
                break;
            default:
                return false;
                break;
            }
            return true;
        }

        //void __YYAPI TextBox::Paint(DrawContext* _pDrawContext, const Rect& _Bounds)
        //{
        //    _pDrawContext->DrawTextServices(pTextServices, _Bounds, 0);
        //}
    }
}
