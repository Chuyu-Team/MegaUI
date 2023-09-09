#pragma once
#include <Windows.h>
#include <atlcomcli.h>
#include <GdiPlus.h>

#include <Base/YY.h>
#include <Base/Sync/Interlocked.h>

#pragma pack(push, __YY_PACKING)

namespace YY
{
    namespace Media
    {
        namespace Graphics
        {
            class AutoGdiplusStartup
            {
            private:
                ULONG_PTR uGdiplusToken;

            public:
                AutoGdiplusStartup()
                    : uGdiplusToken(0)
                {
                }

                ~AutoGdiplusStartup()
                {
                    if (uGdiplusToken)
                    {
                        Gdiplus::GdiplusShutdown(uGdiplusToken);
                    }
                }

                Gdiplus::Status TryGdiplusStartup()
                {
                    if (uGdiplusToken)
                        return Gdiplus::Status::Ok;

                    Gdiplus::GdiplusStartupInput _oGdiplusStartupInput;
                    return Gdiplus::GdiplusStartup(&uGdiplusToken, &_oGdiplusStartupInput, NULL);
                }
            };

            // 此类用于减少一次Gdi+的堆内存分配
            template<typename Type>
            class GdiplusStatic;

            template<>
            class GdiplusStatic<Gdiplus::Bitmap> : public Gdiplus::Bitmap
            {
                using Type = Gdiplus::Bitmap;
            public:
                template<typename... Args>
                GdiplusStatic(const Args&... _oArgs)
                    : Type(_oArgs...)
                {
                }

                GdiplusStatic()
                    : Type((Gdiplus::GpBitmap*)nullptr)
                {
                }

                GdiplusStatic(GdiplusStatic&& _oOther) noexcept
                    : GdiplusStatic()
                {
                    operator=(std::move(_oOther));
                }

                GdiplusStatic(const GdiplusStatic& _oOther) = delete;

                bool IsNull() const
                {
                    return nativeImage == nullptr;
                }

                template<typename... Args>
                void Reset(const Args&... _oArgs)
                {
                    this->~GdiplusStatic();

                    ::new (this) GdiplusStatic(_oArgs...);
                }
                
                GdiplusStatic& operator=(GdiplusStatic&& _oOther) noexcept
                {
                    nativeImage = _oOther.nativeImage;
                    _oOther.nativeImage = nullptr;
                    return *this;
                }
            };

            template<>
            class GdiplusStatic<Gdiplus::Graphics> : public Gdiplus::Graphics
            {
                using Type = Gdiplus::Graphics;

            public:
                template<typename... Args>
                GdiplusStatic(const Args&... _oArgs)
                    : Type(_oArgs...)
                {
                }

                GdiplusStatic()
                    : Type((Gdiplus::GpGraphics*)nullptr)
                {
                }

                GdiplusStatic(GdiplusStatic&& _oOther) noexcept
                    : GdiplusStatic()
                {
                    operator=(std::move(_oOther));
                }

                GdiplusStatic(const GdiplusStatic& _oOther) = delete;

                bool IsNull() const
                {
                    return nativeGraphics == nullptr;
                }
                
                template<typename... Args>
                void Reset(const Args&... _oArgs)
                {
                    this->~GdiplusStatic();

                    ::new (this) GdiplusStatic(_oArgs...);
                }

                GdiplusStatic& operator=(GdiplusStatic&& _oOther) noexcept
                {
                    nativeGraphics = _oOther.nativeGraphics;
                    _oOther.nativeGraphics = nullptr;
                    return *this;
                }
            };

            // 此类可以将Gdi+的对象支持带引用计数。便于统一的内存管理。
            template<typename Type>
            class GdiplusRef
                : public Type
            {
            private:
                uint32_t uRef;

            public:
                template<typename... Args>
                GdiplusRef(const Args&... _oArgs)
                    : Type(_oArgs...)
                    , uRef(1)
                {
                }

                uint32_t __YYAPI AddRef()
                {
                    return Sync::Increment(&uRef);
                }

                uint32_t __YYAPI Release()
                {
                    auto _uOldRef = Sync::Decrement(&uRef);
                    if (_uOldRef == 0)
                    {
                        delete this;
                    }

                    return _uOldRef;
                }
            };
        }
    } // namespace Media
} // namespace YY

#pragma pack(pop)
