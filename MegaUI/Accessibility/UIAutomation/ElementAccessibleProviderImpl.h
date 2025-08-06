#pragma once

#include <combaseapi.h>
#include <UIAutomationCore.h>
#include <UIAutomationClient.h>
#include <UIAutomationCoreApi.h>

#include <YY/Base/Sync/Interlocked.h>
#include <YY/Base/Threading/TaskRunner.h>

#include <MegaUI/core/Element.h>
#include <YY/Base/Utils/ComObjectImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        constexpr auto UIA_FirstPatternId = UIA_InvokePatternId;
        constexpr auto UIA_LastPatternId = UIA_SelectionPattern2Id;


        inline HRESULT __YYAPI VariantSetString(_Out_ VARIANT* _pVariant, _In_z_ const u16char_t* _szValue)
	    {
		    _pVariant->vt = VT_EMPTY;

		    if (_szValue && *_szValue)
		    {
			    auto _szValueCopy = SysAllocString(_szValue);
			    if (!_szValueCopy)
				    return E_OUTOFMEMORY;

			    _pVariant->bstrVal = _szValueCopy;
			    _pVariant->vt = VT_BSTR;
		    }

		    return S_OK;
	    }
        
        /// <summary>
        /// 将一个纯ASCII字符串设置到 VARIANT中。 
        /// </summary>
        /// <param name="_pVariant"></param>
        /// <param name="_szValue"></param>
        /// <returns></returns>
        inline HRESULT __YYAPI VariantSetStringASCII(_Out_ VARIANT* _pVariant, _In_z_ const achar_t* _szValue)
        {
            _pVariant->vt = VT_EMPTY;

            if (!_szValue)
            {
                return S_OK;
            }

            const auto _cchValue = strlen(_szValue);
            if (_cchValue > (std::numeric_limits<UINT>::max)())
                return E_UNEXPECTED;

            auto _szValueCopy = SysAllocStringLen(nullptr, (UINT)_cchValue);
            if(!_szValueCopy)
                return E_OUTOFMEMORY;

            for (size_t i = 0; i != _cchValue; ++i)
            {
                _szValueCopy[i] = _szValue[i];
            }
            _szValueCopy[_cchValue] = L'\0';

            _pVariant->bstrVal = _szValueCopy;
            _pVariant->vt = VT_BSTR;
            return S_OK;
        }
        
        inline HRESULT __YYAPI VariantSetInt32(_Out_ VARIANT* _pVariant, _In_ int32_t _iValue)
        {
            _pVariant->vt = VT_I4;
            _pVariant->lVal = _iValue;
            return S_OK;
        }

        inline HRESULT __YYAPI VariantSetBool(_Out_ VARIANT* _pVariant, _In_ bool _bValue)
        {
            _pVariant->vt = VT_BOOL;
            _pVariant->boolVal = _bValue ? -1 : 0;
            return S_OK;
        }

        inline HRESULT __YYAPI VariantSetDouble(_Out_ VARIANT* _pVariant, _In_ double _Value)
        {
            _pVariant->vt = VT_I8;
            _pVariant->dblVal = _Value;
            return S_OK;
        }

        template<class _Type>
        struct VariantHelp
        {
        };

        template<>
        struct VariantHelp<double>
        {
            static constexpr VARTYPE vt = VT_R8;
        };

        template<>
        struct VariantHelp<int32_t>
        {
            static constexpr VARTYPE vt = VT_I4;
        };

        template<class ArryItemType, class _ValueType>
        inline HRESULT __YYAPI VariantSetSafeArray(_Out_ VARIANT* _pVariant, _In_reads_(_uCount) _ValueType* _pValue, _In_ size_t _uCount)
        {
            _pVariant->vt = VT_EMPTY;

            if (_uCount == 0)
                return S_FALSE;

            if (_uCount > (std::numeric_limits<ULONG>::max)())
                return E_UNEXPECTED;

            constexpr VARTYPE _vt = VariantHelp<ArryItemType>::vt;

            auto _pSafeArry = SafeArrayCreateVector(_vt, 0, (ULONG)_uCount);
            if (!_pSafeArry)
                return E_OUTOFMEMORY;

            ArryItemType* _pData;
            auto _hr = SafeArrayAccessData(_pSafeArry, (void**)&_pData);
            if (SUCCEEDED(_hr))
            {
                for (size_t _uIndex = 0; _uIndex != _uCount; ++_uIndex)
                {
                    _pData[_uIndex] = _pValue[_uIndex];
                }

                _hr = SafeArrayUnaccessData(_pSafeArry);
            }

            if (SUCCEEDED(_hr))
            {
                _pVariant->vt = VT_ARRAY | _vt;
                _pVariant->parray = _pSafeArry;
                return S_OK;
            }

            SafeArrayDestroy(_pSafeArry);
            return _hr;
        }

        inline HRESULT __YYAPI VariantSetPoint(_Out_ VARIANT* _pVariant, _In_ Point _ptValue)
        {
            return VariantSetSafeArray<double>(_pVariant, &_ptValue.X, 2);
        }

        inline HRESULT __YYAPI VariantSetUiaRect(_Out_ VARIANT* _pVariant, _In_ Rect& _Value)
        {
            double _UiaRect[4] = { _Value.Left, _Value.Top, _Value.GetWidth(), _Value.GetHeight() };
            return VariantSetSafeArray<double>(_pVariant, &_UiaRect[0], std::size(_UiaRect));
        }

        union RuntimeId
        {
            struct
            {
                uint64_t pElementPtr64;
                uint32_t uProcessId;
                uint32_t uReserve;
            };

            Element* pElement;
            int32_t UiaIds[4];
        };

        inline bool operator==(const RuntimeId& _Left, const RuntimeId& _Right)
        {
            return _Left.pElementPtr64 == _Right.pElementPtr64 && _Left.uProcessId == _Right.uProcessId;
        }

        class ElementAccessibleProvider
            : public ComObjectImpl<ElementAccessibleProvider, ComObjectInheritChain<IRawElementProviderSimple3, IRawElementProviderSimple2, IRawElementProviderSimple>, IRawElementProviderFragment, IRawElementProviderAdviseEvents>
        {
        protected:
            RefPtr<ThreadTaskRunner> pTaskRunner;
            Element* pElement;
            // GetPatternProvider 接口的缓存
            IUnknown* PatternProviderCache[UIA_LastPatternId - UIA_FirstPatternId + 1];

        public:
            ElementAccessibleProvider(Element* _pElement, RefPtr<YY::Base::Threading::ThreadTaskRunner> _pTaskRunner);

            virtual ~ElementAccessibleProvider();

            Element* __YYAPI GetElement();

            RefPtr<ThreadTaskRunner> __YYAPI GetTaskRunner();

            static int32_t __YYAPI AccessibleRoleToControlType(AccessibleRole _eRole);

            uString __YYAPI GetName();

            static void __YYAPI GetShortcutString(achar_t _ch, u16char_t* _szBuffer, size_t _cchBuffer);

            bool __YYAPI IsOffscreen();

            Rect __YYAPI GetBoundingRectangle();

            static Element* __YYAPI GetVisibleAccessibleParent(Element* _pElem);
            
            static bool __YYAPI IsAccessibleAncestor(Element* pElem, Element* _pAccessibleParent)
            {
                for (; pElem; pElem = GetVisibleAccessibleParent(pElem))
                {
                    if (_pAccessibleParent == pElem)
                        return true;
                }

                return false;
            }

            static HRESULT __YYAPI ForEachAccessibleChildren(
                Element* _pParentElement,
                bool (__YYAPI* _pCallback)(Element* _pChildElement, void* _pUserData),
                void* _pUserData);

            template<class Fun>
            static HRESULT __YYAPI ForEachAccessibleChildren(
                Element* _pParentElement,
                Fun&& _Fun)
            {
                return ForEachAccessibleChildren(
                    _pParentElement,
                    [](Element* _pChildElement, void* _pUserData) -> bool
                    {
                        return (*(Fun*)_pUserData)(_pChildElement);
                    },
                    &_Fun);
            }
            
            static RuntimeId __YYAPI GetElementRuntimeId(Element* _pElement);

            ////////////////////////////////////////////////////////////////
            // IRawElementProviderSimple

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProviderOptions(
                /* [retval][out] */ __RPC__out enum ProviderOptions* _peRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE GetPatternProvider(
                /* [in] */ PATTERNID _iPatternId,
                /* [retval][out] */ __RPC__deref_out_opt IUnknown** _pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
                /* [in] */ PROPERTYID _iPropertyId,
                /* [retval][out] */ __RPC__out VARIANT* _pRetVal) override;

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderSimple** _pRetVal) override;

            // IRawElementProviderSimple2

            virtual HRESULT STDMETHODCALLTYPE ShowContextMenu() override;

            // IRawElementProviderSimple3

            virtual HRESULT STDMETHODCALLTYPE GetMetadataValue(
                /* [in] */ int targetId,
                /* [in] */ METADATAID metadataId,
                /* [retval][out] */ __RPC__out VARIANT* returnVal) override;

            ///////////////////////////////////////////////////////////
            // IRawElementProviderFragment

            virtual HRESULT STDMETHODCALLTYPE Navigate(
                /* [in] */ enum NavigateDirection _eDirection,
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragment** _pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE GetRuntimeId(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE get_BoundingRectangle(
                /* [retval][out] */ __RPC__out struct UiaRect* _pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(
                /* [retval][out] */ __RPC__deref_out_opt SAFEARRAY** _pRetVal) override;

            virtual HRESULT STDMETHODCALLTYPE SetFocus() override;

            virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FragmentRoot(
                /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderFragmentRoot** _pRetVal) override;


            //////////////////////////////////////////////////////////
            // IRawElementProviderAdviseEvents
            
            virtual HRESULT STDMETHODCALLTYPE AdviseEventAdded( 
            /* [in] */ EVENTID _iEventId,
                /* [in] */ __RPC__in SAFEARRAY* _pPropertyIds) override;

            virtual HRESULT STDMETHODCALLTYPE AdviseEventRemoved(
                /* [in] */ EVENTID _iEventId,
                /* [in] */ __RPC__in SAFEARRAY* _pPropertyIds) override;
            
            
            virtual HRESULT __YYAPI HandlePropertyChanged(_In_ const PropertyInfo& _Prop, _In_ PropertyIndicies _eIndicies, _In_ Value _OldValue, _In_ Value _NewValue);

        private:
            HRESULT __YYAPI HandleChildrenChanged(ElementList _OldChildren, ElementList _NewChildren);

            HRESULT __YYAPI HandleAccDescriptionChanged(uString _szOldValue, uString _szNewValue);
            
            HRESULT __YYAPI HandleAccRoleChanged(AccessibleRole _eOldValue, AccessibleRole _eNewValue);
        };

    }
} // namespace YY

#pragma pack(pop)
