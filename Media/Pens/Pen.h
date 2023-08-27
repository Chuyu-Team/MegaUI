#pragma once
#include <Media/Brushes/Brush.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        class Pen : public Resource
        {
        protected:
            constexpr static uint32_t uStaticDeep = Resource::uStaticDeep + 1;

            class SharedData : public Resource::SharedData
            {
            public:
                Brush oBrush;
                float iThickness;

                SharedData(const ResourceMetadata* _pMetadata)
                    : Resource::SharedData(_pMetadata)
                    , oBrush()
                    , iThickness(1.0f)
                {
                }
            };
            
            inline SharedData* GetSharedData() const
            {
                return (SharedData*)pSharedData;
            }

        public:
            Pen(Brush _oBrush, float _iThickness = 1.0f)
            {
                auto _pSharedData = new SharedData(GetStaticResourceMetadata());
                _pSharedData->oBrush = _oBrush;
                _pSharedData->iThickness = _iThickness;

                Attach(_pSharedData);
            }

            Pen(std::nullptr_t)
            {
            }
            
            Pen(const Pen& _oOther)
                : Resource(_oOther)
            {
            }

            Pen(Pen&& _oOther)
                : Resource(std::move(_oOther))
            {
            }

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata()
            {
                static const ResourceMetadata s_Metadata = {Resource::GetStaticResourceMetadata(), uStaticDeep};
                return &s_Metadata;
            }

            Brush __YYAPI GetBrush() const
            {
                auto _pSharedData = GetSharedData();
                if (!_pSharedData)
                    abort();

                return _pSharedData->oBrush;
            }

            float __YYAPI GetThickness() const
            {
                auto _pSharedData = GetSharedData();
                if (!_pSharedData)
                    abort();

                return _pSharedData->iThickness;
            }

            bool __YYAPI operator==(std::nullptr_t) const
            {
                return pSharedData == nullptr;
            }

            bool __YYAPI operator!=(std::nullptr_t) const
            {
                return pSharedData != nullptr;
            }
        };
    }
} // namespace YY
#pragma pack(pop)
