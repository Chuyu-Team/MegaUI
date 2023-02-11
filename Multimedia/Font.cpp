#include "pch.h"
#include <Multimedia/Font.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION();

namespace YY
{
    namespace Multimedia
    {
        HRESULT __YYAPI GetSystemFont(SystemFont _eSystemFont, Font* _pFont)
        {
            __YY_IGNORE_UNINITIALIZED_VARIABLE(_pFont);

            _pFont->szFace.Clear();
            _pFont->iSize = 0;
            _pFont->uWeight = 0;
            _pFont->fStyle = FontStyle::None;

            if (_eSystemFont >= SystemFont::SystemFontCount)
                return E_NOINTERFACE;

            struct SystemFontInfo
            {
                UINT uiAction;
                uint16_t uOffsetToData;
                uint16_t uSizeOfBuffer;
            };

            union
            {
                NONCLIENTMETRICSW NonClient;
                ICONMETRICSW IconMetrics;
                uint8_t Buffer[1];
            } FontInfoBuffer;

            static const SystemFontInfo SystemFontIndexInfoArray[] =
            {
                {SPI_GETNONCLIENTMETRICS, UFIELD_OFFSET(NONCLIENTMETRICSW, lfCaptionFont), sizeof(NONCLIENTMETRICSW)},
                {SPI_GETNONCLIENTMETRICS, UFIELD_OFFSET(NONCLIENTMETRICSW, lfMenuFont), sizeof(NONCLIENTMETRICSW)},
                {SPI_GETNONCLIENTMETRICS, UFIELD_OFFSET(NONCLIENTMETRICSW, lfMessageFont), sizeof(NONCLIENTMETRICSW)},
                {SPI_GETNONCLIENTMETRICS, UFIELD_OFFSET(NONCLIENTMETRICSW, lfSmCaptionFont), sizeof(NONCLIENTMETRICSW)},
                {SPI_GETNONCLIENTMETRICS, UFIELD_OFFSET(NONCLIENTMETRICSW, lfStatusFont), sizeof(NONCLIENTMETRICSW)},
                {SPI_GETICONMETRICS, UFIELD_OFFSET(ICONMETRICSW, lfFont), sizeof(ICONMETRICSW)},
            };

            auto& SystemFontIndexInfo = SystemFontIndexInfoArray[(uint32_t)_eSystemFont];
            FontInfoBuffer.NonClient.cbSize = SystemFontIndexInfo.uSizeOfBuffer;

            if (!SystemParametersInfoW(SystemFontIndexInfo.uiAction, SystemFontIndexInfo.uSizeOfBuffer, &FontInfoBuffer, 0))
            {
                return __HRESULT_FROM_WIN32(GetLastError());
            }
            
            auto& Font = *(LOGFONTW*)(FontInfoBuffer.Buffer + SystemFontIndexInfo.uOffsetToData);
            _pFont->szFace = Font.lfFaceName;
            _pFont->iSize = (float)Font.lfHeight;
            _pFont->uWeight = Font.lfWeight;

            if (Font.lfItalic)
                _pFont->fStyle |= FontStyle::Italic;

            if (Font.lfUnderline)
                _pFont->fStyle |= FontStyle::Underline;

            if (Font.lfStrikeOut)
                _pFont->fStyle |= FontStyle::StrikeOut;

            return S_OK;
        }
    } // namespace Multimedia
} // namespace YY
