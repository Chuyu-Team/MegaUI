# LinearLayout Specs

> 版本：v2.0
> 状态：草稿
> 最后更新：2026-07-07 17:55
> 关联 PRD：docs/linear-layout/prd/PRD-linear-layout-v1.0.md

> v2.0 变更（相对 v1.0）：
> - Layout::UpdateDesiredSize 签名改为 `Size _ConstraintSize`
> - LinearLayout 布局计数直接复用基类 `Layout::GetLayoutChildCount`，LP_None/LP_Absolute 过滤在迭代内联
> - UIParser 采用通用工厂接口 `ParserLayout(ExprNode*, Layout** ppLayout)`，各布局自行解析参数
> - 明确 Margin Cmp 由框架自动计算（`_COMPUTED_IS_RECT=1`）
> - 布局工厂注册表（`LayoutFactoryEntry` / `g_LayoutFactoryTable`）移至 `Layout.h` / `Layout.cpp`
> - 所有函数调用约定统一使用 `__YYAPI`，而非混用 `__fastcall`
> - Element::UpdateDesiredSize 启用 Layout 路径（当前被 `#if 0` 注释），DoLayout 用 `GetDesiredSize()` 替代 `CmpWidth`
> - Layout 工厂注册改为 Layout 基类静态方法（`LookupFactory`），各布局在启动时注册

## 1. 总体设计概览

### 1.1 目标
- 实现 `LinearLayout` 继承自 `Layout`，支持子元素沿 `Orientation` 方向（垂直/水平）线性排列
- 新增 `Margin` 属性到 `Element`
- 完善 `UIParser` 对 `ValueType::Layout` 的解析支持：
  - 新增 `ParserValue()` 中 `case ValueType::Layout` 分支
  - 建立通用工厂接口 `ParserLayout(ExprNode*, Layout** ppLayout)`，各布局自行解析参数

### 1.2 设计原则
- 最小侵入：不修改 `Layout` 基类虚函数签名，不修改 UIParser 已有 `ParsedArgType` 体系
- 工厂解耦：布局字符串→实例的映射通过静态注册表 + 通用函数指针完成
- 参照 Padding：Margin 属性在 tri-level、cascade、bind 上完全参照 `Padding`

### 1.3 非目标
- 不实现换行/折行（Wrap）功能

---

## 2. 模块划分与职责

| 模块 | 位置 | 职责 |
|------|------|------|
| `LinearLayout` | `Include/MegaUI/Core/LinearLayout.h` + `MegaUI/Core/LinearLayout.cpp` | 继承 `Layout`，实现垂直/水平线性排列 |
| `Margin` 属性 | `Element` 成员 + 属性表 | Element 级外边距，类型 `UnitRect`，默认 0 |
| `Layout` 基类 | `Layout.h` + `Layout.cpp` | 虚函数基类，不新增任何成员 |
| UIParser 适配 | `UIParser` | 新增 `ParserLayoutValue()` + `case ValueType::Layout` |

---

## 3. 数据结构与关键类型

### 3.1 Orientation 枚举

```cpp
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
```

### 3.2 Margin 属性 - Element 成员变更

在 `Element` 类中参照 `SpecPadding`/`CmpPadding` 模式新增：

```cpp
UnitRect SpecMargin;   // Specified 值
Rect CmpMargin;        // Computed 像素值（由框架自动计算）
```

属性绑定：
```
_APPLY(Margin, PF_TriLevel | PF_Cascade,
    PG_AffectsDesiredSize | PG_AffectsLayout | PG_AffectsParentLayout,
    &Value::CreateRectZero, nullptr,
    nullptr, nullptr,
    _MEGA_UI_PROP_BIND_RECT_EX(0, 0,
        YY_UFIELD_OFFSET(Element, SpecMargin), 0,
        YY_UFIELD_OFFSET(Element, CmpMargin), 1),
    ValueType::UnitRect)
```

> **重点**：`_COMPUTED_IS_RECT=1` 表示 Computed 层存储的是像素 `Rect`（非 `UnitRect`），由 Element 框架在 DPI/TextScale 变化时自动从 `SpecMargin`（UnitRect）重新计算。`LinearLayout` 直接读取 `CmpMargin` 即可。

### 3.3 布局解析（UIParser 内部闭环）

布局解析完全在 `UIParser.cpp` 内部实现，Core 层（Layout/Element）无任何变更：

```cpp
// 以下均在 UIParser.cpp 文件作用域定义，不对外暴露

// 通用解析函数签名
typedef HRESULT(__YYAPI* LayoutParserFunc)(
    _In_ ExprNode* _pExprNode,
    _Outptr_ Layout** _ppLayout);

// 注册表条目
struct LayoutFactoryEntry
{
    const u8char_t* pszName;
    LayoutParserFunc pfnParser;
};

// 注册表
static const LayoutFactoryEntry g_LayoutFactoryTable[] =
{
    { u8"LinearLayout", &ParserLinearLayout },
};

// 查找函数
_Ret_maybenull_ static LayoutParserFunc __fastcall LookupLayoutParser(
    _In_z_ const u8char_t* _pszName)
{
    for (auto& _Entry : g_LayoutFactoryTable)
        if (u8Stricmp(_Entry.pszName, _pszName) == 0)
            return _Entry.pfnParser;
    return nullptr;
}
```

---

## 4. API 设计

### 4.1 LinearLayout 类

```cpp
class LinearLayout : public Layout
{
public:
    LinearLayout();
    virtual ~LinearLayout() override;

    // 直接工厂（代码中使用）
    static HRESULT __YYAPI Create(
        _In_ LinearLayoutOrientation _eOrientation,
        _In_ CrossAxisAlignment _eCrossAlign,
        _Outptr_ Layout** _ppLayout);

    // 仅 Create 工厂需要，ParserLinearLayout 在 UIParser.cpp 中实现
    // LinearLayout 类本身不包含解析逻辑

    // 属性访问（只读，方向与对齐方式由构造时确定）
    LinearLayoutOrientation __YYAPI GetOrientation() const;
    CrossAxisAlignment __YYAPI GetCrossAxisAlignment() const;

private:
    LinearLayoutOrientation eOrientation = LinearLayoutOrientation::Vertical;
    CrossAxisAlignment eCrossAlign = CrossAxisAlignment::Stretch;

    // --- 仅重写核心虚函数，其余复用 Layout 基类默认实现 ---

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
```

> 方向与交叉轴对齐方式由 `Create()` 构造时确定，构造后不可变更。

### 4.2

`ParserLinearLayout` 是 `UIParser.cpp` 的文件作用域静态函数，通过 `g_LayoutFactoryTable` 注册。

```cpp
// 在 UIParser.cpp 中定义
static HRESULT __YYAPI ParserLinearLayout(
    _In_ ExprNode* _pExprNode,
    _Outptr_ Layout** _ppLayout)
{
    static constexpr const EnumMap s_OrientationEnumMap[] =
    {
        { u8"Vertical", 0 }, { u8"Horizontal", 1 }, { },
    };
    static constexpr const EnumMap s_CrossAxisAlignmentEnumMap[] =
    {
        { u8"Start", 0 }, { u8"Center", 1 }, { u8"End", 2 }, { u8"Stretch", 3 }, { },
    };

    ParsedArg _Args[] =
    {
        { ParsedArgType::int32_t, s_OrientationEnumMap },
        { ParsedArgType::int32_t, s_CrossAxisAlignmentEnumMap },
    };
    _Args[0].iNumber = static_cast<int32_t>(LinearLayoutOrientation::Vertical);
    _Args[1].iNumber = static_cast<int32_t>(CrossAxisAlignment::Stretch);

    HRESULT hr = S_OK;
    if (_pExprNode->Type == ExprNodeType::Funcall)
    {
        auto _cChildren = _pExprNode->ChildExprNode.GetSize();
        if (_cChildren > _countof(_Args))
            return HRESULT_From_LSTATUS(ERROR_BAD_FORMAT);
        hr = ParserFunction(_pExprNode->szValue, _pExprNode, _Args, _cChildren);
        if (FAILED(hr))
            return hr;
    }

    return LinearLayout::Create(
        static_cast<LinearLayoutOrientation>(_Args[0].iNumber),
        static_cast<CrossAxisAlignment>(_Args[1].iNumber),
        _ppLayout);
}
```

### 4.3 Element Margin Getter/Setter

```cpp
/// <summary>
/// 获取元素外边距。
/// </summary>
/// <returns>元素当前计算后的外边距（像素值）。</returns>
UnitRect __YYAPI Element::GetMargin();

/// <summary>
/// 设置元素外边距。
/// </summary>
/// <param name="_Margin">新的外边距值，类型为 UnitRect，支持像素/dp/sp 等单位。</param>
/// <returns>操作成功返回 S_OK。</returns>
HRESULT __YYAPI Element::SetMargin(_In_ const UnitRect& _Margin);
```

### 4.5 UIParser::ParserValue() 新增分支

```cpp
case ValueType::Layout:
    _hr = ParserLayoutValue(&_ExprNode, &_Value);
    break;
```

### 4.6 UIParser::ParserLayoutValue() 实现

```
输入：ExprNode 树（如 "LinearLayout(Horizontal)" 或 "LinearLayout"）
输出：包装好的 Value

1. 从 ExprNode 提取布局名称：
   - 若为 BaseIdentifier：名称即 szValue（如 u8"LinearLayout"）
   - 若为 Funcall：名称即 Funcall 自身的 szValue（如 u8"LinearLayout"）
2. 调用内部 `LookupLayoutParser(pszName)` 查找解析函数
   - 若返回 nullptr，报 ERROR_BAD_FORMAT
3. 调用 pfnParser(pExprNode, &pLayout)
4. 将 pLayout 包装为 Value：*_pValue = Value::CreateLayout(pLayout)
```

### 4.7 Value::CreateLayout 新增

```cpp
/// <summary>
/// 创建一个 Layout 类型的 Value，包装指定布局实例。
/// </summary>
/// <param name="_pLayout">布局实例指针，可为 nullptr（表示空布局）。</param>
/// <returns>包装后的 Value 对象。</returns>
static Value __YYAPI Value::CreateLayout(_In_opt_ Layout* _pLayout);
```

### 4.8 Element::GetDesiredSize 新增（便捷访问）

```cpp
/// <summary>
/// 获取元素当前计算后的期望尺寸。
/// </summary>
/// <returns>期望尺寸（像素值）。</returns>
Size __YYAPI Element::GetDesiredSize() const
{
    return LocDesiredSize;
}
```

> 说明：`LocDesiredSize` 是 Element 的 `DesiredSize` 属性本地缓存，由 `UpdateDesiredSize()` 计算并写入。
> Layout 在 `DoLayout` 中通过此方法获取子元素已计算好的期望尺寸，避免重复调用 `UpdateDesiredSize`。

### 4.9 Element::UpdateDesiredSize 启用 Layout 路径

当前 `Element::UpdateDesiredSize()` 中 Layout 调用被 `#if 0` 注释，必须启用：

```cpp
// 修改前（MegaUI/Core/Element.cpp 约 3130 行）
#if 0
if (pLayout)
{
    TmpSize = pLayout->UpdateDesiredSize(this, x, y);
}
else
#endif
{
    _ContentSize = GetContentSize(_ConstraintContentSize);
}

// 修改后
if (bSelfLayout)
{
    _ContentSize = SelfLayoutUpdateDesiredSize(_ConstraintContentSize);
}
else
{
    auto _Layout = GetLayout();
    if (_Layout.HasValue() && _Layout.GetValue())
    {
        _ContentSize = _Layout.GetValue()->UpdateDesiredSize(this, _ConstraintContentSize);
    }
    else
    {
        _ContentSize = GetContentSize(_ConstraintContentSize);
    }
}
```

> 注意：Layout::UpdateDesiredSize 签名为 `Size(Element*, Size)`，传入的 `_ConstraintContentSize` 已扣除 Border/Padding。

---

## 5. 核心流程

### 5.1 子元素参与判断（内联迭代过滤）

在 `UpdateDesiredSize` / `DoLayout` 中遍历子元素时：

```
for each pChild in pElem->GetChildren():
    iLayoutPos = pChild->GetLayoutPos()
    if iLayoutPos == LP_None:     // 跳过不占位
        continue
    if iLayoutPos == LP_Absolute: // 跳过绝对定位
        continue
    // 参与排列（LP_Auto 或 >=0 的自定义值）
```

> 说明：`Layout::GetLayoutChildCount(pElem)` 可用于获取基础子元素计数（扣除 IgnoreList），但 LP_Absolute/LP_None 的过滤直接在迭代中判断即可，无需单独封装方法。

### 5.2 Margin 间距计算

```
Vertical：spacing = max(prev.CmpMargin.Bottom, curr.CmpMargin.Top)
Horizontal：spacing = max(prev.CmpMargin.Right, curr.CmpMargin.Left)

首元素：起始偏移 = 其 Margin.Top（垂直）或 Margin.Left（水平）
末元素：DesiredSize 中包含其 Margin.Bottom（垂直）或 Margin.Right（水平）
```

### 5.3 UpdateDesiredSize - 垂直模式

```
输入：_ConstraintSize（容器可用空间）
输出：Size(widthDesired, heightDesired)

1. widthDesired = 0, heightDesired = 0
2. 遍历子元素：
   a. 跳过 LP_None / LP_Absolute
   b. childDesired = pChild->UpdateDesiredSize(Size(_ConstraintSize.Width, -1))
   c. 横向：widthDesired = max(widthDesired,
        childDesired.Width + pChild.CmpMargin.Left + pChild.CmpMargin.Right)
   d. 纵向：若为首个 → heightDesired += pChild.CmpMargin.Top
            若为非首个 → heightDesired += max(prev.CmpMargin.Bottom, pChild.CmpMargin.Top)
            heightDesired += childDesired.Height
3. 加上末元素 Bottom margin
4. return Size(widthDesired, heightDesired)
```

### 5.4 UpdateDesiredSize - 水平模式

与垂直对称，主轴为宽度，交叉轴为高度。

### 5.5 DoLayout - 垂直模式

```
输入：pElem, _oSize（容器可用空间，已扣除容器自身的 Border/Padding）
行为：为每个参与排列的子元素设置 PosInLayout 和 SizeInLayout

1. yCurr = 0
2. 遍历子元素（跳过 LP_None/LP_Absolute）：
   a. margin = pChild->CmpMargin
   b. 首元素: yCurr += margin.Top; 否则: yCurr += max(prevMargin.Bottom, margin.Top)
   c. childDesired = pChild->GetDesiredSize()  // DoLayout 阶段必定有效
      childW = childDesired.Width
      childH = childDesired.Height
   d. 交叉轴对齐（switch eCrossAlign）：
      Stretch: childX = margin.Left
               childW = _oSize.Width - margin.Left - margin.Right
               childW = max(childW, 0)
      Start:   childX = margin.Left
      Center:  childX = (_oSize.Width - childW) / 2
               if childX < margin.Left: childX = margin.Left
               if childX > _oSize.Width - margin.Right - childW: childX = _oSize.Width - margin.Right - childW
      End:     childX = _oSize.Width - childW - margin.Right
   e. UpdateLayoutPosition(Point(childX, yCurr))
   f. UpdateLayoutSize(Size(childW, childH))
   g. yCurr += childH
   h. prevMargin = margin
```

> 说明：Element 生命周期保证 `FlushDesiredSize` 先于 `FlushLayout`，因此 `DoLayout` 阶段 `GetDesiredSize()` 必定有效。
> `UpdateDesiredSize` 阶段交叉轴宽度取子元素自然宽度，Stretch 的拉伸由 `DoLayout` 处理。

### 5.6 DoLayout - 水平模式

```
与垂直对称，主轴为宽度，交叉轴为高度。
交叉轴对齐（switch eCrossAlign）：
  Stretch: childY = margin.Top; childH = _oSize.Height - margin.Top - margin.Bottom
            childH = max(childH, 0)
  Start:   childY = margin.Top
  Center:  childY = (_oSize.Height - childH) / 2
           if childY < margin.Top: childY = margin.Top
           if childY > _oSize.Height - margin.Bottom - childH: childY = _oSize.Height - margin.Bottom - childH
  End:     childY = _oSize.Height - childH - margin.Bottom
```

### 5.7 缓存失效

```
Element 框架在子元素增删（Insert/Remove）或 LayoutPos 变更时，自动通过 MarkElementForDesiredSize / MarkElementForLayout 触发重新计算。
LinearLayout 无需重写 OnAdd/OnRemove/OnLayoutPosChanged/Attach/Detach，复用 Layout 基类默认实现即可。
```

---

## 6. 并发与线程模型

- 布局计算在 `FlushLayout` 调用链中同步执行，与现有 Element 系统一致
- 不引入额外线程和锁
- Margin 属性的 Get/Set 走现有 DeferCycle 机制

---

## 7. 错误处理与日志策略

| 场景 | 处理 |
|------|------|
| `Layout::Create()` 失败 | `E_OUTOFMEMORY` |
| 工厂查不到布局名称 | `HRESULT_From_LSTATUS(ERROR_BAD_FORMAT)` |
| 容器 Size 非正 | 正常处理，输出 Size(0,0) |

---

## 8. 配置项设计

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| Orientation | `LinearLayoutOrientation` | `Vertical` | 排列方向 |
| CrossAxisAlignment | `CrossAxisAlignment` | `Stretch` | 交叉轴对齐方式 |

---

## 9. 文件变更清单

### 新增
| 文件 | 说明 |
|------|------|

| `MegaUI/Core/LinearLayout.cpp` | LinearLayout 实现（Create + DoLayout + UpdateDesiredSize 等，不含解析逻辑） |

### 修改
| 文件 | 变更 |
|------|------|
| `Include/MegaUI/Core/Element.h` | 新增 Margin 属性行 + `SpecMargin`/`CmpMargin` + `GetMargin()`/`SetMargin()` + `GetDesiredSize()` 声明 |
| `MegaUI/Core/Element.cpp` | Margin getter/setter + `Value::CreateLayout()` + 启用 Layout UpdateDesiredSize 路径 |
| `Include/MegaUI/Core/Layout.h` | 无变更 |
| `MegaUI/Core/Layout.cpp` | 无变更 |
| `MegaUI/Parser/UIParser.cpp` | `ParserLinearLayout`(内部静态) + `LookupLayoutParser`(内部静态) + `g_LayoutFactoryTable`(文件作用域) + `case ValueType::Layout` + `ParserLayoutValue()` |
| `MegaUI/Parser/UIParser.h` | 无变更（不暴露布局解析内部实现） |
| `MegaUI/MegaUI.vcxproj` / `.vcxproj.filters` | 新增 `LinearLayout.h`、`LinearLayout.cpp` |
| `MegaUI/Core/Value.h` | 新增 `CreateLayout()` 声明 |
| `MegaUI/Core/Value.cpp` | 新增 `CreateLayout()` 实现 |

---

## 10. 兼容性策略

- 不修改 `Layout` 基类虚函数签名
- `Margin` 默认值为 0，现有 Element 不受影响
- 布局解析工厂完全在 UIParser.cpp 内部闭环，Core 层（Layout.h/Element.h）无任何新增

---

## 11. 性能目标

- `UpdateDesiredSize`: O(n)，单次遍历，无堆分配
- `DoLayout`: O(n)，单次遍历，无堆分配
- 缓存脏标记避免重复计算

---

## 12. 单元测试框架

项目现有 MSTest C++ Native Unit Test（`UnitTest.vcxproj`），沿用。
