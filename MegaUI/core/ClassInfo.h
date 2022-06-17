#pragma once
#include <Windows.h>

#include "..\base\MegaUITypeInt.h"

#include "..\base\alloc.h"
#include "Property.h"
#include "Element.h"


#pragma pack(push)
#pragma pack()

namespace YY
{
	namespace MegaUI
	{
		class IClassInfo
		{
		public:
			virtual uint32_t __fastcall AddRef() = 0;
			virtual uint32_t __fastcall Release() = 0;

			virtual raw_const_astring_t __fastcall GetClassName() = 0;
			virtual IClassInfo* __fastcall GetBaseClass() = 0;

			virtual HRESULT __fastcall CreateInstance(Element* pElem, intptr_t* pCookies, Element** ppElem) = 0;
			virtual const PropertyInfo* __fastcall EnumPropertyInfo(uint32_t uIndex) = 0;
			virtual int32_t __fastcall GetPropertyInfoIndex(const PropertyInfo& Prop) = 0;

			virtual bool __fastcall IsValidProperty(const PropertyInfo& Prop) = 0;
			virtual bool __fastcall IsSubclassOf(IClassInfo* pClassInfo) = 0;

			virtual HRESULT __fastcall UnRegister() = 0;
		protected:
			HRESULT __fastcall _RegisterClass(bool bExplicitRegister);
			HRESULT __fastcall _UnregisterClass(bool bExplicitRegister);
		};

		IClassInfo* __fastcall GetRegisterControlClassInfo(raw_const_astring_t pszClassName);

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
				const auto uNewRef = --uRef;

				if (uNewRef == 0)
				{
					_UnregisterClass(false);
					_Class::ClassInfoData.pClassInfoPtr = nullptr;
					HDelete(this);
				}
				
				return uNewRef;
			}

			virtual raw_const_astring_t __fastcall GetClassName()  override
			{
				return _Class::StaticClassInfo::pszClassInfoName;
			}

			virtual IClassInfo* __fastcall GetBaseClass() override
			{
				return _Class::StaticClassInfo::BaseElement::GetStaticClassInfo();
			}

			virtual HRESULT __fastcall CreateInstance(Element* pTopLevelElem, intptr_t* pCookies, Element** ppOutElem) override
			{
				return _Class::Create(_Class::StaticClassInfo::fDefaultCreate, pTopLevelElem, pCookies, ppOutElem);
			}

			static constexpr uint32_t GetPropertyInfoCount()
			{
				return _Class::StaticClassInfo::uPropsCount + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
			}

			static const PropertyInfo* __fastcall EnumPropertyInfoImp(uint32_t uIndex)
			{
				constexpr auto uBasePropCount = ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();

				if (uIndex >= uBasePropCount)
				{
					return &_Class::_ClassInfoData._Props[uIndex - uBasePropCount];
				}

				return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::EnumPropertyInfoImp();
			}

			virtual const PropertyInfo* __fastcall EnumPropertyInfo(uint32_t uIndex) override
			{
				if (uIndex >= GetPropertyInfoCount())
					return nullptr;

				// EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
				return EnumPropertyInfoImp(uIndex);
			}

			virtual bool __fastcall IsValidProperty(const PropertyInfo& Prop) override
			{
				return GetPropertyInfoIndex(Prop) >= 0;
			}

			virtual bool __fastcall IsSubclassOf(IClassInfo* pClassInfo) override
			{
				if (!pClassInfo)
					return false;

				IClassInfo* pThis = this;

				do
				{
					if (pClassInfo == pThis)
						return true;

					pThis = GetBaseClass();
				} while (pThis);

				return false;
			}

			static int32_t __fastcall GetPropertyInfoIndexImp(const PropertyInfo& Prop)
			{
				const uint32_t uIndex = &Prop - &_Class::ClassInfoData.Props[0];

				if (uIndex >= _Class::StaticClassInfo::uPropsCount)
				{
					return ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoIndexImp(Prop);
				}

				
				return uIndex + ClassInfoBase<_Class::StaticClassInfo::BaseElement>::GetPropertyInfoCount();
			}

			virtual int32_t __fastcall GetPropertyInfoIndex(const PropertyInfo& Prop) override
			{
				return GetPropertyInfoIndexImp(Prop);
			}

			/// <summary>
			/// 将这个类注册到全局
			/// </summary>
			/// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
			/// <returns>HRESULT</returns>
			static HRESULT __fastcall Register(bool bExplicitRegister = true)
			{
				auto hr = _Class::StaticClassInfo::BaseElement::ClassInfoType::Register(false);

				if (FAILED(hr))
					return hr;

				auto& pClassInfo = _Class::ClassInfoData.pClassInfoPtr;
				if (pClassInfo)
				{
					pClassInfo->AddRef();

					if (bExplicitRegister)
					{
						hr = pClassInfo->_RegisterClass(true);

						if (FAILED(hr))
							pClassInfo->Release();
					}
				}
				else
				{
					// 这里不会增加引用计数
					auto pTmp = HNewAndZero<_Class::StaticClassInfo::ClassInfoType>();
					if (!pTmp)
						return E_OUTOFMEMORY;

					hr = pTmp->_RegisterClass(bExplicitRegister);
					if (FAILED(hr))
					{
						HDelete(pTmp);
					}
					else
					{
						pTmp->AddRef();
						pClassInfo = pTmp;
					}
				}

				return hr;
			}

			HRESULT __fastcall UnRegister()
			{
				auto pClassInfo = _Class::ClassInfoData.pClassInfoPtr;
				if (!pClassInfo)
					return S_FALSE;

				auto hr = pClassInfo->_UnregisterClass(true);

				if (SUCCEEDED(hr))
				{
					IClassInfo* pThis = this;

					while (pThis)
					{
						auto pTmp = pThis->GetBaseClass();
						pThis->Release();
						pThis = pTmp;
					}
				}

				return hr;
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
				const auto uNewRef = --uRef;

				if (uNewRef == 0)
				{
					_UnregisterClass(false);
					Element::ClassInfoData.pClassInfoPtr = nullptr;
					HDelete(this);
				}

				return uNewRef;
			}

			virtual raw_const_astring_t __fastcall GetClassName()  override
			{
				return Element::StaticClassInfo::pszClassInfoName;
			}

			virtual IClassInfo* __fastcall GetBaseClass() override
			{
				return nullptr;
			}

			virtual HRESULT __fastcall CreateInstance(Element* pTopLevelElem, intptr_t* pCookies, Element** ppOutElem) override
			{
				return Element::Create(Element::StaticClassInfo::fDefaultCreate, pTopLevelElem, pCookies, ppOutElem);
			}

			static constexpr uint32_t GetPropertyInfoCount()
			{
				return Element::StaticClassInfo::uPropsCount;
			}

			
			static const PropertyInfo* __fastcall EnumPropertyInfoImp(unsigned int uIndex)
			{
				return &Element::ClassInfoData.Props[uIndex];
			}

			virtual const PropertyInfo* __fastcall EnumPropertyInfo(unsigned int uIndex) override
			{
				if (uIndex >= GetPropertyInfoCount())
					return nullptr;

				// EnumPropertyInfo 开始前保证参数不会越界，所以在 EnumPropertyInfoImp 内部不负责越界检查
				return EnumPropertyInfoImp(uIndex);
			}

			virtual bool __fastcall IsValidProperty(const PropertyInfo& Prop) override
			{
				return GetPropertyInfoIndex(Prop) >= 0;
			}

			virtual bool __fastcall IsSubclassOf(IClassInfo* pClassInfo) override
			{
				if (!pClassInfo)
					return false;

				IClassInfo* pThis = this;

				do
				{
					if (pClassInfo == pThis)
						return true;

					pThis = GetBaseClass();
				} while (pThis);

				return false;
			}

			static int32_t __fastcall GetPropertyInfoIndexImp(const PropertyInfo& Prop)
			{
				const uint32_t uIndex = &Prop - &Element::ClassInfoData.Props[0];
				if (uIndex >= Element::StaticClassInfo::uPropsCount)
					return -1;

				return uIndex;
			}

			virtual int32_t __fastcall GetPropertyInfoIndex(const PropertyInfo& Prop) override
			{
				const unsigned uIndex = &Prop - &Element::ClassInfoData.Props[0];
				if (uIndex >= Element::StaticClassInfo::uPropsCount)
					return -1;

				return uIndex;
			}


			/// <summary>
			/// 将这个类注册到全局
			/// </summary>
			/// <param name="bExplicitRegister">如果为 true，那么表示它被用户所注册。如果为false，说明它被子控件继承注册。</param>
			/// <returns>HRESULT</returns>
			static HRESULT __fastcall Register(bool bExplicitRegister = true)
			{
				HRESULT hr = S_OK;
				auto& pClassInfo = Element::ClassInfoData.pClassInfoPtr;
				if (pClassInfo)
				{
					pClassInfo->AddRef();

					if (bExplicitRegister)
					{
						hr = pClassInfo->_RegisterClass(true);
						if (FAILED(hr))
						{
							pClassInfo->Release();
						}
					}
				}
				else
				{
					// 这里不会增加引用计数
					auto pTmp = HNewAndZero<Element::StaticClassInfo::ClassInfoType>();
					if (!pTmp)
						return E_OUTOFMEMORY;

					hr = pTmp->_RegisterClass(bExplicitRegister);
					if (FAILED(hr))
					{
						HDelete(pTmp);
					}
					else
					{
						pTmp->AddRef();
						pClassInfo = pTmp;
					}
					
				}

				return hr;
			}

			HRESULT __fastcall UnRegister()
			{
				auto pClassInfo = Element::ClassInfoData.pClassInfoPtr;
				if (!pClassInfo)
					return S_FALSE;

				auto hr = pClassInfo->_UnregisterClass(true);

				if (SUCCEEDED(hr))
				{
					pClassInfo->Release();
				}

				return hr;
			}
		};
	}
}

#pragma pack(pop)
