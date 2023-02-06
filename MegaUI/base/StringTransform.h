#pragma once
#include <Base/String/String.h>

namespace YY
{
    namespace MegaUI
    {
        // To ANSI
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ aString* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ aString* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ aString* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ aString* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ aString* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ aString* _pszDst);

        HRESULT __YYAPI Transform(_In_ const aString& _szSrc, _Inout_ aString* _pszDst);

        // To UTF8
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ u8String* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ u8String* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ u8String* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ u8String* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ u8String* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ u8String* _pszDst);

        HRESULT __YYAPI Transform(_In_ const u8String& _szSrc, _Inout_ u8String* _pszDst);

        // To UTF16LE
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ u16StringLE* _pszDst);

        HRESULT __YYAPI Transform(_In_ const u16StringLE& _szSrc, _Inout_ u16StringLE* _pszDst);
        HRESULT __YYAPI Transform(_In_ u16StringBE&& _szSrc, _Inout_ u16StringLE* _pszDst);

        // To UTF16BE
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ u16StringBE* pszDst);

        HRESULT __YYAPI Transform(_In_ u16StringLE&& _szSrc, _Inout_ u16StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBE& _szSrc, _Inout_ u16StringBE* pszDst);

        // To UTF32LE
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ u32StringLE* pszDst);

        HRESULT __YYAPI Transform(_In_ const u32StringLE& _szSrc, _Inout_ u32StringLE* pszDst);
        HRESULT __YYAPI Transform(_In_ u32StringBE&& _szSrc, _Inout_ u32StringLE* pszDst);

        // To UFT32BE
        HRESULT __YYAPI Transform(_In_ const aStringView& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u8StringView& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringLEView& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u16StringBEView& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringLEView& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBEView& _szSrc, _Inout_ u32StringBE* pszDst);

        HRESULT __YYAPI Transform(_In_ u32StringLE&& _szSrc, _Inout_ u32StringBE* pszDst);
        HRESULT __YYAPI Transform(_In_ const u32StringBE& _szSrc, _Inout_ u32StringBE* pszDst);
    } // namespace MegaUI
} // namespace YY