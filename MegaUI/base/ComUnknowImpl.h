#pragma once

namespace YY
{
    namespace MegaUI
    {
        struct QueryInterfaceItem
        {
            const IID& Id;
            intptr_t iOffset;
        };
        
#define __YY_QUERY_ENTRY(_TYPE) {__uuidof(_TYPE), reinterpret_cast<intptr_t>(static_cast<_TYPE*>(reinterpret_cast<_ROOT_TPYE*>(16))) - 16},

#define __YY_QUERY_ENTRY_WITH_BASE(_TYPE, _BASE_TYPE) {__uuidof(_TYPE), reinterpret_cast<intptr_t>(static_cast<_TYPE*>(static_cast<_BASE_TYPE*>(reinterpret_cast<_ROOT_TPYE*>(16)))) - 16},

#define __YY_BEGIN_COM_QUERY_MAP(_TYPE)                                         \
    HRESULT STDMETHODCALLTYPE QueryInterfaceImpl(                               \
        /* [in] */ REFIID _riid,                                                \
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppvObject) \
    {                                                                           \
        using _ROOT_TPYE = _TYPE;                                               \
        static const QueryInterfaceItem g_QueryMap[] =                          \
        {

#define __YY_END_COM_QUERY_MAP()                            \
        };                                                  \
        for (auto& Item : g_QueryMap)                       \
        {                                                   \
            if (_riid == Item.Id)                           \
            {                                               \
                AddRef();                                   \
                *_ppvObject = ((char*)this) + Item.iOffset; \
                return S_OK;                                \
            }                                               \
        }                                                   \
        return E_NOINTERFACE;                               \
    }

        template<typename _RootType, typename... _InterfaceTypes>
        class ComUnknowImpl : public _InterfaceTypes...
        {
            uint32_t uRef;

        public:
            ComUnknowImpl()
                : uRef(1u)
            {
            }

            HRESULT STDMETHODCALLTYPE QueryInterfaceImpl(
                /* [in] */ REFIID _riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppvObject)
            {
                return E_NOINTERFACE;
            }

            //////////////////////////////////////////////////////////////
            // IUnknown
            virtual HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID _riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppvObject) override
            {
                if (!_ppvObject)
                    return E_INVALIDARG;
                *_ppvObject = nullptr;

                return ((_RootType*)this)->QueryInterfaceImpl(_riid, _ppvObject);
            }

            virtual ULONG STDMETHODCALLTYPE AddRef() override
            {
                return Sync::Increment(&uRef);
            }

            virtual ULONG STDMETHODCALLTYPE Release() override
            {
                auto _uOldRef = Sync::Decrement(&uRef);
                if (_uOldRef == 0)
                {
                    delete (_RootType*)this;
                }

                return _uOldRef;
            }
        };
    }
} // namespace YY
