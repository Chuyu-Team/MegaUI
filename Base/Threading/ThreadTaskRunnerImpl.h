#pragma once

#include <new>

#include <Base/Threading/TaskRunnerInterface.h>
#include <Base/Sync/Interlocked.h>

namespace YY
{
    namespace Base
    {
        namespace Threading
        {
            thread_local IThreadTaskRunner* g_pThreadTaskRunner;

            static constexpr auto uHandleMessageSuccess = 0x123;

            static uint32_t GenerateNewTaskRunnerId()
            {
                static uint32_t s_TaskRunnerId = 0;

                return Sync::Increment(&s_TaskRunnerId);
            }

            class ThreadTaskRunnerImpl : public IThreadTaskRunner
            {
            public:
                HWND hTaskRunnerWnd;
                uint32_t uTaskRunnerId;
                uint32_t uThreadId;
                uint32_t uRef;

                ThreadTaskRunnerImpl()
                    : hTaskRunnerWnd(NULL)
                    , uTaskRunnerId(0)
                    , uThreadId(0)
                    , uRef(1)
                {
                }

                ~ThreadTaskRunnerImpl()
                {
                    if (hTaskRunnerWnd)
                        DestroyWindow(hTaskRunnerWnd);
                }

                HRESULT __YYAPI Init()
                {
                    auto _hTaskRunnerWnd = CreateWindowExW(0, L"Message", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, nullptr);
                    if (!_hTaskRunnerWnd)
                    {
                        return E_FAIL;
                    }

                    SetWindowLongPtrW(_hTaskRunnerWnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);

                    hTaskRunnerWnd = _hTaskRunnerWnd;
                    uTaskRunnerId = GenerateNewTaskRunnerId();
                    uThreadId = GetCurrentThreadId();

                    return S_OK;
                }
                
                static
                LRESULT
                WINAPI
                WindowProc(
                    _In_ HWND _hWnd,
                    _In_ UINT _uMsg,
                    _In_ WPARAM _wParam,
                    _In_ LPARAM _lParam)
                {
                    if (_uMsg == WM_APP)
                    {
                        auto _pTaskRunnerCallback = (TaskRunnerCallback)_wParam;
                        _pTaskRunnerCallback((void*)_lParam);
                        return uHandleMessageSuccess;
                    }

                    return DefWindowProcW(_hWnd, _uMsg, _wParam, _lParam);
                }

                static IThreadTaskRunner* __YYAPI GetCurrentThreadTaskRunner()
                {
                    auto _pThreadTaskRunner = g_pThreadTaskRunner;
                    // 线程已经开始销毁，不再创建
                    if (_pThreadTaskRunner == (IThreadTaskRunner*)-1)
                        return nullptr;

                    if (_pThreadTaskRunner)
                    {
                        _pThreadTaskRunner->AddRef();
                        return _pThreadTaskRunner;
                    }

                    auto _pThreadTaskRunnerImpl = new (std::nothrow) ThreadTaskRunnerImpl();
                    if (!_pThreadTaskRunnerImpl)
                        return nullptr;

                    auto _hr = _pThreadTaskRunnerImpl->Init();
                    if (FAILED(_hr))
                    {
                        _pThreadTaskRunnerImpl->Release();
                        return nullptr;
                    }

                    return g_pThreadTaskRunner = _pThreadTaskRunnerImpl;
                }

                /////////////////////////////////////////////////////
                // ITaskRunner

                virtual uint32_t __YYAPI AddRef() override
                {
                    return Sync::Increment(&uRef);  
                }

                virtual uint32_t __YYAPI Release() override
                {
                    auto _uOldRef = Sync::Decrement(&uRef);
                    if (_uOldRef == 0)
                    {
                        auto _hr = Sync([](void* _pUserData)
                            {
                                if(g_pThreadTaskRunner == (IThreadTaskRunner*)_pUserData)
                                    g_pThreadTaskRunner = nullptr;
                                delete (ThreadTaskRunnerImpl*)_pUserData;
                            }, this);

                        if (FAILED(_hr))
                        {
                            delete this;
                        }
                    }

                    return _uOldRef;
                }

                virtual uint32_t __YYAPI GetId() override
                {
                    return uTaskRunnerId;
                }

                virtual TaskRunnerStyle __YYAPI GetStyle() override
                {
                    return TaskRunnerStyle::FixedThread | TaskRunnerStyle::Sequenced;
                }

                /// <summary>
                /// 将任务异步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Async(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) override
                {
                    if(!PostMessageW(hTaskRunnerWnd, WM_APP, (WPARAM)_pCallback, (LPARAM)_pUserData))
                    {
                        return __HRESULT_FROM_WIN32(GetLastError());
                    }

                    return S_OK;
                }

                /// <summary>
                /// 将任务同步执行。
                /// </summary>
                /// <param name="_pCallback"></param>
                /// <param name="_pUserData"></param>
                /// <returns></returns>
                virtual HRESULT __YYAPI Sync(_In_ TaskRunnerCallback _pCallback, _In_opt_ void* _pUserData) override
                {
                    auto _lResult = SendMessageW(hTaskRunnerWnd, WM_APP, (WPARAM)_pCallback, (LPARAM)_pUserData);
                    if (_lResult != uHandleMessageSuccess)
                        return E_FAIL;

                    return S_OK;
                }
                
                /////////////////////////////////////////////////////
                // IThreadTaskRunner

                virtual uint32_t __YYAPI GetThreadId() override
                {
                    return uThreadId;
                }
            };
        }
    } // namespace Base
} // namespace YY
