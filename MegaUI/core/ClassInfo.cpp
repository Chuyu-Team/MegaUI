#include "pch.h"

#include "ClassInfo.h"

#include <unordered_map>

namespace YY
{
    namespace MegaUI
    {
        struct hash_raw_const_string
        {
            size_t operator()(raw_const_string _Keyval) const noexcept
            {
                return std::_Hash_array_representation(_Keyval, strlen(_Keyval));
            }
        };

        struct equal_to_raw_const_string
        {
            bool operator()(raw_const_string _Left, raw_const_string _Right) const
            {
                return strcmp(_Left, _Right) == 0;
            }
        };


        struct RegisterClassInfo
        {
            IClassInfo* pClassInfo;
            uint32_t uRef;
        };

        static std::unordered_map<raw_const_string, RegisterClassInfo, hash_raw_const_string, equal_to_raw_const_string> g_ClassMap;



        HRESULT __fastcall IClassInfo::_RegisterClass(bool bExplicitRegister)
        {
            auto szClassName = GetClassName();

            auto InsertIter = g_ClassMap.insert(std::make_pair(szClassName, RegisterClassInfo{ this }));


            auto& RegisterInfo = InsertIter.first->second;

            if (!InsertIter.second)
            {
                // Class name相同，但是不是一个 ClassInfo，这种情况我们应该返回错误
                if (RegisterInfo.pClassInfo != this)
                    return E_ABORT;
            }

            if(bExplicitRegister)
                ++RegisterInfo.uRef;

            return S_OK;
        }
        
        HRESULT __fastcall IClassInfo::_UnregisterClass(bool bExplicitRegister)
        {
            auto szClassName = GetClassName();

            auto IterItem = g_ClassMap.find(szClassName);

            if (IterItem == g_ClassMap.end())
                return S_FALSE;

            if (bExplicitRegister)
            {
                auto& RegisterInfo = IterItem->second;

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
    }
}

