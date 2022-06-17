#include "pch.h"

#include "ClassInfo.h"

#include <unordered_map>

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



        HRESULT __fastcall IClassInfo::_RegisterClass(bool bExplicitRegister)
        {
            auto szClassName = GetClassName();


            std::pair<RegisterClassInfoHashMap::iterator, bool> InsertIter;

            try
            {
                InsertIter = g_ClassMap.insert(std::make_pair(szClassName, RegisterClassInfo{ this }));
            }
            catch (const std::exception&)
            {
                return E_OUTOFMEMORY;
            }

            auto& RegisterInfo = InsertIter.first->second;

            if (!InsertIter.second)
            {
                // Class name相同，但是不是一个 ClassInfo，这种情况我们应该返回错误
                if (RegisterInfo.pClassInfo != this)
                    return E_ABORT;
            }

            if (bExplicitRegister)
            {
                ++RegisterInfo.uRef;
            }

            return S_OK;
        }
        
        HRESULT __fastcall IClassInfo::_UnregisterClass(bool bExplicitRegister)
        {
            auto szClassName = GetClassName();
            if (!szClassName)
                return E_UNEXPECTED;

            auto IterItem = g_ClassMap.find(szClassName);

            if (IterItem == g_ClassMap.end())
                return S_FALSE;

            auto& RegisterInfo = IterItem->second;

            if (bExplicitRegister)
            {
                if(RegisterInfo.uRef == 0)
                    return S_FALSE;

                --RegisterInfo.uRef;
            }
            else
            {
                g_ClassMap.erase(IterItem);
            }

            return S_OK;
        }
        
        IClassInfo* __fastcall GetRegisterControlClassInfo(raw_const_astring_t pszClassName)
        {
            if (!pszClassName)
                return nullptr;

            auto iter = g_ClassMap.find(pszClassName);
            if(iter == g_ClassMap.end())
                return nullptr;

            return iter->second.pClassInfo;
        }

        HRESULT __fastcall UnRegisterAllControls()
        {
            DynamicArray<RegisterClassInfo> vecTopRegisterClassInfo;
            auto hr = vecTopRegisterClassInfo.Reserve(g_ClassMap.size());
            if (FAILED(hr))
                return hr;


            for(auto& Info : g_ClassMap)
            {
                if (Info.second.uRef)
                {
                    hr = vecTopRegisterClassInfo.Add(Info.second);

                    if (FAILED(hr))
                        return hr;
                }
            }

            for (auto& Info : vecTopRegisterClassInfo)
            {
                for (; Info.uRef; --Info.uRef)
                {
                    auto TmpHr = Info.pClassInfo->UnRegister();
                    if (FAILED(TmpHr))
                    {
                        hr = TmpHr;
                        break;
                    }
                }
            }

            // 根据预期，Map应该是空的
            if (SUCCEEDED(hr) && g_ClassMap.empty() == false)
            {
                hr = E_FAIL;
            }

            return hr;
        }
    }
}

