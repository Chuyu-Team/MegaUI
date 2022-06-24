#pragma once
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"

#include "..\base\alloc.h"
#include "Property.h"
#include "Element.h"

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

        _Ret_maybenull_ IClassInfo* __fastcall GetRegisterControlClassInfo(_In_z_ raw_const_astring_t _szClassName);

        HRESULT __fastcall UnRegisterAllControls();

        template<typename _Class>
        class ClassInfoBase : public IClassInfo
        {
        private:
            uint32_t uRef;

        public:
            ClassInfoBase()
                : uRef(0)
            {
            }

            virtual ~ClassInfoBase()
            {
            }

            virtual uint32_t __fastcall AddRef() override
            {
                return ++uRef;
            }

            virtual uint32_t __fastcall Release() override
            {
                const auto _uNewRef = --uRef;

                if (_uNewRef == 0)
                {
                    UnregisterClassInternal(false);
                    _Class::ClassInfoData.pClassInfoPtr = nullptr;
                    HDelete(this);
                }

                return _uNewRef;
            }

            virtual raw_const_astring_t __fastcall GetName() override
            {
                return _Class::StaticClassInfo::pszClassInfoName;
            }

            virtual IClassInfo* __fastcall GetBaseClass() override
            {
                return _Class::StaticClassInfo::BaseElement::GetStaticClassInfo();
            }

            virtual HRESULT __fastcall CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                return _Class::Create(_Class::StaticClassInfo::fDefaultCreate, _pTopLevelElem, _pCookies, _ppOutElem);
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return _Class::StaticClassInfo::uPropsCount + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
            }

            static const PropertyInfo* __fastcall EnumPropertyInfoImp(uint32_t _uIndex)
            {
                constexpr auto _uBasePropCount = ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();

                if (_uIndex >= _uBasePropCount)
                {
                    return &_Class::_ClassInfoData._Props[_uIndex - _uBasePropCount];
                }

                return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::EnumPropertyInfoImp();
            }

            virtual const PropertyInfo* __fastcall EnumPropertyInfo(uint32_t _uIndex) override
            {
                if (_uIndex >= GetPropertyInfoCount())
                    return nullptr;

                // EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
                return EnumPropertyInfoImp(_uIndex);
            }

            virtual bool __fastcall IsValidProperty(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndex(_Prop) >= 0;
            }

            virtual bool __fastcall IsSubclassOf(IClassInfo* _pClassInfo) override
            {
                if (!_pClassInfo)
                    return false;

                IClassInfo* _pThis = this;

                do
                {
                    if (_pClassInfo == _pThis)
                        return true;

                    _pThis = GetBaseClass();
                } while (_pThis);

                return false;
            }

            static int32_t __fastcall GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint32_t _uIndex = &_Prop - &_Class::ClassInfoData.Props[0];

                if (_uIndex >= _Class::StaticClassInfo::uPropsCount)
                {
                    return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoIndexImp(_Prop);
                }

                return _uIndex + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
            }

            virtual int32_t __fastcall GetPropertyInfoIndex(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndexImp(_Prop);
            }

            /// <summary>
            /// 将这个类注册到全局
            /// </summary>
            /// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
            /// <returns>HRESULT</returns>
            static HRESULT __fastcall Register(bool _bExplicitRegister = true)
            {
                auto _hr = _Class::StaticClassInfo::BaseElement::ClassInfoType::Register(false);

                if (FAILED(_hr))
                    return _hr;

                auto& _pClassInfo = _Class::ClassInfoData.pClassInfoPtr;
                if (_pClassInfo)
                {
                    _pClassInfo->AddRef();

                    if (_bExplicitRegister)
                    {
                        _hr = _pClassInfo->_RegisterClass(true);

                        if (FAILED(_hr))
                            _pClassInfo->Release();
                    }
                }
                else
                {
                    // 这里不会增加引用计数
                    auto _pTmp = HNewAndZero<_Class::StaticClassInfo::ClassInfoType>();
                    if (!_pTmp)
                        return E_OUTOFMEMORY;

                    _hr = _pTmp->_RegisterClass(_bExplicitRegister);
                    if (FAILED(_hr))
                    {
                        HDelete(_pTmp);
                    }
                    else
                    {
                        _pTmp->AddRef();
                        _pClassInfo = _pTmp;
                    }
                }

                return _hr;
            }

            HRESULT __fastcall UnRegister()
            {
                auto _pClassInfo = _Class::ClassInfoData.pClassInfoPtr;
                if (!_pClassInfo)
                    return S_FALSE;

                auto _hr = _pClassInfo->_UnregisterClass(true);

                if (SUCCEEDED(_hr))
                {
                    IClassInfo* _pThis = this;

                    while (_pThis)
                    {
                        auto _pTmp = _pThis->GetBaseClass();
                        _pThis->Release();
                        _pThis = _pTmp;
                    }
                }

                return _hr;
            }
        };

        template<>
        class ClassInfoBase<Element> : public IClassInfo
        {
        private:
            uint32_t uRef;

        public:
            ClassInfoBase()
                : uRef(0)
            {
            }

            virtual ~ClassInfoBase()
            {
            }

            virtual uint32_t __fastcall AddRef() override
            {
                return ++uRef;
            }

            virtual uint32_t __fastcall Release() override
            {
                const auto _uNewRef = --uRef;

                if (_uNewRef == 0)
                {
                    UnregisterClassInternal(false);
                    Element::g_ClassInfoData.pClassInfoPtr = nullptr;
                    HDelete(this);
                }

                return _uNewRef;
            }

            virtual raw_const_astring_t __fastcall GetName() override
            {
                return Element::StaticClassInfo::szClassName;
            }

            virtual IClassInfo* __fastcall GetBaseClass() override
            {
                return nullptr;
            }

            virtual HRESULT __fastcall CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                return Element::Create(Element::StaticClassInfo::fDefaultCreate, _pTopLevelElem, _pCookies, _ppOutElem);
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return Element::StaticClassInfo::uPropsCount;
            }

            static const PropertyInfo* __fastcall EnumPropertyInfoImp(unsigned int _uIndex)
            {
                return &Element::g_ClassInfoData.Props[_uIndex];
            }

            virtual const PropertyInfo* __fastcall EnumPropertyInfo(unsigned int _uIndex) override
            {
                if (_uIndex >= GetPropertyInfoCount())
                    return nullptr;

                // EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
                return EnumPropertyInfoImp(_uIndex);
            }

            virtual bool __fastcall IsValidProperty(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndex(_Prop) >= 0;
            }

            virtual bool __fastcall IsSubclassOf(IClassInfo* _pClassInfo) override
            {
                if (!_pClassInfo)
                    return false;

                IClassInfo* _pThis = this;

                do
                {
                    if (_pClassInfo == _pThis)
                        return true;

                    _pThis = GetBaseClass();
                } while (_pThis);

                return false;
            }

            static int32_t __fastcall GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint32_t _uIndex = &_Prop - &Element::g_ClassInfoData.Props[0];
                if (_uIndex >= Element::StaticClassInfo::uPropsCount)
                    return -1;

                return _uIndex;
            }

            virtual int32_t __fastcall GetPropertyInfoIndex(const PropertyInfo& _Prop) override
            {
                const unsigned _uIndex = &_Prop - &Element::g_ClassInfoData.Props[0];
                if (_uIndex >= Element::StaticClassInfo::uPropsCount)
                    return -1;

                return _uIndex;
            }

            /// <summary>
            /// 将这个类注册到全局
            /// </summary>
            /// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
            /// <returns>HRESULT</returns>
            static HRESULT __fastcall Register(bool _bExplicitRegister = true)
            {
                HRESULT _hr = S_OK;
                auto& _pClassInfo = Element::g_ClassInfoData.pClassInfoPtr;
                if (_pClassInfo)
                {
                    _pClassInfo->AddRef();

                    if (_bExplicitRegister)
                    {
                        _hr = _pClassInfo->RegisterClassInternal(true);
                        if (FAILED(_hr))
                        {
                            _pClassInfo->Release();
                        }
                    }
                }
                else
                {
                    // 这里不会增加引用计数
                    auto _pTmp = HNewAndZero<Element::StaticClassInfo::ClassInfoType>();
                    if (!_pTmp)
                        return E_OUTOFMEMORY;

                    _hr = _pTmp->RegisterClassInternal(_bExplicitRegister);
                    if (FAILED(_hr))
                    {
                        HDelete(_pTmp);
                    }
                    else
                    {
                        _pTmp->AddRef();
                        _pClassInfo = _pTmp;
                    }
                }

                return _hr;
            }

            HRESULT __fastcall UnRegister()
            {
                auto _pClassInfo = Element::g_ClassInfoData.pClassInfoPtr;
                if (!_pClassInfo)
                    return S_FALSE;

                auto _hr = _pClassInfo->UnregisterClassInternal(true);

                if (SUCCEEDED(_hr))
                {
                    _pClassInfo->Release();
                }

                return _hr;
            }
        };
    } // namespace MegaUI
} // namespace YY

#pragma pack(pop)
