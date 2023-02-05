#include "pch.h"
#include "Render.h"
#include "D2D/D2D1.0Render.h"
#include "D2D/D2D1.1Render.h"
#include "GDI+/GdiPlusRender.h"

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        HRESULT __YYAPI CreateRender(HWND _hWnd, Render** _ppRender)
        {
            #if 0
            return GdiPlusRender::CreateRender(_hWnd, _ppRender);
            #else
            return D2D1_1Render::CreateRender(_hWnd, _ppRender);
            #endif
        }
        
        float __YYAPI DevicePixelToPixel(float _iDevicePixel, int32_t _DPI)
        {
            return _iDevicePixel * float(_DPI) / 96.0f;
        }

        float __YYAPI PointToPixel(float _iFontPoint, int32_t _DPI)
        {
            return _iFontPoint * float(_DPI) / 72.0f;
        }

        Rect __YYAPI DevicePixelToPixel(Rect _iDevicePixelRect, int32_t _DPI)
        {
            _iDevicePixelRect.Left = DevicePixelToPixel(_iDevicePixelRect.Left, _DPI);
            _iDevicePixelRect.Top = DevicePixelToPixel(_iDevicePixelRect.Top, _DPI);
            _iDevicePixelRect.Right = DevicePixelToPixel(_iDevicePixelRect.Right, _DPI);
            _iDevicePixelRect.Bottom = DevicePixelToPixel(_iDevicePixelRect.Bottom, _DPI);

            return _iDevicePixelRect;
        }

        Size __YYAPI DevicePixelToPixel(Size _iDevicePixelSize, int32_t _DPI)
        {
            _iDevicePixelSize.Width = DevicePixelToPixel(_iDevicePixelSize.Width, _DPI);
            _iDevicePixelSize.Height = DevicePixelToPixel(_iDevicePixelSize.Height, _DPI);

            return _iDevicePixelSize;
        }

        float __YYAPI UpdatePixel(float _iOldPixel, int32_t _OldDPI, int32_t _NewDPI)
        {
            if (_OldDPI == 0)
                _OldDPI = 96;

            if (_NewDPI == 0)
                _NewDPI = 96;

            return (float)(double(_iOldPixel) * double(_NewDPI) / double(_OldDPI));
        }
    } // namespace MegaUI
} // namespace YY
