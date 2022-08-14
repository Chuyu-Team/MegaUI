#pragma once
#include <exception>

#include "MegaUITypeInt.h"

#pragma pack(push, __MEGA_UI_PACKING)

namespace YY
{
    namespace MegaUI
    {
        class Exception : public std::exception
        {
        private:
            const uchar_t* szErrorMessage;
            HRESULT hr;
        public:
            /// <summary>
            /// 构造一个 Exception
            /// </summary>
            /// <param name="_szErrorMessage">必须指向一个常量或者有用足够声明周期的缓冲区，Exception不负责维护</param>
            /// <param name="_hr">错误代码</param>
            Exception(const uchar_t* _szErrorMessage = nullptr, HRESULT _hr = 0x8000FFFFL /* E_UNEXPECTED */)
                : szErrorMessage(_szErrorMessage)
                , hr(_hr)
            {
            }

            HRESULT __fastcall GetErrorCode()
            {
                return hr;
            }

            const uchar_t* __fastcall GetErrorMessage()
            {
                return szErrorMessage ? szErrorMessage : _S("");
            }
        };
    }
} // namespace YY

#pragma pack(pop)
