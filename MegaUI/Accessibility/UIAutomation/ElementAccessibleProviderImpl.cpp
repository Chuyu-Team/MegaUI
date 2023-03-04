#include "pch.h"
#include "ElementAccessibleProviderImpl.h"

#include <MegaUI/Window/Window.h>
#include <MegaUI/Accessibility/UIAutomation/AccessibleEventManager.h>

#pragma warning(disable : 28251)

#pragma comment(lib, "Uiautomationcore.lib")

namespace YY
{
    namespace MegaUI
    {
        ElementAccessibleProvider::ElementAccessibleProvider(Element* _pElement, ThreadTaskRunner _TaskRunner)
            : TaskRunner(std::move(_TaskRunner))
            , pElement(_pElement)
            , PatternProviderCache {}
        {
        }

        ElementAccessibleProvider::~ElementAccessibleProvider()
        {
            for (auto _pPantern : PatternProviderCache)
            {
                if (_pPantern)
                    _pPantern->Release();
            }
        }

        int32_t ElementAccessibleProvider::AccessibleRoleToControlType(AccessibleRole _eRole)
        {
            // UI Automation Map
            static const int32_t g_ControlTypeMap[] =
            {
#define _APPLY(_NAME, _ARIA, _MSAA, _CONTROL_TYPE, ...) _CONTROL_TYPE,
                __ACCESSIBLE_ROLE_TABLE(_APPLY)
#undef _APPLY
            };

            if (std::size(g_ControlTypeMap) <= (uint32_t)_eRole)
                return 0;

            return g_ControlTypeMap[(uint32_t)_eRole];
        }

        uString ElementAccessibleProvider::GetName()
        {
            auto _szAccName = pElement->GetAccNameAsDisplayed();
            if (_szAccName.GetSize())
            {
                return _szAccName;
            }

            if (pElement->IsContentProtected())
                return uString();

            return pElement->GetContentStringAsDisplayed();
        }

        void ElementAccessibleProvider::GetShortcutString(achar_t _ch, u16char_t* _szBuffer, size_t _cchBuffer)
        {
            u16char_t _StaticString[32];

            do
            {
                const auto _uState = MapVirtualKeyW(VK_MENU, MAPVK_VK_TO_VSC);
                if (_uState == 0)
                    break;

                auto _iResultLength = GetKeyNameTextW((_uState | 0x200) << 16, _StaticString, (int)std::size(_StaticString));
                if (_iResultLength <= 0)
                    break;

                if (_cchBuffer < size_t(_iResultLength) + 3)
                {
                    std::abort();
                }

                memcpy(_szBuffer, _StaticString, _iResultLength * sizeof(_szBuffer[0]));
                _szBuffer += _iResultLength;

                *_szBuffer++ = '+';
                *_szBuffer++ = _ch;
                *_szBuffer = '\0';
                return;
            } while (false);

            if (_cchBuffer < 3 + 3)
            {
                std::abort();
            }

            *_szBuffer++ = 'A';
            *_szBuffer++ = 'l';
            *_szBuffer++ = 't';
            *_szBuffer++ = '+';
            *_szBuffer++ = _ch;
            *_szBuffer = '\0';
            return;
        }

        bool ElementAccessibleProvider::IsOffscreen()
        {
            //if (!pElement->IsVisible())
            //    return false;

            Rect _VisibleBounds(pElement->GetLocation(), pElement->GetExtent());

            for (auto _pParent = pElement->GetParent(); _pParent; _pParent = _pParent->GetParent())
            {
                Rect _ParentBounds(_pParent->GetLocation(), _pParent->GetExtent());
                _VisibleBounds += _ParentBounds.GetPoint();
                _VisibleBounds &= _ParentBounds;

                if (_VisibleBounds.IsEmpty())
                    return true;
            }

            return false;
        }

        Rect ElementAccessibleProvider::GetBoundingRectangle()
        {
            Rect _Bounds(pElement->GetLocation(), pElement->GetExtent());

            for (auto _pParent = pElement->GetParent(); _pParent; _pParent = _pParent->GetParent())
            {
                _Bounds += _pParent->GetLocation();
            }

            pElement->GetWindow()->ClientToScreen(&_Bounds);

            return _Bounds;
        }

        Element* ElementAccessibleProvider::GetVisibleAccessibleParent(Element* _pElem)
        {
            if (!_pElem)
                return nullptr;

            while (_pElem = _pElem->GetParent())
            {
                if (_pElem->IsAccessible())
                    break;
            }
            return _pElem;
        }

        HRESULT ElementAccessibleProvider::ForEachAccessibleChildren(
            Element* _pParentElement,
            bool(__YYAPI* _pCallback)(Element* _pChildElement, void* _pUserData),
            void* _pUserData)
        {
            if (_pParentElement == nullptr || _pCallback == nullptr)
                return E_NOINTERFACE;

            for (auto _pChild : _pParentElement->GetChildren())
            {
                if (!_pChild->IsVisible())
                    continue;

                if (_pChild->IsAccessible())
                {
                    if (!_pCallback(_pChild, _pUserData))
                        return __HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }
                else
                {
                    auto _hr = ForEachAccessibleChildren(_pChild, _pCallback, _pUserData);
                    if (FAILED(_hr))
                        return _hr;
                }
            }

            return S_OK;
        }

        HRESULT ElementAccessibleProvider::get_ProviderOptions(ProviderOptions* _peRetVal)
        {
            if (!_peRetVal)
                return E_INVALIDARG;

            *_peRetVal = ProviderOptions_ServerSideProvider;
            return S_OK;
        }

        HRESULT ElementAccessibleProvider::GetPatternProvider(PATTERNID _iPatternId, IUnknown** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            const uint32_t _uPatternIndex = UIA_LastPatternId - _iPatternId;
            if (_uPatternIndex >= std::size(PatternProviderCache))
                return E_NOINTERFACE;

            if (auto _pPatternProvider = PatternProviderCache[_uPatternIndex])
            {
                _pPatternProvider->AddRef();
                *_pRetVal = _pPatternProvider;
                return S_OK;
            }

            switch (_iPatternId)
            {
            case UIA_TextPatternId:
            case UIA_TextPattern2Id:

                break;
            }

            // todo:

            return E_NOINTERFACE;
        }

        HRESULT ElementAccessibleProvider::GetPropertyValue(PROPERTYID _iPropertyId, VARIANT* _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;

            _pRetVal->vt = VT_EMPTY;

            if (!pElement->IsAccessible())
            {
                return E_NOTIMPL;
            }

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    _hr = S_OK;

                    switch (_iPropertyId)
                    {
                    case UIA_RuntimeIdPropertyId:
                    {
                        _hr = GetRuntimeId(&_pRetVal->parray);
                        if (SUCCEEDED(_hr))
                            _pRetVal->vt = VT_I4 | VT_ARRAY;

                        return _hr;
                    }
                    case UIA_BoundingRectanglePropertyId:
                    {
                        UiaRect _UIA_BoundingRectangle;
                        get_BoundingRectangle(&_UIA_BoundingRectangle);

                        return _hr = VariantSetSafeArray<double>(_pRetVal, &_UIA_BoundingRectangle.left, 4);
                    }
                    case UIA_ProcessIdPropertyId:
                        return _hr = VariantSetInt32(_pRetVal, (int32_t)GetCurrentProcessId());
                    case UIA_ControlTypePropertyId:
                        return _hr = VariantSetInt32(_pRetVal, AccessibleRoleToControlType(pElement->GetAccRole()));
                    // case UIA_LocalizedControlTypePropertyId:
                    case UIA_NamePropertyId:
                    {
                        auto _szAccName = GetName();
                        if (_szAccName.GetSize() == 0)
                            return _hr = S_FALSE;

                        return _hr = VariantSetString(_pRetVal, _szAccName);
                    }
                    case UIA_AcceleratorKeyPropertyId:
                    case UIA_AccessKeyPropertyId:
                    {
                        auto _ShortcutChar = pElement->GetShortcutChar();
                        if (_ShortcutChar == '\0')
                            return _hr = S_FALSE;

                        u16char_t szShortcutStringBuffer[35];
                        GetShortcutString(_ShortcutChar, szShortcutStringBuffer, std::size(szShortcutStringBuffer));
                        return VariantSetString(_pRetVal, szShortcutStringBuffer);
                    }
                    case UIA_HasKeyboardFocusPropertyId:
                        return _hr = VariantSetBool(_pRetVal, pElement->IsKeyboardFocus());
                    case UIA_IsKeyboardFocusablePropertyId:
                        return _hr = VariantSetBool(_pRetVal, HasFlags(pElement->GetActive(), ActiveStyle::Keyboard));
                    case UIA_IsEnabledPropertyId:
                        return _hr = VariantSetBool(_pRetVal, pElement->IsEnabled());
                    case UIA_AutomationIdPropertyId:
                    {
                        auto _Id = pElement->GetId();
                        if (_Id == 0)
                            return _hr = S_FALSE;

                        u16char_t _szBuffer[256];
                        if (GetAtomNameW(_Id, _szBuffer, _countof(_szBuffer)) <= 0)
                            return _hr = S_FALSE;

                        return _hr = VariantSetString(_pRetVal, _szBuffer);
                    }
                    case UIA_ClassNamePropertyId:
                        return _hr = VariantSetStringASCII(_pRetVal, pElement->GetControlInfo()->GetName());
                    case UIA_HelpTextPropertyId:
                    {
                        auto _szHelpText = pElement->GetAccHelp();
                        if (_szHelpText.GetSize() == 0)
                            _szHelpText = pElement->GetAccDescription();
                        return _hr = VariantSetString(_pRetVal, _szHelpText);
                    }
                    case UIA_ClickablePointPropertyId:
                    {
                        if (pElement->IsVisible() == false || pElement->IsEnabled() == false)
                        {
                            return _hr = S_OK;
                        }

                        Rect _VisibleBounds(pElement->GetLocation(), pElement->GetExtent());

                        for (auto _pParent = pElement->GetParent(); _pParent; _pParent = _pParent->GetParent())
                        {
                            Rect _ParentBounds(_pParent->GetLocation(), _pParent->GetExtent());
                            _VisibleBounds += _ParentBounds.GetPoint();
                            _VisibleBounds &= _ParentBounds;

                            if (_VisibleBounds.IsEmpty())
                                return _hr = S_OK;
                        }

                        return _hr = VariantSetPoint(_pRetVal, _VisibleBounds.GetCenterPoint());
                    }
                    // case UIA_CulturePropertyId:
                    case UIA_IsControlElementPropertyId:
                        return _hr = VariantSetBool(_pRetVal, AccessibleRoleToControlType(pElement->GetAccRole()) != 0);
                    case UIA_IsContentElementPropertyId:
                        // todo: 需要调整为按内容
                        return _hr = VariantSetBool(_pRetVal, AccessibleRoleToControlType(pElement->GetAccRole()) != 0);
                    // case UIA_LabeledByPropertyId:
                    case UIA_IsPasswordPropertyId:
                        return _hr = VariantSetBool(_pRetVal, pElement->IsContentProtected());
                    case UIA_NativeWindowHandlePropertyId:
                        // MegaUI为虚拟控件，一般没有创建句柄
                        if (pElement == pElement->GetWindow()->GetHost())
                            return _hr = VariantSetInt32(_pRetVal, (int32_t)(intptr_t)pElement->GetWindow()->GetWnd());
                        return S_FALSE;
                    case UIA_ItemTypePropertyId:
                        return _hr = VariantSetString(_pRetVal, pElement->GetAccItemType());
                    case UIA_IsOffscreenPropertyId:
                        return _hr = VariantSetBool(_pRetVal, IsOffscreen());
                    // case UIA_OrientationPropertyId:
                    case UIA_FrameworkIdPropertyId:
                        return _hr = VariantSetString(_pRetVal, _U16S("YY::MeauUI"));
                    // case UIA_IsRequiredForFormPropertyId:
                    case UIA_ItemStatusPropertyId:
                        return _hr = VariantSetString(_pRetVal, pElement->GetAccItemStatus());
                        // case UIA_IsDockPatternAvailablePropertyId:
                        //case UIA_IsExpandCollapsePatternAvailablePropertyId:
                        //case UIA_IsGridItemPatternAvailablePropertyId:
                        //case UIA_IsGridPatternAvailablePropertyId:
                        //case UIA_IsInvokePatternAvailablePropertyId:
                        //case UIA_IsMultipleViewPatternAvailablePropertyId:
                        //case UIA_IsRangeValuePatternAvailablePropertyId:
                        //case UIA_IsScrollPatternAvailablePropertyId:
                        // case UIA_IsScrollItemPatternAvailablePropertyId:
                        // case UIA_IsSelectionItemPatternAvailablePropertyId:
                        // case UIA_IsSelectionPatternAvailablePropertyId:
                        //case UIA_IsTablePatternAvailablePropertyId:
                        //case UIA_IsTableItemPatternAvailablePropertyId:
                        //case UIA_IsTextPatternAvailablePropertyId:
                        //case UIA_IsTogglePatternAvailablePropertyId:
                        //case UIA_IsTransformPatternAvailablePropertyId:
                        //case UIA_IsValuePatternAvailablePropertyId:
                        //case UIA_IsWindowPatternAvailablePropertyId:

                    default:
                        return S_OK;
                    }
                });

            return _hr;
        }

        HRESULT ElementAccessibleProvider::get_HostRawElementProvider(IRawElementProviderSimple** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            if (pElement->GetWindow()->GetHost() == pElement)
            {
                return UiaHostProviderFromHwnd(pElement->GetWindow()->GetWnd(), _pRetVal);
            }

            // Element 控件没有实际的窗口

            return E_NOT_SET;
        }
        
        HRESULT ElementAccessibleProvider::ShowContextMenu()
        {
            // Element没有弹出菜单
            return E_NOTIMPL;
        }

        HRESULT ElementAccessibleProvider::GetMetadataValue(int targetId, METADATAID metadataId, VARIANT* returnVal)
        {
            if (returnVal)
                returnVal->vt = VT_EMPTY;

            // todo: 暂时不知道这是怎么用的
            // https://learn.microsoft.com/zh-cn/windows/win32/api/uiautomationcore/ne-uiautomationcore-sayasinterpretas
            return E_NOTIMPL;
        }

        HRESULT ElementAccessibleProvider::GetRuntimeId(SAFEARRAY** pRetVal)
        {
            if (!pRetVal)
                return E_INVALIDARG;
            *pRetVal = nullptr;

            struct
            {
                uint64_t uRawElementPoint;
                uint32_t uProcessId;
            } RuntimeId = {(uint64_t)pElement, GetCurrentProcessId()};

            constexpr auto _uCount = sizeof(RuntimeId) / sizeof(int32_t);
            auto _pSafeArry = SafeArrayCreateVector(VT_I4, 0, _uCount);
            if (!_pSafeArry)
                return E_OUTOFMEMORY;

            int32_t* _pData;
            auto _hr = SafeArrayAccessData(_pSafeArry, (void**)&_pData);
            if (SUCCEEDED(_hr))
            {
                auto _pValue = (int32_t*)&RuntimeId;
                for (size_t _uIndex = 0; _uIndex != _uCount; ++_uIndex)
                {
                    _pData[_uIndex] = _pValue[_uIndex];
                }

                _hr = SafeArrayUnaccessData(_pSafeArry);
            }

            if (SUCCEEDED(_hr))
            {
                *pRetVal = _pSafeArry;
                return S_OK;
            }

            SafeArrayDestroy(_pSafeArry);
            return _hr;
        }

        HRESULT ElementAccessibleProvider::get_BoundingRectangle(UiaRect* _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;

            _pRetVal->left = 0;
            _pRetVal->top = 0;
            _pRetVal->width = 0;
            _pRetVal->height = 0;

            if (!pElement->IsAccessible())
                return S_FALSE;

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    auto _BoundingRectangle = GetBoundingRectangle();
                    _pRetVal->left = _BoundingRectangle.Left;
                    _pRetVal->top = _BoundingRectangle.Top;
                    _pRetVal->width = _BoundingRectangle.GetWidth();
                    _pRetVal->height = _BoundingRectangle.GetHeight();

                    _hr = S_OK;
                });

            return _hr;
        }

        HRESULT __stdcall ElementAccessibleProvider::GetEmbeddedFragmentRoots(SAFEARRAY** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;

            *_pRetVal = nullptr;
            return S_OK;
        }

        HRESULT __stdcall ElementAccessibleProvider::Navigate(NavigateDirection _eDirection, IRawElementProviderFragment** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    _hr = S_OK;

                    Element* _pTarget = nullptr;
                    switch (_eDirection)
                    {
                    case NavigateDirection_Parent:
                        _pTarget = GetVisibleAccessibleParent(pElement);
                        break;
                    case NavigateDirection_NextSibling:
                    {
                        auto _pParent = GetVisibleAccessibleParent(pElement);
                        if (!_pParent)
                        {
                            _hr = E_NOT_SET;
                            return;
                        }

                        bool _bFindCurrent = false;
                        ForEachAccessibleChildren(
                            _pParent,
                            [=, &_bFindCurrent, &_pTarget](Element* _pChildElement) -> bool
                            {
                                if (_pChildElement == pElement)
                                {
                                    _bFindCurrent = true;
                                }
                                else if (_bFindCurrent)
                                {
                                    _pTarget = _pChildElement;
                                    return false;
                                }
                                return true;
                            });

                        break;
                    }
                    case NavigateDirection_PreviousSibling:
                    {
                        auto _pParent = GetVisibleAccessibleParent(pElement);
                        if (!_pParent)
                        {
                            _hr = E_NOT_SET;
                            return;
                        }

                        ForEachAccessibleChildren(
                            _pParent,
                            [=, &_pTarget](Element* _pChildElement) -> bool
                            {
                                if (_pChildElement == pElement)
                                {
                                    return false;
                                }
                                _pTarget = _pChildElement;
                                return true;
                            });

                        break;
                    }
                    
                    case NavigateDirection_FirstChild:
                        ForEachAccessibleChildren(
                            pElement,
                            [=, &_pTarget](Element* _pChildElement) -> bool
                            {
                                _pTarget = _pChildElement;                            
                                return false;
                            });

                        break;
                    case NavigateDirection_LastChild:
                        ForEachAccessibleChildren(
                            pElement,
                            [=, &_pTarget](Element* _pChildElement) -> bool
                            {
                                _pTarget = _pChildElement;
                                return true;
                            });

                        break;
                    }

                    if (_pTarget)
                    {
                        ElementAccessibleProvider* _pAccessibleProvider;
                        _hr = _pTarget->GetAccessibleProvider(&_pAccessibleProvider);
                        if (FAILED(_hr))
                            return;
                        if (!_pAccessibleProvider)
                        {
                            _hr = E_NOINTERFACE;
                            return;
                        }

                        *_pRetVal = static_cast<IRawElementProviderFragment*>(_pAccessibleProvider);
                    }

                    _hr = S_OK;
                });

            return _hr;
        }
        
        HRESULT __stdcall ElementAccessibleProvider::SetFocus()
        {
            HRESULT _hr = E_FAIL;

            TaskRunner.Sync(
                [=, &_hr]()
                {
                    pElement->SetKeyboardFocus();
                    _hr = S_OK;
                });

            return _hr;
        }

        HRESULT __stdcall ElementAccessibleProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** _pRetVal)
        {
            if (!_pRetVal)
                return E_INVALIDARG;
            *_pRetVal = nullptr;

            HRESULT _hr = E_FAIL;
            TaskRunner.Sync(
                [=, &_hr]()
                {
                    auto _pHost = pElement->GetWindow()->GetHost();
                    if (_pHost && _pHost->IsAccessible())
                    {
                        ElementAccessibleProvider* _pAccessibleProvider;
                        _hr = _pHost->GetAccessibleProvider(&_pAccessibleProvider);
                        if (FAILED(_hr))
                            return;

                        if (!_pAccessibleProvider)
                        {
                            _hr = E_FAIL;
                            return;
                        }

                        _hr = _pAccessibleProvider->QueryInterface(__uuidof(IRawElementProviderFragmentRoot), (void**)_pRetVal);
                        _pAccessibleProvider->Release();
                        return;
                    }

                    _hr = E_NOINTERFACE;
                });

            return _hr;
        }

        HRESULT ElementAccessibleProvider::AdviseEventAdded(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            return AccessibleEventManager::AdviseEventAdded(_iEventId, _pPropertyIds);
        }
        
        HRESULT ElementAccessibleProvider::AdviseEventRemoved(EVENTID _iEventId, SAFEARRAY* _pPropertyIds)
        {
            return AccessibleEventManager::AdviseEventRemoved(_iEventId, _pPropertyIds);
        }
    } // namespace MegaUI
} // namespace YY
