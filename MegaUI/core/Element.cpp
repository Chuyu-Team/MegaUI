#include "pch.h"
#include "Element.h"
#include "Property.h"
#include "value.h"
#include "ClassInfo.h"

namespace YY
{
	namespace MegaUI
	{
		_APPLY_MEGA_UI_STATIC_CALSS_INFO(Element, _MEGA_UI_ELEMENT_PROPERTY_TABLE);





		Value* __fastcall Element::GetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, bool bUpdateCache)
		{
			if (eIndicies >= PropertyIndicies::PI_MAX)
				return Value::GetUnavailable();

			if ((unsigned)eIndicies > (Prop.fFlags & PF_TypeBits))
				eIndicies = (PropertyIndicies)(Prop.fFlags & PF_TypeBits);


			const auto iIndex = GetClassInfo()->GetPropertyInfoIndex(Prop);
			if (iIndex < 0)
				return Value::GetUnavailable();

			Value* pValue = Value::GetUnset();

			
			FunTypePropertyCustomCache pFunPropertyCache = nullptr;

			do
			{

				PropertyCustomCacheResult _CacheResult = SkipNone;
				if (Prop.BindCacheInfo.pFunPropertyCustomCache)
				{
					pFunPropertyCache = Prop.BindCacheInfo.bValueMapOrCustomPropFun ? &Element::_PropertyGeneralCache : Prop.BindCacheInfo.pFunPropertyCustomCache;

					PropertyCustomCacheActionInfo Info = { &Prop, eIndicies };
					Info.GetValueInfo.bUsingCache = bUpdateCache == false;

					_CacheResult = (this->*pFunPropertyCache)(PropertyCustomCacheActionMode::GetValue, &Info);

					if (Info.GetValueInfo.pRetValue)
					{
						pValue = Info.GetValueInfo.pRetValue;

						if (pValue->GetType() != ValueType::Unset)
							break;
					}
				}

				// 走 _LocalPropValue，这是一种通用逻辑
				if ((_CacheResult & SkipLocalPropValue) == 0)
				{
					if (auto ppValue = _LocalPropValue.GetItemPtr(iIndex))
					{
						pValue = *ppValue;
						pValue->AddRef();
						break;
					}
				}

				// 如果值是本地的，那么最多就取一下 _LocalPropValue，我们就需要停止
				if (eIndicies == PropertyIndicies::PI_Local)
					break;


				// 尝试获取来自属性表的值
				if ((Prop.fFlags & PF_Cascade) &&(_CacheResult & SkipCascade) == 0)
				{
					
				}

				// 尝试从父节点继承
				if ((Prop.fFlags & PF_Inherit) && (_CacheResult & SkipInherit) == 0)
				{
					if (auto pParent = GetParent())
					{
						auto pValueByParent = pParent->GetValue(Prop, eIndicies, false);

						if (pValueByParent && pValue->GetType() >= ValueType::Null)
						{
							pValue = pValueByParent;
							break;
						}
					}
				}


				// 最终还是没有，那么继承Default 值
				pValue = Prop.pFunDefaultValue();
				
			} while (false);

			if (pFunPropertyCache && pValue && pValue->GetType() >= ValueType::Null
				&& (Prop.fFlags & PF_ReadOnly) == 0 && bUpdateCache)
			{
				PropertyCustomCacheActionInfo Info = { &Prop, eIndicies };
				Info.UpdateValueInfo.pNewValue = pValue;

				(this->*pFunPropertyCache)(PropertyCustomCacheActionMode::UpdateValue, &Info);
			}

			if(!pValue)
				pValue = Value::GetUnset();

			return pValue;
		}

		HRESULT __fastcall Element::SetValue(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pValue)
		{
			if (!pValue)
				return E_POINTER;

			if (eIndicies != PropertyIndicies::PI_Local)
				return E_NOTIMPL;

			if (Prop.fFlags & PF_ReadOnly)
				return E_NOTIMPL;

			const auto iIndex = GetClassInfo()->GetPropertyInfoIndex(Prop);
			if (iIndex < 0)
				return E_NOT_SET;

			const auto uIndex = (size_t)iIndex;

			auto pvOld = GetValue(Prop, eIndicies, false);
			if (!pvOld)
				return E_OUTOFMEMORY;

			if (pvOld->IsEqual(pValue))
				return S_OK;

			_PreSourceChange(Prop, eIndicies, pvOld, pValue);

			auto hr = S_OK;
			if(_LocalPropValue.GetSize() <= uIndex)
				hr = _LocalPropValue.Resize(uIndex + 1);

			if(SUCCEEDED(hr))
				hr = _LocalPropValue.SetItem(uIndex, pValue);

			if (SUCCEEDED(hr))
			{
				pValue->AddRef();
				pvOld->Release();
			}

			_PostSourceChange();

			return hr;
		}

		void __fastcall Element::OnPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew)
		{
			if (Prop.pFunOnPropertyChanged)
				(this->*Prop.pFunOnPropertyChanged)(Prop, eIndicies, pvOld, pvNew);
		}

		void __fastcall Element::StartDefer(intptr_t* pCooike)
		{
			if (!pCooike)
			{
				throw std::exception("pCooike == nullptr", 0);
				return;
			}

			if (auto pDeferCycle = GetDeferObject())
			{
				++pDeferCycle->uEnter;


				// 随便写一个值，看起来比较特殊就可以了
				*pCooike = 0x12345;

				pDeferCycle->AddRef();
			}
		}

		void __fastcall Element::EndDefer(intptr_t Cookie)
		{
			if (Cookie != 0x12345)
			{
				throw std::exception("Cookie Error", 0);
				return;
			}
		}
		
		PropertyCustomCacheResult __fastcall Element::_PropertyGeneralCache(PropertyCustomCacheActionMode eMode, PropertyCustomCacheActionInfo* pInfo)
		{
			uint16_t OffsetToCache = 0;
			uint16_t OffsetToHasCache = 0;
			uint8_t CacheBit;
			uint8_t HasCacheBit;

			auto pProp = pInfo->pProp;

			switch (pInfo->eIndicies)
			{
			case PropertyIndicies::PI_Computed:
			case PropertyIndicies::PI_Specified:
				if (pProp->BindCacheInfo.OffsetToSpecifiedValue)
				{
					OffsetToCache = pProp->BindCacheInfo.OffsetToSpecifiedValue;
					CacheBit = pProp->BindCacheInfo.SpecifiedValueBit;
					OffsetToHasCache = pProp->BindCacheInfo.OffsetToHasSpecifiedValueCache;
					HasCacheBit = pProp->BindCacheInfo.HasSpecifiedValueCacheBit;
					break;
				}
			case PropertyIndicies::PI_Local:
				OffsetToCache = pProp->BindCacheInfo.OffsetToLocalValue;
				CacheBit = pProp->BindCacheInfo.LocalValueBit;
				OffsetToHasCache = pProp->BindCacheInfo.OffsetToHasLocalCache;
				HasCacheBit = pProp->BindCacheInfo.HasLocalValueCacheBit;
				break;
			default:
				return PropertyCustomCacheResult::SkipNone;
				break;
			}

			if (eMode == PropertyCustomCacheActionMode::GetValue)
			{
				Value* pRetValue = nullptr;

				do
				{
					if (OffsetToCache == 0)
						break;

					// 如果属性是 PF_ReadOnly，那么它必然不会实际走到 _LocalPropValue 里面去，必须走 缓存
					// 如果 _UsingCache == true，那么我们可以走缓存
					if ((pProp->fFlags & PF_ReadOnly) || pInfo->GetValueInfo.bUsingCache)
					{
						// 检测实际是否存在缓存，如果检测到没有缓存，那么直接返回
						if (OffsetToHasCache)
						{
							const auto HasValue = *((char*)this + OffsetToHasCache);
							if ((HasValue & (1 << HasCacheBit)) == 0)
							{
								break;
							}
						}

						auto pCache = (char*)this + OffsetToCache;

						switch ((ValueType)pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
							pRetValue = Value::CreateInt32(*(int32_t*)pCache);
							break;
						case ValueType::boolean:
							pRetValue = Value::CreateBool((*(uint8_t*)pCache) & (1 << CacheBit));
							break;
						default:
							break;
						}
					}
				} while (false);
				
				pInfo->GetValueInfo.pRetValue = pRetValue ? pRetValue : Value::GetUnset();

				if (pProp->fFlags & PF_ReadOnly)
				{
					return PropertyCustomCacheResult::SkipLocalPropValue;
				}

				return PropertyCustomCacheResult::SkipNone;
			}
			else if (eMode == PropertyCustomCacheActionMode::UpdateValue)
			{
				do
				{
					if (OffsetToCache == 0)
						break;

					auto pNewValue = pInfo->UpdateValueInfo.pNewValue;

					if (!pNewValue)
						break;

					if (pNewValue->GetType() == ValueType::Unset)
					{
						if (OffsetToHasCache == 0)
							break;

						auto& HasCache = *((char*)this + OffsetToHasCache);

						HasCache &= ~(1 << HasCacheBit);
					}
					else if (pNewValue->GetType() == (ValueType)pProp->BindCacheInfo.eType)
					{
						// 标记缓存已经被设置
						auto& HasCache = *((char*)this + OffsetToHasCache);
						HasCache |= (1 << HasCacheBit);

						auto pCache = (char*)this + OffsetToCache;

						switch ((ValueType)pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
							*(int32_t*)pCache = pNewValue->GetInt32();
							break;
						case ValueType::boolean:
							if (pNewValue->GetBool())
							{
								*pCache |= (1 << CacheBit);
							}
							else
							{
								*pCache &= ~(1 << CacheBit);
							}
							break;
						default:
							break;
						}
					}

				} while (false);
			}

			return PropertyCustomCacheResult::SkipNone;
		}
		
		void __fastcall Element::_OnParentPropertyChanged(const PropertyInfo& Prop, PropertyIndicies eIndicies, Value* pvOld, Value* pvNew)
		{
			
		}
	}
}
