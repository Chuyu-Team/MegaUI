#pragma once
#include <stdlib.h>

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
            inline Brush() noexcept = default;

            inline Brush(std::nullptr_t) noexcept
            {
            }

            inline Brush(const Brush& _oOther) noexcept = default;

            inline Brush(Brush&& _oOther) noexcept = default;

            inline Brush& __YYAPI operator=(const Brush& _oOther) = default;
            inline Brush& __YYAPI operator=(Brush&& _oOther) = default;

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata() noexcept
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

                SharedData(Color _oCloor, const ResourceMetadata* _pMetadata = SolidColorBrush::GetStaticResourceMetadata())
                    : Brush::SharedData(_pMetadata)
                    , oCloor(_oCloor)
                {
                }
            };

            inline SharedData* GetSharedData() const
            {
                return (SharedData*)pSharedData.Get();
            }
            
        public:
            /// <summary>
            /// SolidColorBrush - The constructor accepts the color of the brush
            /// </summary>
            /// <param name="_oCloor"> The color value. </param>
            SolidColorBrush(Color _oCloor)
            {
                auto _pSharedData = new SharedData(_oCloor, GetStaticResourceMetadata());
                pSharedData.Attach(_pSharedData);
            }

            SolidColorBrush(std::nullptr_t)
            {
            }

            SolidColorBrush() = default;
            SolidColorBrush(const SolidColorBrush& _oOther) = default;
            SolidColorBrush(SolidColorBrush&& _oOther) = default;
            inline SolidColorBrush& operator=(const SolidColorBrush& _oOther) = default;
            inline SolidColorBrush& operator=(SolidColorBrush&& _oOther) = default;

//#if defined(_HAS_CXX20) && _HAS_CXX20
//            bool operator==(const SolidColorBrush&) const = default;
//#else
//            bool operator==(const SolidColorBrush& _oOther) const
//            {
//                return Brush::operator==(_oOther);
//            }
//#endif

            bool operator==(std::nullptr_t _null) const
            {
                return pSharedData.Get() == nullptr;
            }

            static const ResourceMetadata* __YYAPI GetStaticResourceMetadata()
            {
                static const ResourceMetadata s_Metadata = {Brush::GetStaticResourceMetadata(), uStaticDeep};
                return &s_Metadata;
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
