#pragma once
#include <Base/YY.h>
#include <Base/Memory/RefPtr.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        struct ResourceMetadata
        {
            const ResourceMetadata* pBaseMetadata;
            // 类的继承深度
            const uint32_t uDeep;
        };

        class Resource
        {
        protected:
            class SharedData : public RefValue
            {
            public:
                const ResourceMetadata* pMetadata;

                SharedData(const ResourceMetadata* _pMetadata = Resource::GetStaticResourceMetadata())
                    : pMetadata(_pMetadata)
                {
                }        
            };
            
            RefPtr<SharedData> pSharedData;

            constexpr static uint32_t uStaticDeep = 0;

        public:
            Resource(std::nullptr_t)
            {
            }

            Resource() = default;
            Resource(const Resource&) = default;
            Resource(Resource&&) = default;

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata() noexcept
            {
                static const ResourceMetadata s_Metadata = {nullptr, uStaticDeep};
                return &s_Metadata;
            }

            const ResourceMetadata* __YYAPI GetResourceMetadata() const noexcept
            {
                return pSharedData.Get() ? pSharedData.Get()->pMetadata : nullptr;
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
                                _Tmp.pSharedData.Attach(pSharedData.Clone());
                            }
                            break;
                        }
                    }
                }

                return _Tmp;
            }

            Resource& operator=(std::nullptr_t) noexcept
            {
                pSharedData = nullptr;
                return *this;
            }

            Resource& operator=(const Resource& _oOther) noexcept = default;

            Resource& operator=(Resource&& _oOther) noexcept = default;

            bool operator==(const Resource& _oOther) const noexcept = default;
            
            bool operator==(const std::nullptr_t) const noexcept
            {
                return pSharedData == nullptr;
            }
        };

    }
} // namespace YY

#pragma pack(pop)
