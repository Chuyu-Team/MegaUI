#include "pch.h"
#include <Media/Graphics/DrawContext.h>

#ifdef _WIN32
#include <Media/Graphics/D2D/D2D1_1DrawContext.h>
#include <Media/Graphics/D2D/D2D1_0DrawContext.h>
#include <Media/Graphics/GDIPlus/GDIPlusDrawContext.h>
#endif

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            DrawContextFactory* __YYAPI DrawContextFactory::GetDefaultDrawContextFactory()
            {
#ifdef _WIN32
                if (auto _pDrawContextFactory = D2D1_1DrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;

                if (auto _pDrawContextFactory = D2D1_0DrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;

                if (auto _pDrawContextFactory = GDIPlusDrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;
#endif
                return nullptr;
            }

#ifdef _WIN32
            DrawContextFactory* __YYAPI DrawContextFactory::GetGdiPlusDrawContextFactory()
            {
                return GDIPlusDrawContext::GetDrawContextFactory();
            }

            DrawContextFactory* __YYAPI DrawContextFactory::GetD2D1_0DrawContextFactory()
            {
                return D2D1_0DrawContext::GetDrawContextFactory();
            }

            DrawContextFactory* __YYAPI DrawContextFactory::GetD2D1_1DrawContextFactory()
            {
                return D2D1_1DrawContext::GetDrawContextFactory();
            }
#endif
        } // namespace Graphics
    } // namespace Media
} // namespace YY
