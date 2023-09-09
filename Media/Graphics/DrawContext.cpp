#include "pch.h"
#include "DrawContext.h"

#include <Media/Graphics/D2D/D2D1_1DrawContext.h>
#include <Media/Graphics/D2D/D2D1_0DrawContext.h>
#include <Media/Graphics/GDIPlus/GDIPlusDrawContext.h>

#pragma warning(disable : 28251)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            DrawContextFactory* __YYAPI DrawContextFactory::GetDefaultDrawContextFactory()
            {
                if (auto _pDrawContextFactory = D2D1_1DrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;

                if (auto _pDrawContextFactory = D2D1_0DrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;

                if (auto _pDrawContextFactory = GDIPlusDrawContext::GetDrawContextFactory())
                    return _pDrawContextFactory;
                return nullptr;
            }
        } // namespace Graphics
    } // namespace Media
} // namespace YY
