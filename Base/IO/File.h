#pragma once
#include <functional>

#include <Windows.h>

#include <Base/YY.h>
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

        /// <summary>
        /// 异步读取文件。
        /// </summary>
        /// <param name="_uOffset">读取文件的偏移。</param>
        /// <param name="_pBuffer">输入缓冲区。请确保读取期间，_pBuffer处于有效状态。</param>
        /// <param name="_cbToRead">要读取的最大字节数。</param>
        /// <param name="_pfnResultCallback">异步完成后，执行的回调。回调保证恢复调用AsyncRead时的线程上下文。
        ///   _lStatus : 操作返回代码，如果返回 ERROR_SUCCESS，那么代表成功。
        ///   _cbRead : 实际读取成功的字节数。
        /// </param>
        /// <returns>
        /// ERROR_SUCCESS: 读取成功，但是数据尚未就绪，需要等待 _pfnResultCallback 完成。
        /// 其他值：失败。
        /// </returns>
        LSTATUS __YYAPI AsyncRead(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbToRead) void* _pBuffer,
            _In_ uint32_t _cbToRead,
            _In_ std::function<void(LSTATUS _lStatus, uint32_t _cbRead)> _pfnResultCallback) noexcept
        {
            struct AsyncReadTask : public IoTaskEntry
            {
                std::function<void(LSTATUS _lStatus, uint32_t _cbRead)> pfnResultCallback;

                HRESULT __YYAPI RunTask() override
                {
                    pfnResultCallback(lStatus, InternalHigh);
                }
            };

            auto _pAsyncTask = RefPtr<AsyncReadTask>::Create();
            if (!_pAsyncTask)
                return ERROR_OUTOFMEMORY;

            _pAsyncTask->pfnResultCallback = std::move(_pfnResultCallback);
            
            return AsyncReadInternal(_uOffset, _pBuffer, _cbToRead, std::move(_pAsyncTask));
        }

        /// <summary>
        /// 异步写入文件。
        /// </summary>
        /// <param name="_uOffset">写入文件的偏移。</param>
        /// <param name="_pBuffer">需要写入的数据缓冲区。</param>
        /// <param name="_cbToBuffer">要写入的字节数</param>
        /// <param name="_pfnResultCallback">异步完成后，执行的回调。回调保证恢复调用AsyncWrite时的线程上下文。
        ///   _lStatus : 操作返回代码，如果返回 ERROR_SUCCESS，那么代表成功。
        ///   _cbWrite : 实际写入成功的字节数。
        /// </param>
        /// <returns>
        /// ERROR_SUCCESS: 读取成功，但是数据尚未就绪，需要等待 _pfnResultCallback 完成。
        /// 其他值：失败。
        /// </returns>
        LSTATUS __YYAPI AsyncWrite(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBufferToWrite) const void* _pBuffer,
            _In_ uint32_t _cbBufferToWrite,
            _In_ std::function<void(LSTATUS _lStatus, uint32_t _cbWrite)> _pfnResultCallback) noexcept
        {
            struct AsyncWriteTask : public IoTaskEntry
            {
                std::function<void(LSTATUS _lStatus, uint32_t _cbRead)> pfnResultCallback;

                HRESULT __YYAPI RunTask() override
                {
                    pfnResultCallback(lStatus, InternalHigh);
                }
            };

            auto _pAsyncTask = RefPtr<AsyncWriteTask>::Create();
            if (!_pAsyncTask)
                return ERROR_OUTOFMEMORY;

            _pAsyncTask->pfnResultCallback = std::move(_pfnResultCallback);
            return AsyncWriteInternal(_uOffset, _pBuffer, _cbBufferToWrite, std::move(_pAsyncTask));
        }

#if defined(_HAS_CXX20) && _HAS_CXX20
        Awaitable __YYAPI AsyncRead(
            _In_ uint64_t _uOffset,
            _Out_writes_bytes_(_cbToRead) void* _pBuffer,
            _In_ uint32_t _cbToRead) noexcept
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                auto _lStatus = AsyncReadInternal(_uOffset, _pBuffer, _cbToRead, _pAsyncTaskEntry);
                if (_lStatus != ERROR_SUCCESS)
                {
                    // 失败时，异步任务不会在触发了。也算是一种任务完成。
                    _pAsyncTaskEntry->hHandle = -1;
                }
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }
#endif        

#if defined(_HAS_CXX20) && _HAS_CXX20
        Awaitable __YYAPI AsyncWrite(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBuffer) const void* _pBuffer,
            _In_ uint32_t _cbBuffer) noexcept
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                auto _lStatus = AsyncWriteInternal(_uOffset, _pBuffer, _cbBuffer, _pAsyncTaskEntry);
                if (_lStatus != ERROR_SUCCESS)
                {
                    // 失败时，异步任务不会在触发了。也算是一种任务完成。
                    _pAsyncTaskEntry->hHandle = -1;
                }
            }
            return Awaitable(std::move(_pAsyncTaskEntry));
        }
#endif

    protected:
        LSTATUS __YYAPI AsyncReadInternal(
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
                return ERROR_SUCCESS;
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // 进入异步读取模式，唤醒一下 Dispatch，IO完成后Dispatch自动会将任务重新转发到调用者
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                    return ERROR_SUCCESS;
                }
                else
                {
                    // 失败！
                    _pIoTask->OnComplete(_lStatus);
                    _pIoTask->Release();
                    return _lStatus;
                }
            }
        }

        LSTATUS __YYAPI AsyncWriteInternal(
            _In_ uint64_t _uOffset,
            _In_reads_bytes_(_cbBuffer) const void* _pBuffer,
            _In_ uint32_t _cbBuffer,
            _In_ RefPtr<IoTaskEntry> _pIoTask) noexcept
        {
            _pIoTask->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
            _pIoTask->Offset = (uint32_t)_uOffset;
            _pIoTask->OffsetHigh = (uint32_t)(_uOffset >> 32);
            
            if (WriteFile(hFile, _pBuffer, _cbBuffer, nullptr, _pIoTask.Clone()))
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
                return ERROR_SUCCESS;
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // 进入异步读取模式，唤醒一下 Dispatch，IO完成后Dispatch自动会将任务重新转发到调用者
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                    return ERROR_SUCCESS;
                }
                else
                {
                    // 失败！
                    _pIoTask->OnComplete(_lStatus);
                    _pIoTask->Release();
                    return _lStatus;
                }
            }
        }

#if defined(_HAS_CXX20) && _HAS_CXX20
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
                else
                {
                    SetLastError(pAsyncTaskEntry->lStatus);
                    return pAsyncTaskEntry->InternalHigh;
                }
            }
        };
#endif
    };

    class AsyncPipe : public AsyncFile
    {
        constexpr AsyncPipe(HANDLE _hFile = INVALID_HANDLE_VALUE)
            : AsyncFile(_hFile)
        {
        }

    protected:
        struct ConnectAwaitable;

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

        /// <summary>
        /// 异步链接管道。
        /// </summary>
        /// <param name="_pfnResultCallback">异步完成后，执行的回调。回调保证恢复调用AsyncConnect时的线程上下文。
        ///     _lStatus: 管道链接的错误代码，ERROR_SUCCESS代表成功。
        /// </param>
        /// <returns>
        /// ERROR_SUCCESS: 读取成功，但是数据尚未就绪，需要等待 _pfnResultCallback 完成。
        /// 其他值：失败。
        /// </returns>
        LSTATUS __YYAPI AsyncConnect(_In_ std::function<void(LSTATUS _lStatus)> _pfnResultCallback) noexcept
        {
            struct AsyncConnectTask : public IoTaskEntry
            {
                std::function<void(LSTATUS _lStatus)> pfnResultCallback;

                HRESULT __YYAPI RunTask() override
                {
                    pfnResultCallback(lStatus);
                }
            };

            auto _pAsyncTask = RefPtr<AsyncConnectTask>::Create();
            if (!_pAsyncTask)
                return ERROR_OUTOFMEMORY;

            _pAsyncTask->pfnResultCallback = std::move(_pfnResultCallback);
            return AsyncConnectIntetnal(std::move(_pAsyncTask));
        }

#if defined(_HAS_CXX20) && _HAS_CXX20
        ConnectAwaitable AsyncConnect()
        {
            auto _pAsyncTaskEntry = RefPtr<AsyncTaskEntry>::Create();
            if (_pAsyncTaskEntry)
            {
                auto _lStatus = AsyncConnectIntetnal(_pAsyncTaskEntry);
                if (_lStatus != ERROR_SUCCESS)
                {
                    _pAsyncTaskEntry->hHandle = -1;
                }
            }
            return ConnectAwaitable(std::move(_pAsyncTaskEntry));
        }
#endif
        
    protected:
        LSTATUS __YYAPI AsyncConnectIntetnal(RefPtr<IoTaskEntry> _pIoTask) noexcept
        {
            _pIoTask->pOwnerTaskRunnerWeak = TaskRunner::GetCurrent();
            auto _bSuccess = ConnectNamedPipe(hFile, _pIoTask.Clone());
            if (_bSuccess)
            {
                if (_pIoTask->OnComplete(ERROR_SUCCESS))
                {
                    _pIoTask->operator()();
                }

                if (bSkipCompletionNotificationOnSuccess)
                {
                    _pIoTask->Release();
                }
                return ERROR_SUCCESS;
            }
            else
            {
                const auto _lStatus = GetLastError();
                if (_lStatus == ERROR_IO_PENDING)
                {
                    // 进入异步读取模式，唤醒一下 Dispatch，IO完成后Dispatch自动会将任务重新转发到调用者
                    TaskRunnerDispatchImplByIoCompletionImpl::Get()->Weakup();
                    return ERROR_SUCCESS;
                }
                else
                {
                    // 失败！
                    _pIoTask->OnComplete(_lStatus);
                    _pIoTask->Release();
                    return _lStatus;
                }
            }
        }

#if defined(_HAS_CXX20) && _HAS_CXX20
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
#endif
    };
}

namespace YY
{
    using namespace YY::Base::IO;
}

#pragma pack(pop)
