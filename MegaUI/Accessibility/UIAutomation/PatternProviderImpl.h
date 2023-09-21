#pragma once
#include <combaseapi.h>
#include <UIAutomationCore.h>

#include <MegaUI/base/ComUnknowImpl.h>
#include <Base/Threading/TaskRunner.h>
#include <MegaUI/Accessibility/UIAutomation/ElementAccessibleProviderImpl.h>
#include <Base/Memory/RefPtr.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace MegaUI
    {
        template<typename _ElementType, typename _ProviderType>
        class PatternProvider;
        
        template<typename _RootType, typename _ElementType, typename _ProviderType>
        class PatternProviderBase
            : public ComUnknowImpl<_RootType, _ProviderType>
        {
        protected:
            _ElementType* pElement;
            RefPtr<ThreadTaskRunner> pTaskRunner;

        public:
            PatternProviderBase(_In_ ElementAccessibleProvider* _pProvider)
                : pElement((_ElementType*)_pProvider->GetElement())
                , pTaskRunner(_pProvider->GetTaskRunner())
            {
            }

            static HRESULT __YYAPI Create(_In_ ElementAccessibleProvider* _pProvider, _COM_Outptr_ IUnknown** _ppPattern)
            {
                if (!_ppPattern)
                    return E_INVALIDARG;
                *_ppPattern = nullptr;

                if (!_RootType::IsPatternSupported(_pProvider->GetElement()))
                    return E_NOINTERFACE;

                auto _pPattern = new (std::nothrow) _RootType(_pProvider);
                if (!_pPattern)
                    return E_OUTOFMEMORY;

                *_ppPattern = static_cast<IUnknown*>(_pPattern);
                return S_OK;
            }

            /// <summary>
            /// 优先使用缓存，如果不存在则创建并同步到缓存。
            /// </summary>
            /// <param name="_pProvider"></param>
            /// <param name="_ppPattern"></param>
            /// <param name="_ppPatternCache"></param>
            /// <returns></returns>
            static HRESULT __YYAPI Create(_In_ ElementAccessibleProvider* _pProvider, _COM_Outptr_ IUnknown** _ppPattern, _Inout_ IUnknown** _ppPatternCache)
            {
                if (!_ppPattern)
                    return E_INVALIDARG;
                *_ppPattern = nullptr;

                auto _pPattern = *_ppPatternCache;
                if (!_pPattern)
                {
                    auto _hr = Create(_pProvider, &_pPattern);
                    if (FAILED(_hr))
                        return _hr;

                    if (!_pPattern)
                        return E_UNEXPECTED;

                    *_ppPatternCache = _pPattern;
                }

                _pPattern->AddRef();
                *_ppPattern = _pPattern;
                return S_OK;
            }
        };

    }
} // namespace YY

#pragma pack(pop)
