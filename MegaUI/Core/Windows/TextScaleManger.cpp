#include "pch.h"
#include <MegaUI/Core/TextScaleManger.h>

#include <windows.ui.viewmanagement.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <YY/Base/Containers/DoublyLinkedList.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;
using ABI::Windows::UI::ViewManagement::UISettings;
using ABI::Windows::UI::ViewManagement::IUISettings;
using ABI::Windows::UI::ViewManagement::IUISettings2;

namespace YY
{
    namespace MegaUI
    {
        class TextScaleManger
        {
            struct TextScaleFactorChangedEntry : public DoublyLinkedListEntryImpl<TextScaleFactorChangedEntry>
            {
                std::function<void(double)> pfnTextScaleFactorChanged;
            };

        private:
            ComPtr<IUISettings2> pUISettings;
            EventRegistrationToken Cookie = {};
            double nTextScaleFactor = 1.0;
            DoublyLinkedList<TextScaleFactorChangedEntry> ObserverList;

        public:
            TextScaleManger()
            {
                HSTRING _hString;
                HSTRING_HEADER _hStringHeader;
                if (FAILED(WindowsCreateStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings, _countof(RuntimeClass_Windows_UI_ViewManagement_UISettings) - 1, &_hStringHeader, &_hString)) || _hString == nullptr)
                    return;

                ComPtr<IInspectable> _pInspectable;
                if (FAILED(RoActivateInstance(_hString, &_pInspectable)) || _pInspectable == nullptr)
                    return;

                if (FAILED(_pInspectable.As(&pUISettings)) || pUISettings == nullptr)
                    return;

                pUISettings->add_TextScaleFactorChanged(
                    Callback<__FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_IInspectable>(this, &TextScaleManger::OnTextScaleFactorChanged).Get(),
                    &Cookie);

                UpdateTextScaleFactor();
            }

            TextScaleManger(const TextScaleManger&) = delete;
            TextScaleManger& operator=(const TextScaleManger&) = delete;

            ~TextScaleManger()
            {
                assert(ObserverList.IsEmpty());

                if (pUISettings && Cookie.value)
                {
                    pUISettings->remove_TextScaleFactorChanged(Cookie);
                }
            }

            static TextScaleManger* __YYAPI Get() noexcept
            {
                static TextScaleManger s_Manger;
                return &s_Manger;
            }

            double __YYAPI GetTextScaleFactor() noexcept
            {
                return nTextScaleFactor;
            }

            void* __YYAPI AddTextScaleFactorChanged(std::function<void(float)> _pfnTextScaleFactorChanged) noexcept
            {
                if (!_pfnTextScaleFactorChanged)
                    return nullptr;

                auto _pEntry = new (std::nothrow) TextScaleFactorChangedEntry;
                if (_pEntry)
                {
                    _pEntry->pfnTextScaleFactorChanged = std::move(_pfnTextScaleFactorChanged);
                    ObserverList.PushBack(_pEntry);
                }
                return _pEntry;
            }

            void __YYAPI RemoveTextScaleFactorChanged(void* _pCookie) noexcept
            {
                auto _pEntry = reinterpret_cast<TextScaleFactorChangedEntry*>(_pCookie);
                if (_pEntry)
                {
                    ObserverList.Remove(_pEntry);
                    delete _pEntry;
                }
            }

        private:
            HRESULT OnTextScaleFactorChanged(IUISettings*, IInspectable*) noexcept
            {
                if (UpdateTextScaleFactor())
                {
                    for (auto _pItem = ObserverList.GetFirst(); _pItem; _pItem = _pItem->pNext)
                    {
                        _pItem->pfnTextScaleFactorChanged(nTextScaleFactor);
                    }
                }

                return S_OK;
            }

            bool __YYAPI UpdateTextScaleFactor() noexcept
            {
                double _nTextScaleFactor = 1.0;
                if (SUCCEEDED(pUISettings->get_TextScaleFactor(&_nTextScaleFactor)) && _nTextScaleFactor)
                {
                    if (nTextScaleFactor == _nTextScaleFactor)
                        return false;

                    nTextScaleFactor = _nTextScaleFactor;
                    return true;
                }
                return false;
            }
        };

        float __YYAPI GetSystemTextScale() noexcept
        {
            return TextScaleManger::Get()->GetTextScaleFactor();
        }

        void* __YYAPI AddTextScaleFactorChanged(std::function<void(float)> _pfnTextScaleFactorChanged) noexcept
        {
            return TextScaleManger::Get()->AddTextScaleFactorChanged(std::move(_pfnTextScaleFactorChanged));
        }

        void __YYAPI RemoveTextScaleFactorChanged(void* _pCookie) noexcept
        {
            TextScaleManger::Get()->RemoveTextScaleFactorChanged(_pCookie);
        }
    }
} // namespace YY
