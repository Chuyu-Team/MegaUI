#include "pch.h"

#include "ControlInfo.h"
#include <Base/Containers/Array.h>

#include <unordered_map>

#pragma warning(disable : 28251)

namespace YY
{
    namespace MegaUI
    {
        struct hash_raw_const_string
        {
            size_t operator()(raw_const_astring_t _Keyval) const noexcept
            {
                return std::_Hash_array_representation(_Keyval, strlen(_Keyval));
            }
        };

        struct equal_to_raw_const_string
        {
            bool operator()(raw_const_astring_t _Left, raw_const_astring_t _Right) const
            {
                return strcmp(_Left, _Right) == 0;
            }
        };


        struct RegisterControlInfo
        {
            IControlInfo* pControlInfo;
            uint32_t uRef;
        };

        typedef std::unordered_map<raw_const_astring_t, RegisterControlInfo, hash_raw_const_string, equal_to_raw_const_string, YY::MegaUI::allocator<std::pair<const raw_const_astring_t, RegisterControlInfo>>> RegisterClassInfoHashMap;

        static RegisterClassInfoHashMap g_ControlInfoMap;



        HRESULT __YYAPI IControlInfo::RegisterControlInternal(bool _bExplicitRegister)
        {
            std::pair<RegisterClassInfoHashMap::iterator, bool> _itInsert;

            try
            {
                _itInsert = g_ControlInfoMap.insert(std::make_pair(GetName(), RegisterControlInfo{this}));
            }
            catch (const std::exception&)
            {
                return E_OUTOFMEMORY;
            }

            auto& _RegisterInfo = _itInsert.first->second;

            if (!_itInsert.second)
            {
                // Class name相同，但是不是一个 ClassInfo，这种情况我们应该返回错误
                if (_RegisterInfo.pControlInfo != this)
                    return E_ABORT;
            }

            if (_bExplicitRegister)
            {
                ++_RegisterInfo.uRef;
            }

            return S_OK;
        }
        
        HRESULT __YYAPI IControlInfo::UnregisterControlInternal(bool _bExplicitRegister)
        {
            auto _it = g_ControlInfoMap.find(GetName());

            if (_it == g_ControlInfoMap.end())
                return S_FALSE;

            auto& _RegisterInfo = _it->second;

            if (_bExplicitRegister)
            {
                if (_RegisterInfo.uRef == 0)
                    return S_FALSE;

                --_RegisterInfo.uRef;
            }
            else
            {
                g_ControlInfoMap.erase(_it);
            }

            return S_OK;
        }
        
        IControlInfo* __YYAPI GetRegisterControlInfo(raw_const_astring_t _szControlName)
        {
            if (!_szControlName)
                return nullptr;

            auto _it = g_ControlInfoMap.find(_szControlName);
            if (_it == g_ControlInfoMap.end())
                return nullptr;

            return _it->second.pControlInfo;
        }

        HRESULT __YYAPI UnRegisterAllControls()
        {
            Array<RegisterControlInfo, AllocPolicy::SOO> _vecTopRegisterControlInfo;
            auto _hr = _vecTopRegisterControlInfo.Reserve(g_ControlInfoMap.size());
            if (FAILED(_hr))
                return _hr;


            for (auto& _Info : g_ControlInfoMap)
            {
                if (_Info.second.uRef)
                {
                    _hr = _vecTopRegisterControlInfo.Add(_Info.second);

                    if (FAILED(_hr))
                        return _hr;
                }
            }

            for (auto& _Info : _vecTopRegisterControlInfo)
            {
                for (; _Info.uRef; --_Info.uRef)
                {
                    auto _hrUnRegister = _Info.pControlInfo->UnRegister();
                    if (FAILED(_hrUnRegister))
                    {
                        _hr = _hrUnRegister;
                        break;
                    }
                }
            }

            // 根据预期，Map应该是空的
            if (SUCCEEDED(_hr) && g_ControlInfoMap.empty() == false)
            {
                _hr = E_FAIL;
            }

            return _hr;
        }
    }
}

