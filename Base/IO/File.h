#pragma once
#include <Base/YY.h>

#include <Windows.h>

#include <Base/Strings/StringView.h>
#include <Base/Threading/TaskRunnerDispatchImpl.h>

#pragma pack(push, __YY_PACKING)

namespace YY::Base::IO
{
    using namespace YY::Base::Threading;

    enum class ShareMode : uint32_t
    {
        None = 0u,
        Delete = FILE_SHARE_DELETE,
        Read = FILE_SHARE_READ,
        Write = FILE_SHARE_WRITE,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(ShareMode);

    enum class Access : uint32_t
    {
        None = 0u,
        Read = GENERIC_READ,
        Write = GENERIC_WRITE,
        Execute = GENERIC_EXECUTE,
        MaximumAllowed = MAXIMUM_ALLOWED,
    };

    YY_APPLY_ENUM_CALSS_BIT_OPERATOR(Access);

    class AsyncFile
    {
    private:
        HANDLE hFile = INVALID_HANDLE_VALUE;

        AsyncFile(HANDLE _hFile = INVALID_HANDLE_VALUE)
            : hFile(_hFile)
        {
        }

        struct Awaitable;
        struct AsyncTaskEntry;

    public:
        AsyncFile(AsyncFile&& _oOther) noexcept
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

        HANDLE __YYAPI GetNativeHandle() const noexcept
        {
            return hFile;
        }

        static AsyncFile __YYAPI Open(_In_z_ const uchar_t* _szFilePath, _In_ Access _eAccess, _In_ ShareMode _eShareMode = ShareMode::None) noexcept
        {
            auto _hFile = CreateFileW(_szFilePath, static_cast<DWORD>(_eAccess), static_cast<DWORD>(_eShareMode), nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

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
            _Out_writes_bytes_(_cbToRead) void* _pBuffer,
            _In_ uint32_t _cbToRead) noexcept
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                _pAsyncTaskEntry->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
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
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }
        
        auto __YYAPI AsyncWrite(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBuffer) void* _pBuffer,
            _In_ uint32_t _cbBuffer) noexcept
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                _pAsyncTaskEntry->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
                _pAsyncTaskEntry->Offset = (uint32_t)_uOffset;
                _pAsyncTaskEntry->OffsetHigh = (uint32_t)(_uOffset >> 32);

                if (WriteFile(hFile, _pBuffer, _cbBuffer, nullptr, _pAsyncTaskEntry.Clone()))
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
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }

    private:
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

        struct Awaitable
        {
        private:
            RefPtr<AsyncTaskEntry> pAsyncTaskEntry;

        public:
            Awaitable(RefPtr<AsyncTaskEntry> _pAsyncTaskEntry)
                : pAsyncTaskEntry(std::move(_pAsyncTaskEntry))
            {
            }

            Awaitable(Awaitable&&) noexcept = default;

            bool await_ready() noexcept
            {
                return pAsyncTaskEntry == nullptr || pAsyncTaskEntry->hHandle == /*hReadyHandle*/ (intptr_t)-1;
            }

            bool await_suspend(std::coroutine_handle<> _hHandle) noexcept
            {
                return YY::Base::Sync::CompareExchange(&pAsyncTaskEntry->hHandle, (intptr_t)_hHandle.address(), 0) == 0;
            }

            uint32_t await_resume() noexcept
            {
                if (pAsyncTaskEntry == nullptr)
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    return 0u;
                }
                else if (pAsyncTaskEntry->lStatus == ERROR_SUCCESS)
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

    };
}

namespace YY
{
    using namespace YY::Base::IO;
}

#pragma pack(pop)
