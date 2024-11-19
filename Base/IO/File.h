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
    protected:
        HANDLE hFile = INVALID_HANDLE_VALUE;
        bool bSkipCompletionNotificationOnSuccess = false;

        constexpr AsyncFile(HANDLE _hFile = INVALID_HANDLE_VALUE)
            : hFile(_hFile)
        {
            if (_hFile != INVALID_HANDLE_VALUE)
            {
                bSkipCompletionNotificationOnSuccess = SetFileCompletionNotificationModes(hFile, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE);
            }
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

        //template<typename LambdaCallback>
        //HRESULT __YYAPI AsyncRead(
        //    _In_ uint64_t _uOffset,
        //    _In_reads_bytes_(_cbBuffer) void* _pBuffer,
        //    _In_ uint32_t _cbBuffer,
        //    _In_ LambdaCallback&& _pfnResultCallback) noexcept
        //{
        //    struct AsyncReadLambda : public IoTaskEntry
        //    {
        //        LambdaCallback pfnLambdaCallback;

        //        void __YYAPI RunTask() override
        //        {
        //            lStatus = DosErrorFormNtStatus(Internal);

        //            pfnLambdaCallback(lStatus, lStatus == ERROR_SUCCESS ? InternalHigh : 0ul);
        //        }
        //    };
        //}

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
                    if (_pAsyncTaskEntry->OnComplete(ERROR_SUCCESS))
                    {
                        _pAsyncTaskEntry->operator()();
                    }

                    if (bSkipCompletionNotificationOnSuccess)
                    {
                        _pAsyncTaskEntry->Release();
                    }
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
                        if (_pAsyncTaskEntry->OnComplete(_lStatus))
                        {
                            _pAsyncTaskEntry->hHandle = -1;
                        }
                        _pAsyncTaskEntry->Release();
                    }
                }
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }
        
        auto __YYAPI AsyncWrite(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBuffer) const void* _pBuffer,
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
                    if (_pAsyncTaskEntry->OnComplete(ERROR_SUCCESS))
                    {
                        _pAsyncTaskEntry->operator()();
                    }

                    if (bSkipCompletionNotificationOnSuccess)
                    {
                        _pAsyncTaskEntry->Release();
                    }
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
                        if (_pAsyncTaskEntry->OnComplete(_lStatus))
                        {
                            _pAsyncTaskEntry->hHandle = -1;
                        }
                        _pAsyncTaskEntry->Release();
                    }
                }
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }

    protected:
        HRESULT __YYAPI AsyncReadInternal(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBuffer) void* _pBuffer,
            _In_ uint32_t _cbBuffer,
            _In_ RefPtr<IoTaskEntry> _pIoTask) noexcept
        {
            _pIoTask->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
            _pIoTask->Offset = (uint32_t)_uOffset;
            _pIoTask->OffsetHigh = (uint32_t)(_uOffset >> 32);

            if (ReadFile(hFile, _pBuffer, _cbBuffer, nullptr, _pIoTask.Clone()))
            {
                // 读取成功
                if (_pIoTask->OnComplete(ERROR_SUCCESS))
                {
                    _pIoTask->operator()();
                }

                if (bSkipCompletionNotificationOnSuccess)
                {
                    _pIoTask->Release();
                }
                return S_OK;
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // 进入异步读取模式，唤醒一下 Dispatch，IO完成后Dispatch自动会将任务重新转发到调用者
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                    return S_FALSE;
                }
                else
                {
                    // 失败！
                    _pIoTask->OnComplete(_lStatus);
                    _pIoTask->Release();
                    return __HRESULT_FROM_WIN32(_lStatus);
                }
            }
        }

        struct AsyncTaskEntry : public IoTaskEntry
        {
            // 0，协程句柄表示尚未设置，-1表示任务完成。
            intptr_t hHandle = 0u;

            HRESULT __YYAPI RunTask() override
            {
                auto _hHandle = (void*)YY::Base::Sync::Exchange(&hHandle, /*hReadyHandle*/ (intptr_t)-1);
                if (_hHandle)
                {
                    std::coroutine_handle<>::from_address(_hHandle).resume();
                }

                return S_OK;
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

    class AsyncPipe : public AsyncFile
    {
        constexpr AsyncPipe(HANDLE _hFile = INVALID_HANDLE_VALUE)
            : AsyncFile(_hFile)
        {
        }
    protected:
        struct ConnectAwaitable
        {
        private:
            RefPtr<AsyncTaskEntry> pAsyncTaskEntry;

        public:
            ConnectAwaitable(RefPtr<AsyncTaskEntry> _pAsyncTaskEntry)
                : pAsyncTaskEntry(std::move(_pAsyncTaskEntry))
            {
            }

            ConnectAwaitable(ConnectAwaitable&&) noexcept = default;

            bool await_ready() noexcept
            {
                return pAsyncTaskEntry == nullptr || pAsyncTaskEntry->hHandle == /*hReadyHandle*/ (intptr_t)-1;
            }

            bool await_suspend(std::coroutine_handle<> _hHandle) noexcept
            {
                return YY::Base::Sync::CompareExchange(&pAsyncTaskEntry->hHandle, (intptr_t)_hHandle.address(), 0) == 0;
            }

            LSTATUS await_resume() noexcept
            {
                return pAsyncTaskEntry ? pAsyncTaskEntry->lStatus : ERROR_OUTOFMEMORY;
            }
        };

    public:
        static AsyncPipe Create(
            _In_z_ const uchar_t* _szPipeName,
            _In_ DWORD _fOpenMode,
            _In_ DWORD _fPipeMode,
            _In_ DWORD _uMaxInstances,
            _In_ DWORD _cbOutBufferSize,
            _In_ DWORD _cbInBufferSize,
            _In_ DWORD _uDefaultTimeOut,
            _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr)
        {
            auto hPipe = CreateNamedPipeW(
                _szPipeName,
                _fOpenMode | FILE_FLAG_OVERLAPPED,
                _fPipeMode,
                _uMaxInstances,
                _cbOutBufferSize,
                _cbInBufferSize,
                _uDefaultTimeOut,
                nullptr);

            if (hPipe != INVALID_HANDLE_VALUE)
            {
                if (!TaskRunnerDispatchImplByIoCompletionImpl::Get()->BindIO(hPipe))
                {
                    CloseHandle(hPipe);
                    hPipe = INVALID_HANDLE_VALUE;
                }
            }

            return AsyncPipe(hPipe);
        }

        ConnectAwaitable AsyncConnect()
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                _pAsyncTaskEntry->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
                _pAsyncTaskEntry->Offset = 0;
                _pAsyncTaskEntry->OffsetHigh = 0;

                auto _bSuccess = ConnectNamedPipe(hFile, _pAsyncTaskEntry.Clone());
                if (_bSuccess)
                {
                    if (_pAsyncTaskEntry->OnComplete(ERROR_SUCCESS))
                    {
                        _pAsyncTaskEntry->operator()();
                    }

                    if (bSkipCompletionNotificationOnSuccess)
                    {
                        _pAsyncTaskEntry->Release();
                    }
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
                        if (_pAsyncTaskEntry->OnComplete(_lStatus))
                        {
                            _pAsyncTaskEntry->hHandle = -1;
                        }
                        _pAsyncTaskEntry->Release();
                    }
                }
            }
            return ConnectAwaitable(std::move(_pAsyncTaskEntry));
        }
    };
}

namespace YY
{
    using namespace YY::Base::IO;
}

#pragma pack(pop)
