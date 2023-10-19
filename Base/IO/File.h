#pragma once
#include <Base/YY.h>

#include <Windows.h>

#include <Base/Strings/StringView.h>
#include <Base/Threading/TaskRunnerDispatchImpl.h>

#pragma pack(push, __YY_PACKING)

/*
DispatchTaskRunner 仅处理调度任务（比如定时器、异步IO），无法参执行其他Task。

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
            struct AsyncTaskEntry : public IoTaskEntry
            {
                // 0，协程句柄表示尚未设置，-1表示任务完成。
                intptr_t hHandle;
                LSTATUS lStatus;

                AsyncTaskEntry()
                    : hHandle(0u)
                    , lStatus(ERROR_IO_PENDING)
                {
                }

                void __YYAPI RunTask() override
                {
                    lStatus = DosErrorFormNtStatus(Internal);
                    auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);
                    if (_hHandle)
                    {
                        std::coroutine_handle<>::from_address(_hHandle).resume();
                    }
                }
            };

            auto _pCurrent = YY::Base::Threading::TaskRunner::GetCurrent();
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (!_pAsyncTaskEntry)
                throw Exception();

            _pAsyncTaskEntry->pOwnerTaskRunnerWeak = _pCurrent;
            _pAsyncTaskEntry->Offset = (uint32_t)_uOffset;
            _pAsyncTaskEntry->OffsetHigh = (uint32_t)(_uOffset >> 32);

            if (ReadFile(hFile, _pBuffer, _cbToRead, nullptr, _pAsyncTaskEntry.Clone()))
            {
                // 读取成功
                _pAsyncTaskEntry->operator()();
                _pAsyncTaskEntry->Release();
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // 进入异步读取模式，唤醒一下 Dispatch，IO完成后Dispatch自动会将任务重新转发到调用者
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                }
                else
                {
                    // 失败！
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

#pragma pack(pop)
