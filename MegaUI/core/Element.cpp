#include "pch.h"
#include "Element.h"
#include "Property.h"
#include "value.h"
#include "ClassInfo.h"

#pragma warning(disable : 28251)
#pragma warning(disable : 26812)

namespace YY
{
	namespace MegaUI
	{
		_APPLY_MEGA_UI_STATIC_CALSS_INFO(Element, _MEGA_UI_ELEMENT_PROPERTY_TABLE);





		Value* __fastcall Element::GetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, bool _bUpdateCache)
        {
            if (_eIndicies >= PropertyIndicies::PI_MAX)
                return Value::GetUnavailable();

            if ((unsigned)_eIndicies > (_Prop.fFlags & PF_TypeBits))
                _eIndicies = (PropertyIndicies)(_Prop.fFlags & PF_TypeBits);

            const auto _iIndex = GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return Value::GetUnavailable();

            Value* _pValue = Value::GetUnset();

            FunTypePropertyCustomCache _pFunPropertyCache = nullptr;

            do
            {

                PropertyCustomCacheResult _CacheResult = SkipNone;
                if (_Prop.BindCacheInfo.pFunPropertyCustomCache)
                {
                    _pFunPropertyCache = _Prop.BindCacheInfo.bValueMapOrCustomPropFun ? &Element::PropertyGeneralCache : _Prop.BindCacheInfo.pFunPropertyCustomCache;

                    PropertyCustomCacheActionInfo _Info = { &_Prop, _eIndicies };
                    _Info.GetValueInfo.bUsingCache = _bUpdateCache == false;

                    _CacheResult = (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::GetValue, &_Info);

                    if (_Info.GetValueInfo.pRetValue)
                    {
                        _pValue = _Info.GetValueInfo.pRetValue;

                        if (_pValue->GetType() != ValueType::Unset)
                            break;
                    }
                }

                // 走 _LocalPropValue，这是一种通用逻辑
                if ((_CacheResult & SkipLocalPropValue) == 0)
                {
                    if (auto _ppValue = LocalPropValue.GetItemPtr(_iIndex))
                    {
                        _pValue = *_ppValue;
                        _pValue->AddRef();
                        break;
                    }
                }

                // 如果值是本地的，那么最多就取一下 _LocalPropValue，我们就需要停止
                if (_eIndicies == PropertyIndicies::PI_Local)
                    break;

                // 尝试获取来自属性表的值
                if ((_Prop.fFlags & PF_Cascade) && (_CacheResult & SkipCascade) == 0)
                {
                }

                // 尝试从父节点继承
                if ((_Prop.fFlags & PF_Inherit) && (_CacheResult & SkipInherit) == 0)
                {
                    if (auto _pParent = GetParent())
                    {
                        auto pValueByParent = _pParent->GetValue(_Prop, _eIndicies, false);

                        if (pValueByParent && _pValue->GetType() >= ValueType::Null)
                        {
                            _pValue = pValueByParent;
                            break;
                        }
                    }
                }

                // 最终还是没有，那么继承Default 值
                _pValue = _Prop.pFunDefaultValue();

            } while (false);

            if (_pFunPropertyCache && _pValue && _pValue->GetType() >= ValueType::Null && (_Prop.fFlags & PF_ReadOnly) == 0 && _bUpdateCache)
            {
                PropertyCustomCacheActionInfo _Info = { &_Prop, _eIndicies };
                _Info.UpdateValueInfo.pNewValue = _pValue;

                (this->*_pFunPropertyCache)(PropertyCustomCacheActionMode::UpdateValue, &_Info);
            }

            if (!_pValue)
                _pValue = Value::GetUnset();

            return _pValue;
        }

        HRESULT __fastcall Element::SetValue(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pValue)
        {
            if (!_pValue)
                return E_POINTER;

            if (_eIndicies != PropertyIndicies::PI_Local)
                return E_NOTIMPL;

            if (_Prop.fFlags & PF_ReadOnly)
                return E_NOTIMPL;

            const auto _iIndex = GetControlClassInfo()->GetPropertyInfoIndex(_Prop);
            if (_iIndex < 0)
                return E_NOT_SET;

            const auto _uIndex = (size_t)_iIndex;

            auto _pvOld = GetValue(_Prop, _eIndicies, false);
            if (!_pvOld)
                return E_OUTOFMEMORY;

            if (_pvOld->IsEqual(_pValue))
                return S_OK;

            PreSourceChange(_Prop, _eIndicies, _pvOld, _pValue);

            auto _hr = S_OK;
            if (LocalPropValue.GetSize() <= _uIndex)
                _hr = LocalPropValue.Resize(_uIndex + 1);

            if (SUCCEEDED(_hr))
                _hr = LocalPropValue.SetItem(_uIndex, _pValue);

            if (SUCCEEDED(_hr))
            {
                _pValue->AddRef();
                _pvOld->Release();
            }

            PostSourceChange();

            return _hr;
		}

		void __fastcall Element::OnPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pOldValue, Value* _pNewValue)
		{
            if (_Prop.pFunOnPropertyChanged)
                (this->*_Prop.pFunOnPropertyChanged)(_Prop, _eIndicies, _pOldValue, _pNewValue);
		}

		void __fastcall Element::StartDefer(intptr_t* _pCooike)
		{
            if (!_pCooike)
			{
				throw std::exception("pCooike == nullptr", 0);
				return;
			}

			if (auto _pDeferCycle = GetDeferObject())
			{
                ++_pDeferCycle->uEnter;


				// 随便写一个值，看起来比较特殊就可以了
                *_pCooike = 0x12345;

				_pDeferCycle->AddRef();
			}
		}

		void __fastcall Element::EndDefer(intptr_t _Cookie)
		{
            if (_Cookie != 0x12345)
			{
				throw std::exception("Cookie Error", 0);
				return;
			}
		}
		
		PropertyCustomCacheResult __fastcall Element::PropertyGeneralCache(PropertyCustomCacheActionMode _eMode, PropertyCustomCacheActionInfo* _pInfo)
		{
            uint16_t _uOffsetToCache = 0;
            uint16_t _uOffsetToHasCache = 0;
            uint8_t _uCacheBit;
            uint8_t _uHasCacheBit;

			auto _pProp = _pInfo->pProp;

			switch (_pInfo->eIndicies)
			{
			case PropertyIndicies::PI_Computed:
			case PropertyIndicies::PI_Specified:
                if (_pProp->BindCacheInfo.OffsetToSpecifiedValue)
				{
                    _uOffsetToCache = _pProp->BindCacheInfo.OffsetToSpecifiedValue;
                    _uCacheBit = _pProp->BindCacheInfo.SpecifiedValueBit;
                    _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasSpecifiedValueCache;
                    _uHasCacheBit = _pProp->BindCacheInfo.HasSpecifiedValueCacheBit;
					break;
				}
			case PropertyIndicies::PI_Local:
                _uOffsetToCache = _pProp->BindCacheInfo.OffsetToLocalValue;
                _uCacheBit = _pProp->BindCacheInfo.LocalValueBit;
                _uOffsetToHasCache = _pProp->BindCacheInfo.OffsetToHasLocalCache;
                _uHasCacheBit = _pProp->BindCacheInfo.HasLocalValueCacheBit;
				break;
			default:
				return PropertyCustomCacheResult::SkipNone;
				break;
			}

			if (_eMode == PropertyCustomCacheActionMode::GetValue)
			{
                Value* _pRetValue = nullptr;

				do
				{
                    if (_uOffsetToCache == 0)
						break;

					// 如果属性是 PF_ReadOnly，那么它必然不会实际走到 _LocalPropValue 里面去，必须走 缓存
					// 如果 _UsingCache == true，那么我们可以走缓存
                    if ((_pProp->fFlags & PF_ReadOnly) || _pInfo->GetValueInfo.bUsingCache)
					{
						// 检测实际是否存在缓存，如果检测到没有缓存，那么直接返回
                        if (_uOffsetToHasCache)
						{
                            const auto _uHasValue = *((uint8_t*)this + _uOffsetToHasCache);
                            if ((_uHasValue & (1 << _uHasCacheBit)) == 0)
							{
								break;
							}
						}

						auto _pCache = (char*)this + _uOffsetToCache;

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            _pRetValue = Value::CreateInt32(*(int32_t*)_pCache);
							break;
						case ValueType::boolean:
                            _pRetValue = Value::CreateBool((*(uint8_t*)_pCache) & (1 << _uCacheBit));
							break;
						default:
							break;
						}
					}
				} while (false);
				
				_pInfo->GetValueInfo.pRetValue = _pRetValue ? _pRetValue : Value::GetUnset();

				if (_pProp->fFlags & PF_ReadOnly)
				{
					return PropertyCustomCacheResult::SkipLocalPropValue;
				}

				return PropertyCustomCacheResult::SkipNone;
			}
            else if (_eMode == PropertyCustomCacheActionMode::UpdateValue)
			{
				do
				{
                    if (_uOffsetToCache == 0)
						break;

					auto _pNewValue = _pInfo->UpdateValueInfo.pNewValue;

					if (!_pNewValue)
						break;

					if (_pNewValue->GetType() == ValueType::Unset)
					{
                        if (_uOffsetToHasCache == 0)
							break;

						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);

						_uHasCache &= ~(1 << _uHasCacheBit);
					}
                    else if (_pNewValue->GetType() == (ValueType)_pProp->BindCacheInfo.eType)
					{
						// 标记缓存已经被设置
						auto& _uHasCache = *((uint8_t*)this + _uOffsetToHasCache);
						_uHasCache |= (1 << _uHasCacheBit);

						auto _pCache = (char*)this + _uOffsetToCache;

						switch ((ValueType)_pProp->BindCacheInfo.eType)
						{
						case ValueType::int32_t:
                            *(int32_t*)_pCache = _pNewValue->GetInt32();
							break;
						case ValueType::boolean:
							if (_pNewValue->GetBool())
							{
                                *_pCache |= (1 << _uCacheBit);
							}
							else
							{
                                *_pCache &= ~(1 << _uCacheBit);
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
		
		void __fastcall Element::OnParentPropertyChanged(const PropertyInfo& _Prop, PropertyIndicies _eIndicies, Value* _pOldValue, Value* pNewValue)
		{
			
		}
	}
}
