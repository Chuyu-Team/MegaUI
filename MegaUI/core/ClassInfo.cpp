#include "pch.h"

#include "ClassInfo.h"

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


        struct RegisterClassInfo
        {
            IClassInfo* pClassInfo;
            uint32_t uRef;
        };

        typedef std::unordered_map<raw_const_astring_t, RegisterClassInfo, hash_raw_const_string, equal_to_raw_const_string, YY::MegaUI::allocator<std::pair<const raw_const_astring_t, RegisterClassInfo>>> RegisterClassInfoHashMap;

        static RegisterClassInfoHashMap g_ClassMap;



        HRESULT __fastcall IClassInfo::RegisterClassInternal(bool _bExplicitRegister)
        {
            std::pair<RegisterClassInfoHashMap::iterator, bool> _itInsert;

            try
            {
                _itInsert = g_ClassMap.insert(std::make_pair(GetName(), RegisterClassInfo{this}));
            }
            catch (const std::exception&)
            {
                return E_OUTOFMEMORY;
            }

            auto& _RegisterInfo = _itInsert.first->second;

            if (!_itInsert.second)
            {
                // Class name相同，但是不是一个 ClassInfo，这种情况我们应该返回错误
                if (_RegisterInfo.pClassInfo != this)
                    return E_ABORT;
            }

            if (_bExplicitRegister)
            {
                ++_RegisterInfo.uRef;
            }

            return S_OK;
        }
        
        HRESULT __fastcall IClassInfo::UnregisterClassInternal(bool _bExplicitRegister)
        {
            auto _it = g_ClassMap.find(GetName());

            if (_it == g_ClassMap.end())
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
                g_ClassMap.erase(_it);
            }

            return S_OK;
        }
        
        IClassInfo* __fastcall GetRegisterControlClassInfo(raw_const_astring_t _szClassName)
        {
            if (!_szClassName)
                return nullptr;

            auto _it = g_ClassMap.find(_szClassName);
            if (_it == g_ClassMap.end())
                return nullptr;

            return _it->second.pClassInfo;
        }

        HRESULT __fastcall UnRegisterAllControls()
        {
            DynamicArray<RegisterClassInfo> _vecTopRegisterClassInfo;
            auto _hr = _vecTopRegisterClassInfo.Reserve(g_ClassMap.size());
            if (FAILED(_hr))
                return _hr;


            for(auto& _Info : g_ClassMap)
            {
                if (_Info.second.uRef)
                {
                    _hr = _vecTopRegisterClassInfo.Add(_Info.second);

                    if (FAILED(_hr))
                        return _hr;
                }
            }

            for (auto& _Info : _vecTopRegisterClassInfo)
            {
                for (; _Info.uRef; --_Info.uRef)
                {
                    auto _hrUnRegister = _Info.pClassInfo->UnRegister();
                    if (FAILED(_hrUnRegister))
                    {
                        _hr = _hrUnRegister;
                        break;
                    }
                }
            }

            // 根据预期，Map应该是空的
            if (SUCCEEDED(_hr) && g_ClassMap.empty() == false)
            {
                _hr = E_FAIL;
            }

            return _hr;
        }
    }
}

