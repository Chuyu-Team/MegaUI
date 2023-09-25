#pragma once
#include <Base/YY.h>
#include <Base/Exception.h>
#include <MegaUI/Base/ErrorCode.h>

namespace YY
{
    namespace Base
    {
        template<typename OutType, typename InType>
        bool SafeCast(_In_ const InType& _oIn, _Out_opt_ OutType* _pOut) noexcept;

        /// <summary>
        /// ��ȫ������ת�������ʧ�����׳��쳣��
        /// </summary>
        /// <param name="_oIn"></param>
        /// <returns></returns>
        template<typename OutType, typename InType>
        inline OutType SafeCast(_In_ const InType& _oIn) noexcept(false)
        {
            OutType _oOut;
            if (!SafeCast(_oIn, &_oOut))
            {
                throw Exception(_S("ָ������ת��ʧ�ܡ�"), E_INVALIDARG);
            }

            return _oOut;
        }

        template<>
        inline bool SafeCast(_In_ const int32_t& _oIn, _Out_opt_ uint8_t* _pOut) noexcept
        {
            if (_oIn > UINT8_MAX || _oIn < 0)
                return false;

            if (_pOut)
                *_pOut = (uint8_t)_oIn;
            return true;
        }

        template<>
        inline bool SafeCast(_In_ const uint32_t& _oIn, _Out_opt_ uint16_t* _pOut) noexcept
        {
            if (_oIn > UINT16_MAX || _oIn < 0)
                return false;

            if (_pOut)
                *_pOut = (uint16_t)_oIn;
            return true;
        }
    } // namespace Base
} // namespace YY
