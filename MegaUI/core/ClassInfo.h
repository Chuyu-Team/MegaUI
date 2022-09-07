#pragma once
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "Property.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class IClassInfo
        {
        public:
            virtual uint32_t __fastcall AddRef() = 0;
            virtual uint32_t __fastcall Release() = 0;

            virtual _Ret_z_ raw_const_astring_t __fastcall GetName() = 0;

            virtual _Ret_maybenull_ IClassInfo* __fastcall GetBaseClass() = 0;

            /// <summary>
            /// 创建Element，此接口一般给脚本解析器使用。
            /// </summary>
            /// <param name="pTopLevelElem_">顶层Element，如果传入，那么后续将通过pTopLevelElem_的 Defer接口进行推迟更改通知。</param>
            /// <param name="pCookies_">返回 Defer 的Cookies值，如果为 nullptr，那么不会触发Defer</param>
            /// <param name="ppElem_">返回创建的Element</param>
            /// <returns>如果函数成功，那么 >=0 </returns>
            virtual HRESULT __fastcall CreateInstance(_In_opt_ Element* _pTopLevelElem, _Out_opt_ intptr_t* _pCookies, _Outptr_ Element** _ppElem_) = 0;

            virtual _Ret_maybenull_ const PropertyInfo* __fastcall EnumPropertyInfo(_In_ uint32_t _uIndex) = 0;

            virtual _Success_(return >= 0) int32_t __fastcall GetPropertyInfoIndex(_In_ const PropertyInfo& _Prop) = 0;

            virtual bool __fastcall IsValidProperty(_In_ const PropertyInfo& _Prop) = 0;

            virtual bool __fastcall IsSubclassOf(_In_ IClassInfo* _pClassInfo) = 0;

            virtual HRESULT __fastcall UnRegister() = 0;

        protected:
            /// <summary>
            /// 将这个类注册的全局，重复调用将增加引用计数。_UnregisterClass 会减少引用计数。
            /// </summary>
            /// <param name="bExplicitRegister_">如果为 true，那么说明它被调用者显式的注册。</param>
            /// <returns></returns>
            HRESULT __fastcall RegisterClassInternal(bool _bExplicitRegister);
            HRESULT __fastcall UnregisterClassInternal(bool _bExplicitRegister);
        };

        template<typename _Class>
        class ClassInfoBase;

        _Ret_maybenull_ IClassInfo* __fastcall GetRegisterControlClassInfo(_In_z_ raw_const_astring_t _szClassName);

        HRESULT __fastcall UnRegisterAllControls();


    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
