#pragma once
#include <Media/Resource.h>
#include <Media/Color.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        class Brush : public Resource
        {
        protected:
            constexpr static uint32_t uStaticDeep = Resource::uStaticDeep + 1;

        public:
            inline Brush()
                : Resource()
            {
            }

            inline Brush(const Brush& _oOther)
                : Resource(_oOther)
            {
            }

            inline Brush& __YYAPI operator=(const Brush& _oOther)
            {
                Resource::operator=(_oOther);
                return *this;
            }

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata()
            {
                static const ResourceMetadata s_Metadata = {Resource::GetStaticResourceMetadata(), uStaticDeep};
                return &s_Metadata;
            }
        };

        class SolidColorBrush : public Brush
        {
        protected:
            constexpr static uint32_t uStaticDeep = Brush::uStaticDeep + 1;

            class SharedData : public Brush::SharedData
            {
            public:
                Color oCloor;

                SharedData(const ResourceMetadata* _pMetadata)
                    : Resource::SharedData(_pMetadata)
                {
                }
            };

            inline SharedData* GetSharedData() const
            {
                return (SharedData*)pSharedData;
            }

        public:
            /// <summary>
            /// SolidColorBrush - The constructor accepts the color of the brush
            /// </summary>
            /// <param name="_oCloor"> The color value. </param>
            SolidColorBrush(Color _oCloor)
            {
                auto _pSharedData = new SharedData(GetStaticResourceMetadata());
                _pSharedData->oCloor = _oCloor;

                pSharedData = _pSharedData;
            }

            SolidColorBrush()
            {
            }

            SolidColorBrush(const SolidColorBrush& _oOther)
                : Brush(_oOther)
            {
            }
            
            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata()
            {
                static const ResourceMetadata s_Metadata = {Brush::GetStaticResourceMetadata(), uStaticDeep};
                return &s_Metadata;
            }

            inline SolidColorBrush& operator=(const SolidColorBrush& _oOther)
            {
                Brush::operator=(_oOther);
                return *this;
            }

            Color __YYAPI GetColor() const
            {
                auto _pSharedData = GetSharedData();
                if (!_pSharedData)
                    abort();

                return _pSharedData->oCloor;
            }
        };
    }
} // namespace YY

#pragma pack(pop)
