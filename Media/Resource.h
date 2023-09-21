#pragma once
#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        class Resource
        {
        public:
            struct ResourceMetadata
            {
                const ResourceMetadata* pBaseMetadata;
                // 类的继承深度
                uint32_t uDeep;
            };

            class SharedData
            {
            private:
                uint32_t uRef;
                const ResourceMetadata* pMetadata;

            public:
                SharedData(const ResourceMetadata* _pMetadata)
                    : uRef(1)
                    , pMetadata(_pMetadata)
                {
                }

                virtual ~SharedData()
                {
                }

                uint32_t __YYAPI AddRef()
                {
                    return Sync::Increment(&uRef);
                }

                uint32_t __YYAPI Release()
                {
                    auto _uOldRef = Sync::Decrement(&uRef);
                    if (_uOldRef == 0)
                    {
                        delete this;
                    }

                    return _uOldRef;
                }

                bool __YYAPI IsShared() const
                {
                    return uRef > 1;
                }

                const ResourceMetadata* __YYAPI GetResourceMetadata() const
                {
                    return pMetadata;
                }
            };
            
        protected:
            SharedData* pSharedData;
            constexpr static uint32_t uStaticDeep = 0;

        public:
            Resource() noexcept
                : pSharedData(nullptr)
            {
            }

            Resource(const Resource& _oOther) noexcept
                : pSharedData(_oOther.Clone())
            {
            }

            Resource(Resource&& _oOther) noexcept
                : pSharedData(_oOther.Detach())
            {
            }
            
            ~Resource() noexcept
            {
                if (pSharedData)
                    pSharedData->Release();
            }

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata() noexcept
            {
                static const ResourceMetadata s_Metadata = {nullptr, uStaticDeep};
                return &s_Metadata;
            }

            const ResourceMetadata* __YYAPI GetResourceMetadata() const noexcept
            {
                return pSharedData ? pSharedData->GetResourceMetadata() : nullptr;
            }

            template<class Type>
            Type TryCast() const noexcept
            {
                Type _Tmp;
                if (auto _pCurrentMetadata = GetResourceMetadata())
                {
                    auto _pTargetMetadata = Type::GetStaticResourceMetadata();
                    for (; _pCurrentMetadata; _pCurrentMetadata = _pCurrentMetadata->pBaseMetadata)
                    {
                        if (_pCurrentMetadata->uDeep < _pTargetMetadata->uDeep)
                            break;

                        if (_pCurrentMetadata->uDeep == _pTargetMetadata->uDeep)
                        {
                            if (_pCurrentMetadata == _pTargetMetadata)
                            {
                                _Tmp.Attach(Clone());
                            }
                            break;
                        }
                    }
                }

                return _Tmp;
            }

            Resource& operator=(std::nullptr_t) noexcept
            {
                if (pSharedData)
                {
                    pSharedData->Release();
                    pSharedData = nullptr;
                }

                return *this;
            }

            Resource& operator=(const Resource& _oOther) noexcept
            {
                if (pSharedData != _oOther.pSharedData)
                {
                    if (pSharedData)
                        pSharedData->Release();
                    pSharedData = _oOther.pSharedData;
                    if (pSharedData)
                        pSharedData->AddRef();
                }

                return *this;
            }

            Resource& operator=(Resource&& _oOther) noexcept
            {
                if (pSharedData != _oOther.pSharedData)
                {
                    Attach(_oOther.Detach());
                }

                return *this;
            }

            bool operator==(std::nullptr_t) const noexcept
            {
                return pSharedData == nullptr;
            }

            bool operator==(const Resource& _oOther) const noexcept
            {
                return pSharedData == _oOther.pSharedData;
            }

            bool operator!=(std::nullptr_t) const noexcept
            {
                return pSharedData != nullptr;
            }

            bool operator!=(const Resource& _oOther) const noexcept
            {
                return pSharedData != _oOther.pSharedData;
            }

        protected:
            void Attach(_In_opt_ SharedData* _pSharedData) noexcept
            {
                if (pSharedData != _pSharedData)
                {
                    if (pSharedData)
                        pSharedData->Release();

                    pSharedData = _pSharedData;
                }
            }

            _Ret_maybenull_ SharedData* __YYAPI Detach() noexcept
            {
                auto _pSharedData = pSharedData;
                pSharedData = nullptr;
                return _pSharedData;
            }

            _Ret_maybenull_ SharedData* __YYAPI Clone() const noexcept
            {
                if (!pSharedData)
                    return nullptr;

                auto _pSharedData = static_cast<SharedData*>(pSharedData);
                _pSharedData->AddRef();
                return _pSharedData;
            }
        };

    }
} // namespace YY

#pragma pack(pop)
