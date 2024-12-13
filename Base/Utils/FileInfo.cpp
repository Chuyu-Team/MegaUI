#include <Base/Utils/FileInfo.h>

#include <memory>
#include <Base/ErrorCode.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
            HRESULT __YYAPI GetFileVersion(LPCWSTR _szFilePath, Version* _pVersion) noexcept
            {
				_pVersion->uInternalValue = 0;
                auto _hFileMoudle = LoadLibraryExW(_szFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
                if (!_hFileMoudle)
                    return HRESULT_From_LSTATUS(GetLastError());

                auto _lStatus = GetFileVersion(_hFileMoudle, _pVersion);
                FreeLibrary(_hFileMoudle);
                return _lStatus;
            }

			HRESULT __YYAPI GetFileVersion(HMODULE _hMoudle, Version* _pVersion) noexcept
			{
				_pVersion->uInternalValue = 0;
				__try
				{
					HRSRC _hRsrcVersion = FindResourceExW(_hMoudle, RT_VERSION, MAKEINTRESOURCE(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

					if (_hRsrcVersion == NULL)
					{
						return HRESULT_From_LSTATUS(GetLastError());
					}

					HGLOBAL _hGlobal = LoadResource(_hMoudle, _hRsrcVersion);
					if (_hGlobal == NULL)
					{
						return HRESULT_From_LSTATUS(GetLastError());
					}

					VS_FIXEDFILEINFO* _pFileInfo = NULL;
					UINT _cbFileInfo;
#ifndef _ATL_XP_TARGETING
					if (!VerQueryValueW(_hGlobal, L"\\", (LPVOID*)&_pFileInfo, &_cbFileInfo))
					{
						return HRESULT_From_LSTATUS(GetLastError());
					}
#else
					// XP系统不允许直接调用，不然会触发内存非法访问。所以先复制到一个内存块上
					const DWORD _cbFileInfoBuffer = SizeofResource(_hMoudle, _hRsrcVersion);
					void* _pFileInfoBuffer = alloca(_cbFileInfoBuffer);

					memcpy(_pFileInfoBuffer, _hGlobal, _cbFileInfoBuffer);

					if (!VerQueryValueW(_pFileInfoBuffer, L"\\", (LPVOID*)&_pFileInfo, &_cbFileInfo))
					{
						return HRESULT_From_LSTATUS(GetLastError());
					}
#endif

					_pVersion->uLowPart = _pFileInfo->dwFileVersionLS;
					_pVersion->uHightPart = _pFileInfo->dwFileVersionMS;
					return S_OK;
				}
				//抓取EXCEPTION_IN_PAGE_ERROR异常，防止IO问题导致程序崩溃。
				__except ((GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR || GetExceptionCode() == 0xC000009C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
				{
					return HRESULT_From_LSTATUS(ERROR_DISK_OPERATION_FAILED);
				}
			}
        }
    }
}
