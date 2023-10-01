#pragma once
#include <Base/YY.h>

#include <Windows.h>

#include <Base/Strings/StringView.h>
#include <Base/Threading/TaskRunnerDispatchImpl.hpp>

#pragma pack(push, __YY_PACKING)

/*
DispatchTaskRunner ������������񣨱��綨ʱ�����첽IO�����޷���ִ������Task��

*/

namespace YY::Base::IO
{
    using namespace YY::Base::Threading;

    class AsyncFile
    {
    private:
        HANDLE hFile = INVALID_HANDLE_VALUE;

        AsyncFile(HANDLE _hFile = INVALID_HANDLE_VALUE)
            : hFile(_hFile)
        {
        }

    public:
        AsyncFile(AsyncFile&& _oOther)
            : hFile(_oOther.hFile)
        {
            _oOther.hFile = INVALID_HANDLE_VALUE;
        }

        ~AsyncFile()
        {
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
            }
        }

        static AsyncFile Open(const uchar_t* _szFilePath) noexcept
        {
            auto _hFile = CreateFileW(_szFilePath, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

            if (_hFile != INVALID_HANDLE_VALUE)
            {
                if (!TaskRunnerDispatchImplByIoCompletionImpl::Get()->BindIO(_hFile))
                {
                    CloseHandle(_hFile);
                    _hFile = INVALID_HANDLE_VALUE;
                }
            }

            return AsyncFile(_hFile);
        }

        auto __YYAPI AsyncRead(
            _In_ uint64_t _uOffset,
            _Out_ void* _pBuffer,
            _In_ uint32_t _cbToRead)
        {
            struct AsyncTaskEntry : public DispatchEntry
            {
                // 0��Э�̾����ʾ��δ���ã�-1��ʾ������ɡ�
                intptr_t hHandle;
                LSTATUS lStatus;

                AsyncTaskEntry(RefPtr<SequencedTaskRunner> _pResumeTaskRunner)
                    : hHandle(0u)
                    , lStatus(ERROR_IO_PENDING)
                {
                    pResumeTaskRunner = std::move(_pResumeTaskRunner);
                }

                void __YYAPI operator()() override
                {
                    lStatus = DosErrorFormNtStatus(Internal);
                    auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);
                    if (_hHandle)
                    {
                        std::coroutine_handle<>::from_address(_hHandle).resume();
                    }
                }
            };

            auto _pCurrent = YY::Base::Threading::SequencedTaskRunner::GetCurrent();
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create(std::move(_pCurrent));
            if (!_pAsyncTaskEntry)
                throw Exception();

            _pAsyncTaskEntry->Offset = (uint32_t)_uOffset;
            _pAsyncTaskEntry->OffsetHigh = (uint32_t)(_uOffset >> 32);

            if (ReadFile(hFile, _pBuffer, _cbToRead, nullptr, _pAsyncTaskEntry.Clone()))
            {
                // ��ȡ�ɹ�
                _pAsyncTaskEntry->operator()();
                _pAsyncTaskEntry->Release();
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // �����첽��ȡģʽ������һ�� Dispatch��IO��ɺ�Dispatch�Զ��Ὣ��������ת����������
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                }
                else
                {
                    // ʧ�ܣ�
                    _pAsyncTaskEntry->lStatus = _lStatus;
                    _pAsyncTaskEntry->hHandle = -1;
                    _pAsyncTaskEntry->Release();
                }
            }

            struct awaitable
            {
            private:
                RefPtr<AsyncTaskEntry> pAsyncTaskEntry;

            public:
                awaitable(RefPtr<AsyncTaskEntry> _pAsyncTaskEntry)
                    : pAsyncTaskEntry(std::move(_pAsyncTaskEntry))
                {
                }

                awaitable(awaitable&&) = default;

                bool await_ready() noexcept
                {
                    return pAsyncTaskEntry->hHandle == /*hReadyHandle*/ (intptr_t)-1;
                }

                bool await_suspend(std::coroutine_handle<> _hHandle) noexcept
                {
                    return YY::Base::Sync::CompareExchange(&pAsyncTaskEntry->hHandle, (intptr_t)_hHandle.address(), 0) == 0;
                }

                uint32_t await_resume() noexcept
                {
                    if (pAsyncTaskEntry->lStatus == ERROR_SUCCESS)
                    {
                        return pAsyncTaskEntry->InternalHigh;
                    }
                    else
                    {
                        SetLastError(pAsyncTaskEntry->lStatus);
                        return 0u;
                    }
                }
            };

            return awaitable(std::move(_pAsyncTaskEntry));
        }
    };
}
