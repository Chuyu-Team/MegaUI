#pragma once
#include <YY/Base/YY.h>
#include <MegaUI/Core/Layout.h>

#pragma pack(push, __YY_PACKING)

namespace YY {
namespace MegaUI {
/// <summary>
/// 线性布局排列方向。
/// </summary>
enum class LinearLayoutOrientation
{
    Vertical = 0,
    Horizontal,
};

/// <summary>
/// 交叉轴对齐方式。与排列方向无关，统一使用 Start/Center/End/Stretch。
/// 垂直排列时：Start=左对齐, Center=居中, End=右对齐, Stretch=拉伸填满
/// 水平排列时：Start=顶对齐, Center=居中, End=底对齐, Stretch=拉伸填满
/// </summary>
enum class CrossAxisAlignment
{
    Start = 0,
    Center,
    End,
    Stretch,
};

/// <summary>
/// 线性布局，沿指定方向排列子元素，支持交叉轴对齐。
/// </summary>
class LinearLayout : public Layout
{
private:
    LinearLayoutOrientation eOrientation = LinearLayoutOrientation::Vertical;
    CrossAxisAlignment eCrossAlign = CrossAxisAlignment::Stretch;

public:
    constexpr LinearLayout() = default;

    /// <summary>
    /// 创建 LinearLayout 实例。
    /// </summary>
    /// <param name="_eOrientation">排列方向。</param>
    /// <param name="_eCrossAlign">交叉轴对齐方式。</param>
    /// <param name="_ppLayout">接收创建的 Layout 实例。</param>
    /// <returns>操作成功返回 S_OK。</returns>
    static HRESULT __YYAPI Create(
        _In_ LinearLayoutOrientation _eOrientation,
        _In_ CrossAxisAlignment _eCrossAlign,
        _Outptr_ Layout** _ppLayout);

    /// <summary>
    /// 获取排列方向。
    /// </summary>
    /// <returns>当前排列方向。</returns>
    LinearLayoutOrientation __YYAPI GetOrientation() const;

    /// <summary>
    /// 设置排列方向。
    /// </summary>
    /// <param name="_eOrientation">新的排列方向。</param>
    /// <returns>操作成功返回 S_OK。</returns>
    HRESULT __YYAPI SetOrientation(LinearLayoutOrientation _eOrientation);

    /// <summary>
    /// 获取交叉轴对齐方式。
    /// </summary>
    /// <returns>当前交叉轴对齐方式。</returns>
    CrossAxisAlignment __YYAPI GetCrossAxisAlignment() const;

    /// <summary>
    /// 设置交叉轴对齐方式。
    /// </summary>
    /// <param name="_eAlign">新的交叉轴对齐方式。</param>
    /// <returns>操作成功返回 S_OK。</returns>
    HRESULT __YYAPI SetCrossAxisAlignment(CrossAxisAlignment _eAlign);

private:
    // --- 重写 Layout 虚函数 ---

    /// <summary>
    /// 沿 Orientation 方向排列子元素，为每个子元素设置其在布局中的位置与尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_oSize">容器可用空间（已扣除容器自身的 Border/Padding）。</param>
    virtual void __YYAPI DoLayout(
        _In_ Element* pElem,
        Size _oSize) override;

    /// <summary>
    /// 计算线性布局的期望尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_ConstraintSize">容器可用空间约束（已扣除容器自身的 Border/Padding）。</param>
    /// <returns>所有参与排列子元素所需的总体尺寸。</returns>
    virtual Size __YYAPI UpdateDesiredSize(
        _In_ Element* pElem,
        Size _ConstraintSize) override;

    // --- 内部辅助 ---

    /// <summary>
    /// 计算垂直方向排列时的期望尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_ConstraintSize">容器可用空间约束。</param>
    /// <returns>所有参与排列子元素所需的总体尺寸。</returns>
    Size __YYAPI UpdateDesiredSizeVertical(
        _In_ Element* pElem,
        Size _ConstraintSize);

    /// <summary>
    /// 计算水平方向排列时的期望尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_ConstraintSize">容器可用空间约束。</param>
    /// <returns>所有参与排列子元素所需的总体尺寸。</returns>
    Size __YYAPI UpdateDesiredSizeHorizontal(
        _In_ Element* pElem,
        Size _ConstraintSize);

    /// <summary>
    /// 沿垂直方向排列子元素，为每个子元素设置其在布局中的位置与尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_oSize">容器可用空间。</param>
    void __YYAPI DoLayoutVertical(
        _In_ Element* pElem,
        Size _oSize);

    /// <summary>
    /// 沿水平方向排列子元素，为每个子元素设置其在布局中的位置与尺寸。
    /// </summary>
    /// <param name="pElem">布局容器元素。</param>
    /// <param name="_oSize">容器可用空间。</param>
    void __YYAPI DoLayoutHorizontal(
        _In_ Element* pElem,
        Size _oSize);
};
}
} // namespace YY

#pragma pack(pop)
