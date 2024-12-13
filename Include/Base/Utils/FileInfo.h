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
            /// ���ļ���VS_FIXEDFILEINFO��Ϣ�л�ȡ�ļ��汾�š�
            /// 
            /// ��ܰ��ʾ���ú�������Windows������Ӱ�졣
            /// </summary>
            /// <param name="_szFilePath">��Ҫ��ȡ�汾�ŵĿ�ִ���ļ�·����</param>
            /// <param name="_pVersion"></param>
            /// <returns></returns>
            HRESULT __YYAPI GetFileVersion(_In_z_ LPCWSTR _szFilePath, _Out_ Version* _pVersion) noexcept;

            /// <summary>
            /// ��ģ��VS_FIXEDFILEINFO��Ϣ�л�ȡ�ļ��汾�š�
            /// 
            /// ��ܰ��ʾ���ú�������Windows������Ӱ�졣
            /// </summary>
            /// <param name="_hMoudle">��Ҫ��ȡ�汾�ŵ�ģ���������������Լ��ؼ��ɡ��������Ϊ nullptr��������ȡ��ǰ��ִ��ģ��İ汾�š�</param>
            /// <param name="_pVersion"></param>
            /// <returns></returns>
            HRESULT __YYAPI GetFileVersion(_In_opt_ HMODULE _hMoudle, _Out_ Version* _pVersion) noexcept;
        }
    }
}
#pragma pack(pop)
