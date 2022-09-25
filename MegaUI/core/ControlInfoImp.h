#pragma once
#include "..\base\MegaUITypeInt.h"
#include "ControlInfo.h"
#include "..\base\alloc.h"
#include "Element.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        template<typename _Class>
        class ControlInfoImp : public IControlInfo
        {
        private:
            uint32_t uRef;

        public:
            ControlInfoImp()
                : uRef(0)
            {
            }

            virtual ~ControlInfoImp()
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
                    UnregisterControlInternal(false);
                    _Class::g_ControlInfoData.pControlInfoPtr = nullptr;
                    HDelete(this);
                }

                return _uNewRef;
            }

            virtual raw_const_astring_t __MEGA_UI_API GetName() override
            {
                return _Class::g_ControlInfoData.szControlName;
            }

            virtual IControlInfo* __MEGA_UI_API GetBaseControlInfo() override
            {
                return _Class::StaticControlInfo::BaseControl::GetStaticControlInfo();
            }

            virtual HRESULT __MEGA_UI_API CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                if (!_ppOutElem)
                    return E_INVALIDARG;
                *_ppOutElem = nullptr;
                _Class* _pClass;
                auto _hr = _Class::Create(_Class::StaticControlInfo::fDefaultCreate, _pTopLevelElem, _pCookies, &_pClass);
                if (SUCCEEDED(_hr))
                    *_ppOutElem = static_cast<Element*>(_pClass);

                return _hr;
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return _Class::StaticControlInfo::uPropsCount + ControlInfoImp<_Class::StaticControlInfo::BaseControl>::GetPropertyInfoCount();
            }

            static const PropertyInfo* __MEGA_UI_API EnumPropertyInfoImp(uint32_t _uIndex)
            {
                constexpr auto _uBasePropCount = ControlInfoImp<_Class::StaticControlInfo::BaseControl>::GetPropertyInfoCount();

                if (_uIndex >= _uBasePropCount)
                {
                    return &_Class::g_ControlInfoData.Props[_uIndex - _uBasePropCount];
                }

                return ControlInfoImp<_Class::StaticControlInfo::BaseControl>::EnumPropertyInfoImp(_uIndex);
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

            virtual bool __MEGA_UI_API IsSubclassOf(IControlInfo* _pControlInfo) override
            {
                if (!_pControlInfo)
                    return false;

                IControlInfo* _pThis = this;

                do
                {
                    if (_pControlInfo == _pThis)
                        return true;

                    _pThis = GetBaseControlInfo();
                } while (_pThis);

                return false;
            }

            static int32_t __MEGA_UI_API GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint_t _uIndex = &_Prop - &_Class::g_ControlInfoData.Props[0];

                if (_uIndex >= _Class::StaticControlInfo::uPropsCount)
                {
                    return ControlInfoImp<_Class::StaticControlInfo::BaseControl>::GetPropertyInfoIndexImp(_Prop);
                }

                static_assert(ControlInfoImp<_Class>::GetPropertyInfoCount() < int32_max, "");
                return int32_t(_uIndex + ControlInfoImp<_Class::StaticControlInfo::BaseControl>::GetPropertyInfoCount());
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
                auto _hr = _Class::StaticControlInfo::BaseControl::StaticControlInfo::ControlInfoType::Register(false);

                if (FAILED(_hr))
                    return _hr;

                auto& _pControlInfo = _Class::g_ControlInfoData.pControlInfoPtr;
                if (_pControlInfo)
                {
                    _pControlInfo->AddRef();

                    if (_bExplicitRegister)
                    {
                        _hr = _pControlInfo->RegisterControlInternal(true);

                        if (FAILED(_hr))
                            _pControlInfo->Release();
                    }
                }
                else
                {
                    // 这里不会增加引用计数
                    auto _pTmp = HNewAndZero<_Class::StaticControlInfo::ControlInfoType>();
                    if (!_pTmp)
                        return E_OUTOFMEMORY;

                    _hr = _pTmp->RegisterControlInternal(_bExplicitRegister);
                    if (FAILED(_hr))
                    {
                        HDelete(_pTmp);
                    }
                    else
                    {
                        _pTmp->AddRef();
                        _pControlInfo = _pTmp;
                    }
                }

                return _hr;
            }

            HRESULT __MEGA_UI_API UnRegister()
            {
                auto _pControlInfo = _Class::g_ControlInfoData.pControlInfoPtr;
                if (!_pControlInfo)
                    return S_FALSE;

                auto _hr = _pControlInfo->UnregisterControlInternal(true);

                if (SUCCEEDED(_hr))
                {
                    IControlInfo* _pThis = this;

                    while (_pThis)
                    {
                        auto _pTmp = _pThis->GetBaseControlInfo();
                        _pThis->Release();
                        _pThis = _pTmp;
                    }
                }

                return _hr;
            }
        };

        template<>
        class ControlInfoImp<Element> : public IControlInfo
        {
        private:
            uint32_t uRef;

        public:
            ControlInfoImp()
                : uRef(0)
            {
            }

            virtual ~ControlInfoImp()
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
                    UnregisterControlInternal(false);
                    Element::g_ControlInfoData.pControlInfoPtr = nullptr;
                    HDelete(this);
                }

                return _uNewRef;
            }

            virtual raw_const_astring_t __MEGA_UI_API GetName() override
            {
                return Element::StaticControlInfo::szControlName;
            }

            virtual IControlInfo* __MEGA_UI_API GetBaseControlInfo() override
            {
                return nullptr;
            }

            virtual HRESULT __MEGA_UI_API CreateInstance(Element* _pTopLevelElem, intptr_t* _pCookies, Element** _ppOutElem) override
            {
                return Element::Create(Element::StaticControlInfo::fDefaultCreate, _pTopLevelElem, _pCookies, _ppOutElem);
            }

            static constexpr uint32_t GetPropertyInfoCount()
            {
                return Element::StaticControlInfo::uPropsCount;
            }

            static const PropertyInfo* __MEGA_UI_API EnumPropertyInfoImp(unsigned int _uIndex)
            {
                return &Element::g_ControlInfoData.Props[_uIndex];
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

            virtual bool __MEGA_UI_API IsSubclassOf(IControlInfo* _pControlInfo) override
            {
                if (!_pControlInfo)
                    return false;

                IControlInfo* _pThis = this;

                do
                {
                    if (_pControlInfo == _pThis)
                        return true;

                    _pThis = GetBaseControlInfo();
                } while (_pThis);

                return false;
            }

            static int32_t __MEGA_UI_API GetPropertyInfoIndexImp(const PropertyInfo& _Prop)
            {
                const uint_t _uIndex = &_Prop - &Element::g_ControlInfoData.Props[0];
                if (_uIndex >= (uint_t)Element::StaticControlInfo::uPropsCount)
                    return -1;

                static_assert(Element::StaticControlInfo::uPropsCount <= int32_max, "");
                return (int32_t)_uIndex;
            }

            virtual int32_t __MEGA_UI_API GetPropertyInfoIndex(const PropertyInfo& _Prop) override
            {
                const uint_t _uIndex = &_Prop - &Element::g_ControlInfoData.Props[0];
                if (_uIndex >= (uint_t)Element::StaticControlInfo::uPropsCount)
                    return -1;

                static_assert(Element::StaticControlInfo::uPropsCount <= int32_max, "");
                return (int32_t)_uIndex;
            }

            /// <summary>
            /// 将这个类注册到全局
            /// </summary>
            /// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
            /// <returns>HRESULT</returns>
            static HRESULT __MEGA_UI_API Register(bool _bExplicitRegister = true)
            {
                HRESULT _hr = S_OK;
                auto& _pControlInfo = Element::g_ControlInfoData.pControlInfoPtr;
                if (_pControlInfo)
                {
                    _pControlInfo->AddRef();

                    if (_bExplicitRegister)
                    {
                        _hr = _pControlInfo->RegisterControlInternal(true);
                        if (FAILED(_hr))
                        {
                            _pControlInfo->Release();
                        }
                    }
                }
                else
                {
                    // 这里不会增加引用计数
                    auto _pTmp = HNewAndZero<Element::StaticControlInfo::ControlInfoType>();
                    if (!_pTmp)
                        return E_OUTOFMEMORY;

                    _hr = _pTmp->RegisterControlInternal(_bExplicitRegister);
                    if (FAILED(_hr))
                    {
                        HDelete(_pTmp);
                    }
                    else
                    {
                        _pTmp->AddRef();
                        _pControlInfo = _pTmp;
                    }
                }

                return _hr;
            }

            HRESULT __MEGA_UI_API UnRegister()
            {
                auto _pControlInfo = Element::g_ControlInfoData.pControlInfoPtr;
                if (!_pControlInfo)
                    return S_FALSE;

                auto _hr = _pControlInfo->UnregisterControlInternal(true);

                if (SUCCEEDED(_hr))
                {
                    _pControlInfo->Release();
                }

                return _hr;
            }
        };
    }
} // namespace YY