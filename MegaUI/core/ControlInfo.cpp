#include "pch.h"
#include "ControlInfo.h"

#include <map>

#include <Base/Containers/Array.h>

#include <Base/Memory/Alloc.h>

__YY_IGNORE_INCONSISTENT_ANNOTATION_FOR_FUNCTION()

namespace YY
{
    namespace MegaUI
    {
        struct RegisterControlInfoLess
        {
            bool operator()(const u8StringView& _Left, const u8StringView& _Right) const
            {
                if (_Left.GetSize() == _Right.GetSize())
                {
                    return memcmp(_Left.GetConstString(), _Right.GetConstString(), _Left.GetSize() * sizeof(_Left[0])) < 0;
                }

                return _Left.GetSize() < _Right.GetSize();
            }
        };

        struct RegisterControlInfo
        {
            IControlInfo* pControlInfo;
            uint32_t uRef;
        };

        static std::map<u8StringView, RegisterControlInfo, RegisterControlInfoLess> g_ControlInfoMap;

        HRESULT __YYAPI IControlInfo::RegisterControlInternal(bool _bExplicitRegister)
        {
            std::pair<std::map<u8StringView, RegisterControlInfo>::iterator, bool> _itInsert;

            try
            {
                _itInsert = g_ControlInfoMap.insert(std::make_pair(u8StringView(GetName()), RegisterControlInfo {this}));
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
        
        IControlInfo* __YYAPI GetRegisterControlInfo(u8StringView _szControlName)
        {
            if (_szControlName.GetSize() == 0)
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

