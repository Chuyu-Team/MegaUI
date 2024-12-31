#pragma once
#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
            template <typename ...Types>
            struct ComObjectQuery;

            template <>
            struct ComObjectQuery<>
            {
            public:
                _Ret_maybenull_ void* __YYAPI Cast(_In_ REFIID _riid) noexcept
                {
                    return nullptr;
                }
            };

            template <typename FirstBaseInterface, typename ...OtherBaseInterfaces>
            struct ComObjectQuery<FirstBaseInterface, OtherBaseInterfaces...>
                : public FirstBaseInterface
                , public ComObjectQuery<OtherBaseInterfaces...>
            {
            public:
                _Ret_maybenull_ void* __YYAPI Cast(_In_ REFIID _riid) noexcept
                {
                    if (_riid == __uuidof(FirstBaseInterface))
                    {
                        return static_cast<FirstBaseInterface*>(this);
                    }

                    return ComObjectQuery<OtherBaseInterfaces...>::Cast(_riid);
                }
            };

            /// <summary>
            /// 为DerivedInterface 基类接口提供 QueryInterface 支持。
            /// </summary>
            /// <typeparam name="DerivedInterface"></typeparam>
            /// <typeparam name="...InheritChainInterfaces"></typeparam>
            template<typename DerivedInterface, typename... InheritChainInterfaces>
            class ComObjectInheritChain : public DerivedInterface
            {
            private:
                template<typename... Types>
                struct QueryChainImpl;

                template<typename FirstInheritChainInterface, typename ...OtherInheritChainInterfaces>
                struct QueryChainImpl<FirstInheritChainInterface, OtherInheritChainInterfaces...>
                {
                    static _Ret_maybenull_ void* __YYAPI CastImpl(DerivedInterface* _pThis, _In_ REFIID _riid) noexcept
                    {
                        if (_riid == __uuidof(FirstInheritChainInterface))
                        {
                            return static_cast<FirstInheritChainInterface*>(_pThis);
                        }

                        return QueryChainImpl<OtherInheritChainInterfaces...>::CastImpl(_pThis, _riid);
                    }
                };

                template<>
                struct QueryChainImpl<>
                {
                    static _Ret_maybenull_ void* __YYAPI CastImpl(DerivedInterface* _pThis, _In_ REFIID _riid) noexcept
                    {
                        return nullptr;
                    }
                };

            public:
                _Ret_maybenull_ void* __YYAPI Cast(_In_ REFIID _riid) noexcept
                {
                    if (_riid == __uuidof(DerivedInterface))
                    {
                        return static_cast<DerivedInterface*>(this);
                    }

                    return QueryChainImpl<InheritChainInterfaces...>::CastImpl(static_cast<DerivedInterface*>(this), _riid);
                }
            };

            template <typename DerivedInterface, typename ...InheritChainInterfaces, typename ...BaseInterfaces>
            struct ComObjectQuery<ComObjectInheritChain<DerivedInterface, InheritChainInterfaces...>, BaseInterfaces...>
                : public ComObjectInheritChain<DerivedInterface, InheritChainInterfaces...>
                , public ComObjectQuery<BaseInterfaces...>
            {
                _Ret_maybenull_ void* __YYAPI Cast(_In_ REFIID _riid) noexcept
                {
                    if (auto pObject = ComObjectInheritChain<DerivedInterface, InheritChainInterfaces...>::Cast(_riid))
                    {
                        return pObject;
                    }

                    return ComObjectQuery<BaseInterfaces...>::Cast(_riid);
                }
            };

            /// <summary>
            /// 简化Com接口实现而存在。示例：<para/>
            /// <para/>
            /// 1. 为实现 TestComObject实现 ITextProvider接口，其中 QueryInterface可以请求 ITextProvider以及IUnknown，则可以编写如下代码：<para/>
            /// * IUnknown为隐含实现，无需指定。<para/>
            /// class TestComObject: public YY::MegaUI::ComObjectImpl&lt;TestComObject, ITextProvider>;<para/>
            /// <para/>
            /// 2. 为实现 TestComObject实现 ITextProvider、ITextRangeProvider接口，其中 QueryInterface可以请求 ITextProvider、ITextRangeProvider以及IUnknown，则可以编写如下代码：<para/>
            /// class TestComObject: public YY::MegaUI::ComObjectImpl&lt;TestComObject, ITextProvider, ITextRangeProvider>;<para/>
            /// <para/>
            /// 3. 为实现 TestComObject实现 ITextProvider2、ITextRangeProvider接口，其中 QueryInterface可以请求 ITextProvider2、ITextProvider、ITextRangeProvider以及IUnknown，则可以编写如下代码：<para/>
            /// * 这里需要使用YY::ComObjectInheritChain，来描述 ITextProvider2的基类。<para/>
            /// class TestComObject: public YY::MegaUI::ComObjectImpl&lt;TestComObject, YY::ComObjectInheritChain&lt;ITextProvider2, ITextProvider>, ITextRangeProvider>;<para/>
            /// 
            /// ComObjectImpl 从下面二个接口演变而来：
            /// * https://github.com/ProjectMile/Mile.Windows.Helpers/blob/main/Mile.Helpers/Mile.Helpers.CppBase.h ComObject类
            /// * https://github.com/microsoft/msix-packaging/blob/master/src/inc/shared/ComHelper.hpp ComClass类
            /// </summary>
            /// <typeparam name="DerivedClass">Com接口的实现基类名称</typeparam>
            /// <typeparam name="...BaseInterfaces">Com接口需要继承的接口。</typeparam>
            template<typename DerivedClass, typename... BaseInterfaces>
            class ComObjectImpl : public ComObjectQuery<BaseInterfaces...>
            {
            private:
                uint32_t uRef = 1u;

            public:
                //////////////////////////////////////////////////////////////
                // IUnknown
                HRESULT STDMETHODCALLTYPE QueryInterface(
                    /* [in] */ REFIID _riid,
                    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppObject) override
                {
                    if (!_ppObject)
                        return E_INVALIDARG;

                    *_ppObject = nullptr;
                    return static_cast<DerivedClass*>(this)->QueryInterfaceImpl(_riid, _ppObject);
                }

                ULONG STDMETHODCALLTYPE AddRef() override
                {
                    return Sync::Increment(&uRef);
                }

                ULONG STDMETHODCALLTYPE Release() override
                {
                    auto _uOldRef = Sync::Decrement(&uRef);
                    if (_uOldRef == 0)
                    {
                        delete static_cast<DerivedClass*>(this);
                    }

                    return _uOldRef;
                }

            protected:
                inline HRESULT STDMETHODCALLTYPE QueryInterfaceImpl(
                    /* [in] */ REFIID _riid,
                    /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* _ppObject)
                {
                    *_ppObject = nullptr;
                    auto _pObject = ComObjectQuery<BaseInterfaces...>::Cast(_riid);
                    if (!_pObject)
                    {
                        static_assert(std::is_base_of<IUnknown, ComObjectImpl>::value, "Com接口必须有一个 IUnknown基类。");
                        if (_riid != __uuidof(IUnknown))
                        {
                            return E_NOINTERFACE;
                        }

                        // 为 IUnknown做保底，因为Com接口始终继承自 IUnknown，始终手工为 QueryInterface 编写 IUnknown这是非常无聊的。
                        _pObject = reinterpret_cast<IUnknown*>(this);
                    }

                    *_ppObject = _pObject;
                    AddRef();
                    return S_OK;
                }
            };
        } // namespace Utils
    } // namespace Base

    using namespace YY::Base::Utils;
} // namespace YY

#pragma pack(pop)
