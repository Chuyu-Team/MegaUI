#pragma once
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "Property.h"

#pragma pack(push, __MEGA_UI_PACKING)

// 控件的类信息声明
#define _APPLY_MEGA_UI_STATIC_CONTROL_INFO_EXTERN(_CONTROL_NAME, _BASE_CONTROL, _CONTROL_INFO_TYPE,       \
                                                _DEFAULT_CREATE_FLAGS, _PROPERTY_TABLE)           \
public:                                                                                           \
    friend ControlInfoImp<_CONTROL_NAME>;                                                            \
    struct StaticControlInfo                                                                        \
    {                                                                                             \
        using ControlInfoType = _CONTROL_INFO_TYPE;                                                   \
        using BaseControl = _BASE_CONTROL;                                                          \
        constexpr static uint32_t fDefaultCreate = _DEFAULT_CREATE_FLAGS;                         \
        constexpr static raw_const_astring_t szControlName = #_CONTROL_NAME;                          \
        constexpr static uint32_t uPropsCount = 0 _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_COUNT); \
                                                                                                  \
        ControlInfoType* pControlInfoPtr;                                                             \
                                                                                                  \
        union                                                                                     \
        {                                                                                         \
            struct                                                                                \
            {                                                                                     \
                _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_EXTERN);                                  \
            };                                                                                    \
                                                                                                  \
            const PropertyInfo Props[uPropsCount];                                                \
        };                                                                                        \
    };                                                                                            \
    _CONTROL_NAME(const _CONTROL_NAME&) = delete;                                                     \
    _CONTROL_NAME& operator=(const _CONTROL_NAME&) = delete;                                          \
                                                                                                  \
    static StaticControlInfo g_ControlInfoData;                                                       \
    virtual IControlInfo* __MEGA_UI_API GetControlInfo();                                      \
    static IControlInfo* __MEGA_UI_API GetStaticControlInfo();                                 \
    static HRESULT __MEGA_UI_API Register();                                                      \
    static HRESULT __MEGA_UI_API UnRegister();                                                    \
    static HRESULT __MEGA_UI_API Create(                                                          \
        _In_ uint32_t _fCreate,                                                                   \
        _In_opt_ Element* _pTopLevel,                                                             \
        _Out_opt_ intptr_t* _pCooike,                                                             \
        _Outptr_ _CONTROL_NAME** _ppOut);

// 展开控件类信息
#define _APPLY_MEGA_UI_STATIC_CONTROL_INFO(_CONTROL_NAME, _PROPERTY_TABLE)                                                          \
    _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST);                                                                   \
                                                                                                                                \
    _CONTROL_NAME::StaticControlInfo _CONTROL_NAME::g_ControlInfoData =                                                                 \
        {                                                                                                                       \
            nullptr,                                                                                                            \
            {{_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY)}}};                                                                      \
    IControlInfo* __MEGA_UI_API _CONTROL_NAME::GetControlInfo()                                                                \
    {                                                                                                                           \
        return g_ControlInfoData.pControlInfoPtr;                                                                                   \
    }                                                                                                                           \
    IControlInfo* __MEGA_UI_API _CONTROL_NAME::GetStaticControlInfo()                                                          \
    {                                                                                                                           \
        return g_ControlInfoData.pControlInfoPtr;                                                                                   \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CONTROL_NAME::Register()                                                                               \
    {                                                                                                                           \
        return _CONTROL_NAME::StaticControlInfo::ControlInfoType::Register();                                                         \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CONTROL_NAME::UnRegister()                                                                             \
    {                                                                                                                           \
        if (!g_ControlInfoData.pControlInfoPtr)                                                                                     \
            return S_FALSE;                                                                                                     \
        return g_ControlInfoData.pControlInfoPtr->UnRegister();                                                                     \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CONTROL_NAME::Create(uint32_t _fCreate, Element* _pTopLevel, intptr_t* _pCooike, _CONTROL_NAME** _ppOut) \
    {                                                                                                                           \
        if (!_ppOut)                                                                                                            \
            return E_INVALIDARG;                                                                                                \
        *_ppOut = nullptr;                                                                                                      \
                                                                                                                                \
        auto _pElement = HNew<_CONTROL_NAME>();                                                                                   \
        if (!_pElement)                                                                                                         \
            return E_OUTOFMEMORY;                                                                                               \
                                                                                                                                \
        auto _hr = _pElement->Initialize(_fCreate, _pTopLevel, _pCooike);                                                       \
        if (SUCCEEDED(_hr))                                                                                                     \
        {                                                                                                                       \
            *_ppOut = _pElement;                                                                                                \
        }                                                                                                                       \
        else                                                                                                                    \
        {                                                                                                                       \
            HDelete(_pElement);                                                                                                 \
        }                                                                                                                       \
        return _hr;                                                                                                             \
    }

namespace YY
{
    namespace MegaUI
    {
        class IControlInfo
        {
        public:
            virtual uint32_t __MEGA_UI_API AddRef() = 0;
            virtual uint32_t __MEGA_UI_API Release() = 0;

            virtual _Ret_z_ raw_const_astring_t __MEGA_UI_API GetName() = 0;

            virtual _Ret_maybenull_ IControlInfo* __MEGA_UI_API GetBaseControlInfo() = 0;

            /// <summary>
            /// 创建Element，此接口一般给脚本解析器使用。
            /// </summary>
            /// <param name="pTopLevelElem_">顶层Element，如果传入，那么后续将通过pTopLevelElem_的 Defer接口进行推迟更改通知。</param>
            /// <param name="pCookies_">返回 Defer 的Cookies值，如果为 nullptr，那么不会触发Defer</param>
            /// <param name="ppElem_">返回创建的Element</param>
            /// <returns>如果函数成功，那么 >=0 </returns>
            virtual HRESULT __MEGA_UI_API CreateInstance(_In_opt_ Element* _pTopLevelElem, _Out_opt_ intptr_t* _pCookies, _Outptr_ Element** _ppElem_) = 0;

            virtual _Ret_maybenull_ const PropertyInfo* __MEGA_UI_API EnumPropertyInfo(_In_ uint32_t _uIndex) = 0;

            virtual _Success_(return >= 0) int32_t __MEGA_UI_API GetPropertyInfoIndex(_In_ const PropertyInfo& _Prop) = 0;

            virtual bool __MEGA_UI_API IsValidProperty(_In_ const PropertyInfo& _Prop) = 0;

            virtual bool __MEGA_UI_API IsSubclassOf(_In_ IControlInfo* _pControlInfo) = 0;

            virtual HRESULT __MEGA_UI_API UnRegister() = 0;

        protected:
            /// <summary>
            /// 将这个类注册的全局，重复调用将增加引用计数。_UnregisterClass 会减少引用计数。
            /// </summary>
            /// <param name="bExplicitRegister_">如果为 true，那么说明它被调用者显式的注册。</param>
            /// <returns></returns>
            HRESULT __MEGA_UI_API RegisterControlInternal(bool _bExplicitRegister);
            HRESULT __MEGA_UI_API UnregisterControlInternal(bool _bExplicitRegister);
        };

        template<typename _Class>
        class ControlInfoImp;

        _Ret_maybenull_ IControlInfo* __MEGA_UI_API GetRegisterControlInfo(_In_z_ raw_const_astring_t _szControlName);

        HRESULT __MEGA_UI_API UnRegisterAllControls();


    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
