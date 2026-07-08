# LinearLayout 实施计划

> 版本：v1.1
> 状态：草稿
> 最后更新：2026-07-07 20:38
> 关联 Specs：docs/linear-layout/specs/SPEC-linear-layout-v2.0.md

## 1. 目标

本计划将 LinearLayout feature 分解为可独立验证的任务，按依赖顺序排列。

## 2. 任务列表

| 编号 | 名称 | 优先级 | 依赖 | 复杂度 |
|------|------|--------|------|--------|
| T1 | Margin 属性落地 | P0 | - | M |
| T2 | Element GetDesiredSize + 启用 Layout UpdateDesiredSize 路径 | P0 | - | S |
| T3 | Value::CreateLayout | P0 | - | S |
| T4 | UIParser 布局解析基础设施 | P0 | - | M |
| T5 | LinearLayout 骨架 + 头文件 | P0 | - | M |
| T6 | LinearLayout UpdateDesiredSize 实现 | P0 | T1, T2, T3, T5 | L |
| T7 | LinearLayout DoLayout 实现 | P0 | T6 | L |
| T8 | UIParser 布局解析适配（ParserLinearLayout + ParserLayoutValue） | P0 | T3, T4, T5 | M |
| T9 | 单元测试 | P0 | T7, T8 | L |
| T10 | 工程文件更新 + 骨架构建 | P0 | T5 | S |

### T1 - Margin 属性落地

| 项 | 内容 |
|------|------|
| 影响文件 | `Include/MegaUI/Core/Element.h`、`MegaUI/Core/Element.cpp` |
| 完成定义 | Margin 属性可通过 `SetValue`/`GetValue` 存取；`_COMPUTED_IS_RECT=1` 自动完成 UnitRect→像素 Rect 转换 |
| 验证 | 骨架构建通过 |

### T2 - GetDesiredSize + 启用 Layout 路径

| 项 | 内容 |
|------|------|
| 影响文件 | `Include/MegaUI/Core/Element.h`、`MegaUI/Core/Element.cpp` |
| 完成定义 | `GetDesiredSize()` 返回 `LocDesiredSize`；`#if 0` 注释替换为 Layout 路径启用代码 |
| 验证 | 骨架构建通过 |

### T3 - Value::CreateLayout

| 项 | 内容 |
|------|------|
| 影响文件 | `Include/MegaUI/Core/Value.h`、`MegaUI/Core/Value.cpp` |
| 完成定义 | `Value::CreateLayout(Layout*)` 可创建包装 Layout 指针的 Value |
| 验证 | 骨架构建通过 |

### T4 - UIParser 布局解析基础设施

| 项 | 内容 |
|------|------|
| 影响文件 | `MegaUI/Parser/UIParser.cpp` |
| 完成定义 | `LayoutParserFunc` 类型、`LayoutFactoryEntry` 结构、`g_LayoutFactoryTable` 注册表、`LookupLayoutParser` 查找函数均在 UIParser.cpp 文件作用域定义且可编译 |
| 验证 | 骨架构建通过 |

### T5 - LinearLayout 骨架 + 头文件

| 项 | 内容 |
|------|------|
| 影响文件 | `Include/MegaUI/Core/LinearLayout.h`、`MegaUI/Core/LinearLayout.cpp` |
| 完成定义 | 类声明完整（枚举、`Create`、虚函数声明、内部辅助）；不含解析逻辑 |
| 验证 | 骨架构建通过 |

### T6 - LinearLayout UpdateDesiredSize 实现

| 项 | 内容 |
|------|------|
| 影响文件 | `MegaUI/Core/LinearLayout.cpp` |
| 完成定义 | 垂直/水平期望尺寸计算正确，含 Margin 间距 |
| 验证 | 单元测试通过 |

### T7 - LinearLayout DoLayout 实现

| 项 | 内容 |
|------|------|
| 影响文件 | `MegaUI/Core/LinearLayout.cpp` |
| 完成定义 | 四种交叉轴对齐正确，子元素 PosInLayout/SizeInLayout 正确 |
| 验证 | 单元测试通过 |

### T8 - UIParser 布局解析适配

| 项 | 内容 |
|------|------|
| 影响文件 | `MegaUI/Parser/UIParser.cpp` |
| 完成定义 | `ParserLinearLayout`（内部静态）+ `ParserLayoutValue` + `case ValueType::Layout` 正确解析并创建实例 |
| 验证 | 单元测试通过 |

### T9 - 单元测试

| 项 | 内容 |
|------|------|
| 影响文件 | `UnitTest/LinearLayoutUnitTest.cpp` |
| 完成定义 | 34 个测试用例全部通过 |

### T10 - 工程文件更新

| 项 | 内容 |
|------|------|
| 影响文件 | `MegaUI/MegaUI.vcxproj`、`MegaUI/MegaUI.vcxproj.filters` |
| 完成定义 | 新增 `LinearLayout.h`/`LinearLayout.cpp`，MSBuild 编译通过 |

## 3. 依赖关系

```
T1 ──┐
T2 ──┼── T6 ── T7 ──┐
T3 ──┤               │
     ├── T8 ─────────┼── T9
T4 ──┤               │
T5 ──┤               │
     └── T10 　　　   │
```

## 4. 风险任务

| 任务 | 风险 | 缓解 |
|------|------|------|
| T6 | Margin 计算边界 | 详尽测试覆盖 |
| T7 | 交叉轴对齐分支多 | 每种对齐独立单测 |
| T8 | ParserFunction 交互 | 参照现有 ParserRectValue 模式 |
