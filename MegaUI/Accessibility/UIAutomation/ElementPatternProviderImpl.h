#pragma once
#include <UIAutomationClient.h>
#include <UIAutomationCore.h>

#include <MegaUI/Accessibility/UIAutomation/PatternProviderImpl.h>
#include <MegaUI/core/Element.h>
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        #if 0
        template<>
        class PatternProvider<Element, ITextRangeProvider2>
            : public ComUnknowImpl<PatternProvider<Element, ITextRangeProvider2>, ITextRangeProvider2>
        {
        protected:
            ThreadTaskRunner TaskRunner;
            Element* pElement;
            uString szContent;
            size_t uStart;
            size_t uSize;

        public:
            PatternProvider(Element* _pElement, ThreadTaskRunner _TaskRunner, uString _szContent, size_t _uStart, size_t _uSize)
                : TaskRunner(std::move(_TaskRunner))
                , pElement(_pElement)
                , szContent(_szContent)
                , uStart(_uStart)
                , uSize(_uSize)
            {
            }

            PatternProvider(Element* _pElement, ThreadTaskRunner _TaskRunner, uString _szContent)
                : PatternProvider(_pElement, _TaskRunner, _szContent, 0, _szContent.GetSize())
            {
            }

            __YY_BEGIN_COM_QUERY_MAP(PatternProvider)
                __YY_QUERY_ENTRY(IUnknown)
                __YY_QUERY_ENTRY(ITextRangeProvider)
                __YY_QUERY_ENTRY(ITextRangeProvider2)
            __YY_END_COM_QUERY_MAP();


            static HorizontalTextAlignment __YYAPI ContentAlignStyleToHorizontalTextAlignment(ContentAlignStyle _fStyle)
            {
                if (HasFlags(_fStyle, ContentAlignStyle::Right))
                {
                    return HorizontalTextAlignment::HorizontalTextAlignment_Right;
                }
                else if (HasFlags(_fStyle, ContentAlignStyle::Center))
                {
                    return HorizontalTextAlignment::HorizontalTextAlignment_Centered;
                }
                else
                {
                    return HorizontalTextAlignment::HorizontalTextAlignment_Left;
                }
            }



            // ITextRangeProvider

            virtual HRESULT STDMETHODCALLTYPE Clone(
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;

                *_ppRetVal = nullptr;

                auto _pNewProvider = new(std::nothrow) PatternProvider(*this);
                if (!_pNewProvider)
                    return E_OUTOFMEMORY;

                *_ppRetVal = _pNewProvider;
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE Compare(
                /* [in] */ __RPC__in_opt ITextRangeProvider* _pRange,
                /* [retval][out] */ __RPC__out BOOL* _pbRetVal) override
            {
                if (!_pbRetVal)
                    return E_INVALIDARG;
                *_pbRetVal = FALSE;

                auto _pOther = dynamic_cast<PatternProvider*>(_pRange);
                if (!_pOther)
                    return S_OK;

                if (_pOther == this)
                {
                    *_pbRetVal = TRUE;
                }
                else
                {
                    *_pbRetVal = _pOther->pElement == pElement && _pOther->uStart == uStart && _pOther->uSize == uSize;
                }
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE CompareEndpoints(
                /* [in] */ enum TextPatternRangeEndpoint _eEndpoint,
                /* [in] */ __RPC__in_opt ITextRangeProvider* _pTargetRange,
                /* [in] */ enum TextPatternRangeEndpoint _eTargetEndpoint,
                /* [retval][out] */ __RPC__out int* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = 0;
                
                auto _pOther = dynamic_cast<PatternProvider*>(_pTargetRange);
                if (!_pOther)
                    return E_INVALIDARG;

                if (pElement != _pOther->pElement)
                    return E_INVALIDARG;

                size_t _uLeft = 0;
                switch (_eEndpoint)
                {
                case TextPatternRangeEndpoint_Start:
                    _uLeft = uStart;
                    break;
                case TextPatternRangeEndpoint_End:
                    _uLeft = uStart + uSize;
                    break;
                default:
                    return E_INVALIDARG;
                    break;
                }

                size_t _uRight = 0;
                switch (_eTargetEndpoint)
                {
                case TextPatternRangeEndpoint_Start:
                    _uRight = _pOther->uStart;
                    break;
                case TextPatternRangeEndpoint_End:
                    _uRight = _pOther->uStart + _pOther->uSize;
                    break;
                default:
                    return E_INVALIDARG;
                    break;
                }
                
                *_pRetVal = (int)(_uLeft - _uRight);
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(
                /* [in] */ enum TextUnit _eUnit) override
            {
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE FindAttribute(
                /* [in] */ TEXTATTRIBUTEID _iAttributeId,
                /* [in] */ VARIANT _Val,
                /* [in] */ BOOL _bBackward,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_NOINTERFACE;

                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE FindText(
                /* [in] */ __RPC__in BSTR _szText,
                /* [in] */ BOOL _bBackward,
                /* [in] */ BOOL _bIgnoreCase,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_NOINTERFACE;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE GetAttributeValue(
                /* [in] */ TEXTATTRIBUTEID _iAttributeId,
                /* [retval][out] */ __RPC__out VARIANT* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                _pRetVal->vt = VT_EMPTY;

                HRESULT _hr = E_FAIL;
                pTaskRunner->Sync(
                    [=, &_hr]()
                    {
                        switch (_iAttributeId)
                        {
                        case UIA_BackgroundColorAttributeId:
                        {
                            auto _Background = pElement->GetValue(Element::g_ControlInfoData.BackgroundProp);
                            if (_Background.GetType() == ValueType::Color)
                            {
                                _hr = VariantSetInt32(_pRetVal, (int32_t)_Background.GetColor().ColorRGBA);
                            }
                            break;
                        }
                        case UIA_FontNameAttributeId:
                            _hr = VariantSetString(_pRetVal, pElement->GetFontFamily());
                            break;
                        case UIA_FontSizeAttributeId:
                            _hr = VariantSetDouble(_pRetVal, pElement->GetFontSize());
                            break;
                        case UIA_FontWeightAttributeId:
                            _hr = VariantSetInt32(_pRetVal, pElement->GetFontWeight());
                            break;
                        case UIA_ForegroundColorAttributeId:
                        {
                            auto _ForegroundValue = pElement->GetValue(Element::g_ControlInfoData.ForegroundProp);
                            if (_ForegroundValue.GetType() == ValueType::Color)
                            {
                                _hr = VariantSetInt32(_pRetVal, (int32_t)_ForegroundValue.GetColor().ColorRGBA);
                            }
                            break;
                        }
                        case UIA_HorizontalTextAlignmentAttributeId:
                            _hr = VariantSetInt32(_pRetVal, ContentAlignStyleToHorizontalTextAlignment(pElement->GetContentAlign()));
                            break;
                        case UIA_IsItalicAttributeId:
                            _hr = VariantSetBool(_pRetVal, HasFlags(pElement->GetFontStyle(), FontStyle::Italic));
                            break;
                        case UIA_IsReadOnlyAttributeId:
                            _hr = VariantSetBool(_pRetVal, true);
                            break;
                        }
                    });

                return _hr;                
            }

            virtual HRESULT STDMETHODCALLTYPE GetBoundingRectangles(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE GetEnclosingElement(
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderSimple** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE GetText(
                /* [in] */ int _iMaxLength,
                /* [retval][out] */ __RPC__deref_out_opt BSTR* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = nullptr;

                const size_t _cchTextRead = _iMaxLength < 0 || size_t(_iMaxLength) >= uSize ? uSize : size_t(_iMaxLength);
                if (_cchTextRead > uint32_max)
                    return E_UNEXPECTED;

                auto _szText = SysAllocStringLen(nullptr, _cchTextRead);
                if (!_szText)
                    return E_OUTOFMEMORY;
                memcpy(_szText, szContent.GetConstString() + uStart, uSize * sizeof(szContent.GetConstString()[0]));
                *_pRetVal = _szText;
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE Move(
                /* [in] */ enum TextUnit _eUnit,
                /* [in] */ int _iCount,
                /* [retval][out] */ __RPC__out int* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = 0;
                
                auto _szSrc = szContent.GetConstString() + uStart;

                if (_iCount == 0)
                {
                    return E_INVALIDARG;
                }
                else if (_iCount > 0)
                {
                    auto _szEnd = _szSrc + uSize;

                    do
                    {
                        auto _szNext = CharNextW(_szSrc);
                        if (!_szNext)
                            break;

                        if (_szNext >= _szEnd)
                            break;
                        _szSrc = _szNext;
                        --_iCount;
                    } while (_iCount > 0);
                }
                else
                {
                    do
                    {
                        auto _pPrev = CharPrevW(szContent.GetConstString(), _szSrc);
                        if (!_pPrev)
                            break;
                        _szSrc = _pPrev;
                        ++_iCount;
                    } while (_iCount < 0);                    
                }

                auto _uNewStart = _szSrc - szContent.GetConstString();

                *_pRetVal = _uNewStart - uStart;

            }

            virtual HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(
                /* [in] */ enum TextPatternRangeEndpoint endpoint,
                /* [in] */ enum TextUnit unit,
                /* [in] */ int count,
                /* [retval][out] */ __RPC__out int* pRetVal) = 0;

            virtual HRESULT STDMETHODCALLTYPE MoveEndpointByRange(
                /* [in] */ enum TextPatternRangeEndpoint endpoint,
                /* [in] */ __RPC__in_opt ITextRangeProvider* targetRange,
                /* [in] */ enum TextPatternRangeEndpoint targetEndpoint) = 0;

            virtual HRESULT STDMETHODCALLTYPE Select(void) = 0;

            virtual HRESULT STDMETHODCALLTYPE AddToSelection(void) = 0;

            virtual HRESULT STDMETHODCALLTYPE RemoveFromSelection(void) = 0;

            virtual HRESULT STDMETHODCALLTYPE ScrollIntoView(
                /* [in] */ BOOL alignToTop) = 0;

            virtual HRESULT STDMETHODCALLTYPE GetChildren(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) = 0;
        };

        template<>
        class PatternProvider<Element, ITextProvider2>
            : public ComUnknowImpl<PatternProvider<Element, ITextProvider2>, ITextProvider2>
        {
        protected:
            ThreadTaskRunner TaskRunner;
            Element* pElement;
            
        public:
            PatternProvider(Element* _pElement, ThreadTaskRunner _TaskRunner)
                : TaskRunner(std::move(_TaskRunner))
                , pElement(_pElement)
            {
            }

            __YY_BEGIN_COM_QUERY_MAP(PatternProvider)
                __YY_QUERY_ENTRY(IUnknown)
                __YY_QUERY_ENTRY(ITextProvider)
                __YY_QUERY_ENTRY(ITextProvider2)
            __YY_END_COM_QUERY_MAP();

            // ITextProvider

            virtual HRESULT STDMETHODCALLTYPE GetSelection(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE GetVisibleRanges(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE RangeFromChild(
                /* [in] */ __RPC__in_opt IRawElementProviderSimple* _pChildElement,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE RangeFromPoint(
                /* [in] */ struct UiaPoint _Point,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;

                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DocumentRange(
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                HRESULT _hr = E_FAIL;
                pTaskRunner->Sync(
                    [=, &_hr]()
                    {
                        auto _szDisplayed = pElement->GetContentStringAsDisplayed();

                        auto _pTextRangeProvider = new(std::nothrow) PatternProvider<Element, ITextRangeProvider2>(pElement, TaskRunner, _szDisplayed);
                        if (!_pTextRangeProvider)
                        {
                            _hr = E_OUTOFMEMORY;
                            return;
                        }

                        *_ppRetVal = _pTextRangeProvider;
                        _hr = S_OK;
                    });

                return _hr;
            }

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(
                /* [retval][out] */ __RPC__out enum SupportedTextSelection* _pRetVal) override
            {
                if (!_pRetVal)
                    return E_INVALIDARG;
                *_pRetVal = SupportedTextSelection::SupportedTextSelection_None;
                return S_OK;
            }

            // ITextProvider2

            virtual HRESULT STDMETHODCALLTYPE RangeFromAnnotation(
                /* [in] */ __RPC__in_opt IRawElementProviderSimple* _pAnnotationElement,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (!_ppRetVal)
                    return E_INVALIDARG;
                *_ppRetVal = nullptr;
                return E_NOTIMPL;
            }

            virtual HRESULT STDMETHODCALLTYPE GetCaretRange(
                /* [out] */ __RPC__out BOOL* _pbIsActive,
                /* [retval][out] */ __RPC__deref_out_opt ITextRangeProvider** _ppRetVal) override
            {
                if (_pbIsActive)
                    *_pbIsActive = FALSE;

                if (_ppRetVal)
                    *_ppRetVal = nullptr;

                if (_ppRetVal == nullptr || _pbIsActive == nullptr)
                    return E_INVALIDARG;

                return E_NOTIMPL;
            }
        };
        #endif

        template<>
        class PatternProvider<Element, IInvokeProvider>
            : public PatternProviderBase<PatternProvider<Element, IInvokeProvider>, Element, IInvokeProvider>
        {            
        public:
            PatternProvider(_In_ ElementAccessibleProvider* _pProvider)
                : PatternProviderBase(_pProvider)
            {
            }

            __YY_BEGIN_COM_QUERY_MAP(PatternProvider)
                __YY_QUERY_ENTRY(IUnknown)
                __YY_QUERY_ENTRY(IInvokeProvider)
            __YY_END_COM_QUERY_MAP();

            static bool __YYAPI IsPatternSupported(_In_ Element* _pElement)
            {
                if (!_pElement)
                    return false;

                auto _eAccRole = _pElement->GetAccRole();
                switch (_eAccRole)
                {
                case AccessibleRole::Button:
                    return true;
                default:
                    return false;
                }
            }

            // IInvokeProvider

            virtual HRESULT STDMETHODCALLTYPE Invoke() override
            {
                pTaskRunner->Async(
                    [](void* _pUserData)
                    {
                        auto _pElement = (Element*)_pUserData;
                        _pElement->DefaultAction();
                    },
                    pElement);

                return S_OK;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
