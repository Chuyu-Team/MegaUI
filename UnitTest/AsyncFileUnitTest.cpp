#include "pch.h"
#include "CppUnitTest.h"
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <YY/Base/IO/File.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace YY;

namespace UnitTest
{
    TEST_CLASS(AsyncFile)
    {
    public:
        static Coroutine<void> ReadFileCoroutine()
        {
            wchar_t _szFilePath[512] = {};
            GetModuleFileNameW((HMODULE)&__ImageBase, _szFilePath, std::size(_szFilePath));

            auto _hFile = YY::AsyncFile::Open(_szFilePath, Access::Read, ShareMode::Read | ShareMode::Delete);
            auto _cbFile = GetFileSize(_hFile.GetNativeHandle(), nullptr);

            Assert::AreNotEqual(_cbFile, 0ul);

            std::string _szBuffer;
            _szBuffer.resize(_cbFile);
            co_await _hFile.AsyncRead(0, _szBuffer.data(), _cbFile);

            co_return;
        }

        TEST_METHOD(异步读取文件)
        {
            auto _pTaskRunner = SequencedTaskRunner::Create();

            wchar_t _szFilePath[512] = {};
            GetModuleFileNameW((HMODULE)&__ImageBase, _szFilePath, std::size(_szFilePath));
            auto _hFileSrc = CreateFileW(_szFilePath, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            const auto _cbFileSrc = GetFileSize(_hFileSrc, nullptr);
            Assert::AreNotEqual(_hFileSrc, INVALID_HANDLE_VALUE);

            std::string _szBufferSrc;
            _szBufferSrc.resize(_cbFileSrc);
            DWORD _cbRead = 0;
            ReadFile(_hFileSrc, _szBufferSrc.data(), _cbFileSrc, &_cbRead, nullptr);
            Assert::AreEqual(_cbFileSrc, _cbRead);

            auto _hFile = YY::AsyncFile::Open(_szFilePath, Access::Read, ShareMode::Read | ShareMode::Delete);

            std::string _szBufferAsyncReadFile;

            auto pfnReadFile = [&]() ->Coroutine<void>
                {
                    auto _cbFile = GetFileSize(_hFile.GetNativeHandle(), nullptr);
                    Assert::AreNotEqual(_cbFile, 0ul);

                    _szBufferAsyncReadFile.resize(_cbFile);
                    auto _cbRead = co_await _hFile.AsyncRead(0, _szBufferAsyncReadFile.data(), _cbFile);
                    _szBufferAsyncReadFile.resize(_cbRead);
                    co_return;
                };

            for (int i=2;i;--i)
            {
                _szBufferAsyncReadFile.clear();
                _pTaskRunner->PostTask([&]()
                    {
                        pfnReadFile();
                    });

                Sleep(500);

                Assert::AreEqual(_szBufferSrc, _szBufferAsyncReadFile);
            }
        }
    };
}
