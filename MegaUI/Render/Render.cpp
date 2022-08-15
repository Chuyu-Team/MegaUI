#include "pch.h"
#include "Render.h"
#include "D2D/D2DRender.h"
#include "GDI+/GdiPlusRender.h"

namespace YY
{
    namespace MegaUI
    {
        HRESULT __fastcall CreateRender(HWND _hWnd, Render** _ppRender)
        {
            return GdiPlusRender::CreateRender(_hWnd, _ppRender);
            //return D2DRender::CreateRender(_hWnd, _ppRender);
        }
    } // namespace MegaUI
} // namespace YY
