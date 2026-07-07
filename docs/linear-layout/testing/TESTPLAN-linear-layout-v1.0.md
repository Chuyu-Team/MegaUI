# LinearLayout 测试策略与测试计划

> 版本：v1.0
> 状态：草稿
> 最后更新：2026-07-07 17:14
> 关联 PRD：docs/linear-layout/prd/PRD-linear-layout-v1.0.md
> 关联 Specs：docs/linear-layout/specs/SPEC-linear-layout-v2.0.md

## 1. 测试目标

验证 `LinearLayout` 的布局计算、`Margin` 属性系统及 `UIParser` 布局解析功能的正确性和稳定性。

## 2. 测试策略

| 层次 | 策略 | 工具 |
|------|------|------|
| 单元测试 | 覆盖所有核心逻辑、边界条件和异常路径 | MSTest C++ Native |
| 集成测试 | 与 Element 生命周期联动验证（FlushDesiredSize→FlushLayout） | MSTest C++ Native |
| UIParser 测试 | 布局字符串解析正确性 | MSTest C++ Native |
| 构建验证 | 骨架编译 + 完整构建 | MSBuild |

> 本 feature 不涉及性能测试和系统测试，布局计算为纯算术运算，单元测试足以验证正确性。

## 3. 测试范围

### 覆盖

| 模块 | 覆盖范围 |
|------|----------|
| `LinearLayout` | `UpdateDesiredSize`、`DoLayout`（垂直+水平，四种交叉轴对齐 × 两种方向） |
| `Margin` 属性 | Get/Set、Cascade、Cmp 像素转换、间距计算 |
| `Element::UpdateDesiredSize` | Layout 路径启用后正确调用 `Layout::UpdateDesiredSize` |
| `Layout::LookupFactory` | 查找存在/不存在的布局名 |
| `UIParser` | `ParserLayoutValue` 解析无参数/有参数/非法名称 |
| LP_None / LP_Absolute | 跳过逻辑 |

### 不覆盖

- DPI 变化时 Margin 重新计算（框架级行为，已有 Padding 覆盖）
- 嵌套布局（后续 feature）

## 4. 关键风险点

| 风险 | 缓解 |
|------|------|
| `Element::UpdateDesiredSize` 启用 Layout 路径后影响现有行为 | Layout 路径仅在设置了 Layout 属性时才触发，无 Layout 仍走原路径 |
| Margin 正值导致子元素溢出容器 | 在 `DoLayout` 中将 `childW` / `childH` 钳位到 ≥ 0 |
| 交叉轴对齐在 Stretch 模式下 Width/Height 为 -1 的语义 | 测试用例覆盖所有组合 |

## 5. 关键测试场景（高层）

### 5.1 UpdateDesiredSize

| 场景 | 关键验证点 |
|------|-----------|
| 空子元素 | 返回 Size(0,0) |
| 单子元素 Vertical | width = child.w + margin.lr, height = child.h + margin.tb |
| 三子元素 Horizontal | 主轴宽度 = sum(child.w) + sum(spacing)，交叉轴高度 = max(child.h + margin.tb) |
| 全部 LP_None | 返回 Size(0,0) |
| 混合 LP_Absolute | LP_Absolute 被跳过，仅 LP_Auto 参与计算 |
| Margin 取最大值 | spacing = max(prev.bottom, curr.top) |

### 5.2 DoLayout

| 场景 | 关键验证点 |
|------|-----------|
| Vertical, CrossAlign=Stretch | 子元素 PosInLayout.X = margin.left, SizeInLayout.Width = container.w - margin.lr |
| Vertical, CrossAlign=Center | 子元素 PosInLayout.X 在容器中居中，受 Margin 边界约束 |
| Vertical, CrossAlign=End | 子元素 PosInLayout.X = container.w - child.w - margin.right |
| Horizontal, CrossAlign=Start | 子元素 PosInLayout.Y = margin.top |
| Horizontal, CrossAlign=Center | 子元素 PosInLayout.Y 在容器中居中，受 Margin 边界约束 |

### 5.3 Margin 属性

| 场景 | 关键验证点 |
|------|-----------|
| 默认值 | Margin = (0,0,0,0) |
| Set+Get | 通过 SetValue/GetValue 正常存取 |
| Cascade | 父元素设置 Margin，子元素继承 |

### 5.4 UIParser

| 场景 | 关键验证点 |
|------|-----------|
| `"LinearLayout"` | 创建默认实例 (Vertical + Stretch) |
| `"LinearLayout(Horizontal)"` | 创建 Horizontal + Stretch |
| `"LinearLayout(Vertical, Center)"` | 创建 Vertical + Center |
| `"UnknownLayout"` | 返回 ERROR_BAD_FORMAT |

### 5.5 Layout::LookupFactory

| 场景 | 关键验证点 |
|------|-----------|
| 已注册名称 | 返回非 nullptr 函数指针 |
| 未注册名称 | 返回 nullptr |

---

## 6. 准入 / 准出

| 条件 | 标准 |
|------|------|
| 准入 | Specs + 架构文档已确认，Plan 已生成 |
| 准出 | 所有单元测试通过，构建成功，静态分析无高置信度告警 |
