#include "pch.h"
#include "Render.h"
#include "D2D/D2D1.0Render.h"
#include "D2D/D2D1.1Render.h"
#include "GDI+/GdiPlusRender.h"

namespace YY
{
    namespace MegaUI
    {
        HRESULT __MEGA_UI_API CreateRender(HWND _hWnd, Render** _ppRender)
        {
            //return GdiPlusRender::CreateRender(_hWnd, _ppRender);
            return D2D1_1Render::CreateRender(_hWnd, _ppRender);
        }
    } // namespace MegaUI
} // namespace YY
