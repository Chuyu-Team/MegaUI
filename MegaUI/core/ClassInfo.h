#pragma once
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"
#include "Property.h"

#pragma pack(push, __MEGA_UI_PACKING)

// 控件的类信息声明
#define _APPLY_MEGA_UI_STATIC_CALSS_INFO_EXTERN(_CLASS_NAME, _BASE_CLASS, _CLASS_INFO_TYPE,       \
                                                _DEFAULT_CREATE_FLAGS, _PROPERTY_TABLE)           \
public:                                                                                           \
    friend ClassInfoBase<_CLASS_NAME>;                                                            \
    struct StaticClassInfo                                                                        \
    {                                                                                             \
        using ClassInfoType = _CLASS_INFO_TYPE;                                                   \
        using BaseElement = _BASE_CLASS;                                                          \
        constexpr static uint32_t fDefaultCreate = _DEFAULT_CREATE_FLAGS;                         \
        constexpr static raw_const_astring_t szClassName = #_CLASS_NAME;                          \
        constexpr static uint32_t uPropsCount = 0 _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_COUNT); \
                                                                                                  \
        ClassInfoType* pClassInfoPtr;                                                             \
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
    _CLASS_NAME(const _CLASS_NAME&) = delete;                                                     \
    _CLASS_NAME& operator=(const _CLASS_NAME&) = delete;                                          \
                                                                                                  \
    static StaticClassInfo g_ClassInfoData;                                                       \
    virtual IClassInfo* __MEGA_UI_API GetControlClassInfo();                                      \
    static IClassInfo* __MEGA_UI_API GetStaticControlClassInfo();                                 \
    static HRESULT __MEGA_UI_API Register();                                                      \
    static HRESULT __MEGA_UI_API UnRegister();                                                    \
    static HRESULT __MEGA_UI_API Create(                                                          \
        _In_ uint32_t _fCreate,                                                                   \
        _In_opt_ Element* _pTopLevel,                                                             \
        _Out_opt_ intptr_t* _pCooike,                                                             \
        _Outptr_ _CLASS_NAME** _ppOut);

// 展开控件类信息
#define _APPLY_MEGA_UI_STATIC_CALSS_INFO(_CLASS_NAME, _PROPERTY_TABLE)                                                          \
    _PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY_VALUE_TYPE_LIST);                                                                   \
                                                                                                                                \
    _CLASS_NAME::StaticClassInfo _CLASS_NAME::g_ClassInfoData =                                                                 \
        {                                                                                                                       \
            nullptr,                                                                                                            \
            {{_PROPERTY_TABLE(_APPLY_MEGA_UI_PROPERTY)}}};                                                                      \
    IClassInfo* __MEGA_UI_API _CLASS_NAME::GetControlClassInfo()                                                                \
    {                                                                                                                           \
        return g_ClassInfoData.pClassInfoPtr;                                                                                   \
    }                                                                                                                           \
    IClassInfo* __MEGA_UI_API _CLASS_NAME::GetStaticControlClassInfo()                                                          \
    {                                                                                                                           \
        return g_ClassInfoData.pClassInfoPtr;                                                                                   \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CLASS_NAME::Register()                                                                               \
    {                                                                                                                           \
        return _CLASS_NAME::StaticClassInfo::ClassInfoType::Register();                                                         \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CLASS_NAME::UnRegister()                                                                             \
    {                                                                                                                           \
        if (!g_ClassInfoData.pClassInfoPtr)                                                                                     \
            return S_FALSE;                                                                                                     \
        return g_ClassInfoData.pClassInfoPtr->UnRegister();                                                                     \
    }                                                                                                                           \
    HRESULT __MEGA_UI_API _CLASS_NAME::Create(uint32_t _fCreate, Element* _pTopLevel, intptr_t* _pCooike, _CLASS_NAME** _ppOut) \
    {                                                                                                                           \
        if (!_ppOut)                                                                                                            \
            return E_INVALIDARG;                                                                                                \
        *_ppOut = nullptr;                                                                                                      \
                                                                                                                                \
        auto _pElement = HNew<_CLASS_NAME>();                                                                                   \
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
        class IClassInfo
        {
        public:
            virtual uint32_t __MEGA_UI_API AddRef() = 0;
            virtual uint32_t __MEGA_UI_API Release() = 0;

            virtual _Ret_z_ raw_const_astring_t __MEGA_UI_API GetName() = 0;

            virtual _Ret_maybenull_ IClassInfo* __MEGA_UI_API GetBaseClass() = 0;

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

            virtual bool __MEGA_UI_API IsSubclassOf(_In_ IClassInfo* _pClassInfo) = 0;

            virtual HRESULT __MEGA_UI_API UnRegister() = 0;

        protected:
            /// <summary>
            /// 将这个类注册的全局，重复调用将增加引用计数。_UnregisterClass 会减少引用计数。
            /// </summary>
            /// <param name="bExplicitRegister_">如果为 true，那么说明它被调用者显式的注册。</param>
            /// <returns></returns>
            HRESULT __MEGA_UI_API RegisterClassInternal(bool _bExplicitRegister);
            HRESULT __MEGA_UI_API UnregisterClassInternal(bool _bExplicitRegister);
        };

        template<typename _Class>
        class ClassInfoBase;

        _Ret_maybenull_ IClassInfo* __MEGA_UI_API GetRegisterControlClassInfo(_In_z_ raw_const_astring_t _szClassName);

        HRESULT __MEGA_UI_API UnRegisterAllControls();


    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
