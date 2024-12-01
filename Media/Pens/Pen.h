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

                SharedData(Brush _oBrush = Brush(), float _iThickness = 1.0f, const ResourceMetadata* _pMetadata = Pen::GetStaticResourceMetadata())
                    : Resource::SharedData(_pMetadata)
                    , oBrush(std::move(_oBrush))
                    , iThickness(_iThickness)
                {
                }
            };
            
            inline SharedData* GetSharedData() const
            {
                return (SharedData*)pSharedData.Get();
            }

        public:
            Pen(Brush _oBrush, float _iThickness = 1.0f)
            {
                auto _pSharedData = new SharedData(std::move(_oBrush), _iThickness);
                pSharedData.Attach(_pSharedData);
            }
            
            Pen(std::nullptr_t)
            {
            }
            Pen() = default;
            Pen(const Pen& _oOther) = default;
            Pen(Pen&& _oOther) = default;
            inline Pen& operator=(const Pen& _oOther) = default;
            inline Pen& operator=(Pen&& _oOther) = default;

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
        };
    }
} // namespace YY
#pragma pack(pop)
