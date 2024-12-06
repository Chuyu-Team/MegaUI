#pragma once
#include <Base/Utils/Version.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
    {
        namespace Utils
        {
            /// <summary>
            /// 从文件中VS_FIXEDFILEINFO信息中获取文件版本号。
            /// 
            /// 温馨提示：该函数不受Windows兼容性影响。
            /// </summary>
            /// <param name="_szFilePath">需要获取版本号的可执行文件路径。</param>
            /// <param name="_pVersion"></param>
            /// <returns></returns>
            HRESULT __YYAPI GetFileVersion(_In_z_ LPCWSTR _szFilePath, _Out_ Version* _pVersion) noexcept;

            /// <summary>
            /// 从模块VS_FIXEDFILEINFO信息中获取文件版本号。
            /// 
            /// 温馨提示：该函数不受Windows兼容性影响。
            /// </summary>
            /// <param name="_hMoudle">需要获取版本号的模块句柄，以数据属性加载即可。如果参数为 nullptr，则代表获取当前可执行模块的版本号。</param>
            /// <param name="_pVersion"></param>
            /// <returns></returns>
            HRESULT __YYAPI GetFileVersion(_In_opt_ HMODULE _hMoudle, _Out_ Version* _pVersion) noexcept;
        }
    }
}
#pragma pack(pop)
