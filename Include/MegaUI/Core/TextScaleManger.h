#pragma once
#include <Base/YY.h>
#include <functional>

namespace YY
{
    namespace MegaUI
    {
        /// <summary>
        /// 返回系统的文本缩放因子。理论上这个值是1.0，但是用户可以通过系统设置 - 文本缩放进行调整。
        /// </summary>
        /// <returns></returns>
        float __YYAPI GetSystemTextScale() noexcept;

        /// <summary>
        /// 注册文本缩放因子变化的回调。
        /// </summary>
        /// <param name="_pfnTextScaleFactorChanged"></param>
        /// <returns>
        /// 返回值Cookie，用于可以使用RemoveTextScaleFactorChanged移除监听操作。
        /// 如果返回null，则函数失败。
        /// 如果返回非null，则函数成功。 
        /// </returns>
        void* __YYAPI AddTextScaleFactorChanged(std::function<void(float)> _pfnTextScaleFactorChanged) noexcept;
        
        /// <summary>
        /// 移除之前AddTextScaleFactorChanged注册的回调通知。
        /// </summary>
        /// <param name="_pCookie">AddTextScaleFactorChanged返回的Cookie值。</param>
        /// <returns></returns>
        void __YYAPI RemoveTextScaleFactorChanged(_In_opt_ void* _pCookie) noexcept;
    }
} // namespace YY
