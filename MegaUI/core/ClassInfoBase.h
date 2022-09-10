#pragma once
#include "..\base\MegaUITypeInt.h"
#include "ClassInfo.h"
#include "..\base\alloc.h"
#include "Element.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
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

            virtual uint32_t __MEGA_UI_API AddRef() override
            {
                return ++uRef;
            }

            virtual uint32_t __MEGA_UI_API Release() override
            {
                const auto _uNewRef = --uRef;

                if (_uNewRef == 0)
                {
                    UnregisterClassInternal(false);
                    _Class::g_ClassInfoData.pClassInfoPtr = nullptr;
                    HDelete(this);
                }

                return _uNewRef;
            }

            virtual raw_const_astring_t __MEGA_UI_API GetName() override
            {
                return _Class::g_ClassInfoData.szClassName;
            }

            virtual IClassInfo* __MEGA_UI_API GetBaseClass() override
            {
                return _Class::StaticClassInfo::BaseElement::GetStaticControlClassInfo();
            }

            virtual HRESULT __MEGA_UI_API CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                if (!_ppOutElem)
                    return E_INVALIDARG;
                *_ppOutElem = nullptr;
                _Class* _pClass;
                auto _hr = _Class::Create(_Class::StaticClassInfo::fDefaultCreate, _pTopLevelElem, _pCookies, &_pClass);
                if (SUCCEEDED(_hr))
                    *_ppOutElem = static_cast<Element*>(_pClass);

                return _hr;
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return _Class::StaticClassInfo::uPropsCount + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
            }

            static const PropertyInfo* __MEGA_UI_API EnumPropertyInfoImp(uint32_t _uIndex)
            {
                constexpr auto _uBasePropCount = ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();

                if (_uIndex >= _uBasePropCount)
                {
                    return &_Class::g_ClassInfoData.Props[_uIndex - _uBasePropCount];
                }

                return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::EnumPropertyInfoImp(_uIndex);
            }

            virtual const PropertyInfo* __MEGA_UI_API EnumPropertyInfo(uint32_t _uIndex) override
            {
                if (_uIndex >= GetPropertyInfoCount())
                    return nullptr;

                // EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
                return EnumPropertyInfoImp(_uIndex);
            }

            virtual bool __MEGA_UI_API IsValidProperty(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndex(_Prop) >= 0;
            }

            virtual bool __MEGA_UI_API IsSubclassOf(IClassInfo* _pClassInfo) override
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

            static int32_t __MEGA_UI_API GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint32_t _uIndex = &_Prop - &_Class::g_ClassInfoData.Props[0];

                if (_uIndex >= _Class::StaticClassInfo::uPropsCount)
                {
                    return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoIndexImp(_Prop);
                }

                return _uIndex + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
            }

            virtual int32_t __MEGA_UI_API GetPropertyInfoIndex(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndexImp(_Prop);
            }

            /// <summary>
            /// 将这个类注册到全局
            /// </summary>
            /// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
            /// <returns>HRESULT</returns>
            static HRESULT __MEGA_UI_API Register(bool _bExplicitRegister = true)
            {
                auto _hr = _Class::StaticClassInfo::BaseElement::StaticClassInfo::ClassInfoType::Register(false);

                if (FAILED(_hr))
                    return _hr;

                auto& _pClassInfo = _Class::g_ClassInfoData.pClassInfoPtr;
                if (_pClassInfo)
                {
                    _pClassInfo->AddRef();

                    if (_bExplicitRegister)
                    {
                        _hr = _pClassInfo->RegisterClassInternal(true);

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

            HRESULT __MEGA_UI_API UnRegister()
            {
                auto _pClassInfo = _Class::g_ClassInfoData.pClassInfoPtr;
                if (!_pClassInfo)
                    return S_FALSE;

                auto _hr = _pClassInfo->UnregisterClassInternal(true);

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

            virtual uint32_t __MEGA_UI_API AddRef() override
            {
                return ++uRef;
            }

            virtual uint32_t __MEGA_UI_API Release() override
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

            virtual raw_const_astring_t __MEGA_UI_API GetName() override
            {
                return Element::StaticClassInfo::szClassName;
            }

            virtual IClassInfo* __MEGA_UI_API GetBaseClass() override
            {
                return nullptr;
            }

            virtual HRESULT __MEGA_UI_API CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                return Element::Create(Element::StaticClassInfo::fDefaultCreate, _pTopLevelElem, _pCookies, _ppOutElem);
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return Element::StaticClassInfo::uPropsCount;
            }

            static const PropertyInfo* __MEGA_UI_API EnumPropertyInfoImp(unsigned int _uIndex)
            {
                return &Element::g_ClassInfoData.Props[_uIndex];
            }

            virtual const PropertyInfo* __MEGA_UI_API EnumPropertyInfo(unsigned int _uIndex) override
            {
                if (_uIndex >= GetPropertyInfoCount())
                    return nullptr;

                // EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
                return EnumPropertyInfoImp(_uIndex);
            }

            virtual bool __MEGA_UI_API IsValidProperty(const PropertyInfo& _Prop) override
            {
                return GetPropertyInfoIndex(_Prop) >= 0;
            }

            virtual bool __MEGA_UI_API IsSubclassOf(IClassInfo* _pClassInfo) override
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

            static int32_t __MEGA_UI_API GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint32_t _uIndex = &_Prop - &Element::g_ClassInfoData.Props[0];
                if (_uIndex >= Element::StaticClassInfo::uPropsCount)
                    return -1;

                return _uIndex;
            }

            virtual int32_t __MEGA_UI_API GetPropertyInfoIndex(const PropertyInfo& _Prop) override
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
            static HRESULT __MEGA_UI_API Register(bool _bExplicitRegister = true)
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

            HRESULT __MEGA_UI_API UnRegister()
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
    }
} // namespace YY