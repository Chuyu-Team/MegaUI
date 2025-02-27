﻿#pragma once
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class WindowElementAccessibleProvider
            : public ElementAccessibleProvider
            , public IRawElementProviderFragmentRoot
        {
        public:
            WindowElementAccessibleProvider(Element* _pElement, RefPtr<ThreadTaskRunner> _pTaskRunner);

            // IUnknown

            virtual HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID _riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppvObject) override;

            virtual ULONG STDMETHODCALLTYPE AddRef() override;

            virtual ULONG STDMETHODCALLTYPE Release() override;

            virtual HRESULT STDMETHODCALLTYPE GetPatternProvider(
                /* [in] */ PATTERNID _iPatternId,
                /* [retval][out] */ __RPC__deref_out_opt IUnknown** _pRetVal) override;
            
            // IRawElementProviderFragment

            virtual HRESULT STDMETHODCALLTYPE get_BoundingRectangle(
                /* [retval][out] */ __RPC__out struct UiaRect* _pRetVal) override;

            // IRawElementProviderFragmentRoot

            virtual HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(
                /* [in] */ double _X,
                /* [in] */ double _Y,
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** _pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE GetFocus(
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** _pRetVal) override;
            

            // ElementAccessibleProvider

            virtual HRESULT __YYAPI HandlePropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value _OldValue, _In_ Value _NewValue) override;

        };
    }
} // namespace YY

#pragma pack(pop)
