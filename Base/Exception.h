#pragma once
#include <exception>

#include <Base/YY.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Base
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

            Exception(HRESULT _hr)
                : szErrorMessage(nullptr)
                , hr(_hr)
            {
            }

            HRESULT __YYAPI GetErrorCode()
            {
                return hr;
            }

            const uchar_t* __YYAPI GetErrorMessage()
            {
                return szErrorMessage ? szErrorMessage : _S("");
            }
        };
    } // namespace Base
} // namespace YY

#pragma pack(pop)
