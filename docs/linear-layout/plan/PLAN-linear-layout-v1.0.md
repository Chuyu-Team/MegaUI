# LinearLayout 实施计划

> 版本：v1.0
> 状态：草稿
> 最后更新：2026-07-07 17:14
> 关联 PRD：docs/linear-layout/prd/PRD-linear-layout-v1.0.md
> 关联 Specs：docs/linear-layout/specs/SPEC-linear-layout-v2.0.md

## 1. 目标

本计划将 LinearLayout feature 分解为可独立验证的任务，按依赖顺序排列。每个任务明确受影响文件、完成定义和验证方式。

## 2. 任务列表

| 编号 | 名称 | 优先级 | 依赖 | 复杂度 |
|------|------|--------|------|--------|
| T1 | Margin 属性落地 | P0 | - | M |
| T2 | Element GetDesiredSize + 启用 Layout UpdateDesiredSize 路径 | P0 | - | S |
| T3 | Value::CreateLayout | P0 | - | S |
| T4 | Layout 基类工厂注册表 | P0 | - | M |
| T5 | LinearLayout 骨架 + 头文件 | P0 | T4 | M |
| T6 | LinearLayout UpdateDesiredSize 实现 | P0 | T1, T2, T3, T5 | L |
| T7 | LinearLayout DoLayout 实现 | P0 | T6 | L |
| T8 | UIParser 适配 | P0 | T3, T4, T5 | M |
| T9 | 单元测试 | P0 | T7, T8 | L |
| T10 | 工程文件更新 + 骨架构建 | P0 | T5 | S |

### T1 - Margin 属性落地

| 项 | 内容 |
|------|------|
| 目标 | Element 新增 Margin 属性，含属性表绑定、成员变量、Getter/Setter |
| 影响文件 | `Include/MegaUI/Core/Element.h`、`MegaUI/Core/Element.cpp` |
| 完成定义 | Margin 属性可通过 `SetValue`/`GetValue` 存取，PropertyGroup 正确触发重布局 |
| 实现要点 | 在 `_MEGA_UI_ELEMENT_PROPERTY_TABLE` 中参照 `Padding` 行插入 `Margin` 行；新增 `SpecMargin`(UnitRect) + `CmpMargin`(Rect) 成员变量；`GetMargin()`/`SetMargin()` 公开方法；`_COMPUTED_IS_RECT=1` 使框架自动完成 UnitRect→像素 Rect 转换 |
| 验证 | 骨架构建通过 |

### T2 - Element GetDesiredSize + 启用 Layout UpdateDesiredSize 路径

| 项 | 内容 |
|------|------|
| 目标 | 新增 `GetDesiredSize()` 便捷访问；将 `#if 0` 注释的 Layout 路径改为实际启用 |
| 影响文件 | `Include/MegaUI/Core/Element.h`、`MegaUI/Core/Element.cpp` |
| 完成定义 | `GetDesiredSize()` 返回 `LocDesiredSize`；设置 Layout 属性的 Element 在 `UpdateDesiredSize` 中走 `Layout::UpdateDesiredSize` 分支 |
| 实现要点 | `GetDesiredSize()` 为 inline const 方法；启用 Layout 分支逻辑：bSelfLayout → SelfLayoutUpdateDesiredSize，否则若 Layout 非空 → Layout::UpdateDesiredSize，否则 → GetContentSize |
| 验证 | 骨架构建通过 |

### T3 - Value::CreateLayout

| 项 | 内容 |
|------|------|
| 目标 | 新增 `Value::CreateLayout(Layout*)` 静态工厂方法 |
| 影响文件 | `Include/MegaUI/Core/Value.h`、`MegaUI/Core/Value.cpp` |
| 完成定义 | 可创建 `ValueType::Layout` 类型的 Value 包装给定 Layout 指针 |
| 实现要点 | 参照 `CreateStyleSheet` 模式实现 |
| 验证 | 骨架构建通过 |

### T4 - Layout 基类工厂注册表

| 项 | 内容 |
|------|------|
| 目标 | Layout 基类新增工厂注册基础设施 |
| 影响文件 | `Include/MegaUI/Core/Layout.h`、`MegaUI/Core/Layout.cpp` |
| 完成定义 | `LayoutParserFunc` 类型定义、`LayoutFactoryEntry` 结构、`LookupFactory()` 静态方法、`g_LayoutFactoryTable` 注册表（初始为空或含 LinearLayout） |
| 实现要点 | `struct ExprNode;` 前向声明避免反向依赖；`g_LayoutFactoryTable` 在 `Layout.cpp` 中定义为文件作用域静态数组 |
| 验证 | 骨架构建通过 |

### T5 - LinearLayout 骨架 + 头文件

| 项 | 内容 |
|------|------|
| 目标 | 新建 `LinearLayout` 类声明和骨架实现 |
| 影响文件 | `Include/MegaUI/Core/LinearLayout.h`（新建）、`MegaUI/Core/LinearLayout.cpp`（新建） |
| 完成定义 | 类声明完整（枚举、工厂、ParserLayout、虚函数声明、内部辅助）；`Create`/`ParserLayout`/`GetOrientation`/`SetOrientation` 等骨架实现可编译 |
| 实现要点 | `LinearLayoutOrientation` 和 `CrossAxisAlignment` 枚举；`ParserLayout` 静态方法含 `s_OrientationEnumMap` + `s_CrossAxisAlignmentEnumMap` + `ParsedArg _Args[]` + 调用 `UIParser::ParserFunction`；`Create` 接受 `(Orientation, CrossAxisAlignment, _Outptr_)` |
| 验证 | 骨架构建通过 |

### T6 - LinearLayout UpdateDesiredSize 实现

| 项 | 内容 |
|------|------|
| 目标 | 实现 `UpdateDesiredSize` 虚函数（垂直+水平） |
| 影响文件 | `MegaUI/Core/LinearLayout.cpp` |
| 完成定义 | 垂直/水平两种方向正确计算期望尺寸，含 Margin 间距 |
| 实现要点 | 统一入口根据 `eOrientation` 分发到 `UpdateDesiredSizeVertical`/`Horizontal`；遍历子元素跳过 LP_None/LP_Absolute；垂直模式主轴高度 = sum(child.h) + sum(spacing)，交叉轴宽度 = max(child.w + margin.lr)；水平模式对称；spacing = max(prevMargin, currMargin) |
| 验证 | 单元测试通过 |

### T7 - LinearLayout DoLayout 实现

| 项 | 内容 |
|------|------|
| 目标 | 实现 `DoLayout` 虚函数（垂直+水平），含交叉轴对齐 |
| 影响文件 | `MegaUI/Core/LinearLayout.cpp` |
| 完成定义 | 子元素 PosInLayout/SizeInLayout 正确设置，Stretch/Center/End/Start 四种交叉轴对齐正确 |
| 实现要点 | 统一入口根据 `eOrientation` 分发到 `DoLayoutVertical`/`Horizontal`；使用 `GetDesiredSize()` 获取子元素期望尺寸；交叉轴对齐计算：Stretch→填满容器、Start→靠左/上、Center→居中、End→靠右/下 |
| 验证 | 单元测试通过 |

### T8 - UIParser 适配

| 项 | 内容 |
|------|------|
| 目标 | UIParser 新增 `ValueType::Layout` 解析分支 |
| 影响文件 | `Include/MegaUI/Parser/UIParser.h`、`MegaUI/Parser/UIParser.cpp` |
| 完成定义 | `ParserLayoutValue()` 实现完整，XML 中 `Layout="LinearLayout(Horizontal, Center)"` 可正确解析 |
| 实现要点 | `ParserValue()` switch 中新增 `case ValueType::Layout`；`ParserLayoutValue()` 从 ExprNode 提取名称→`LookupFactory`→`pfnParser`→`Value::CreateLayout` |
| 验证 | 单元测试通过 |

### T9 - 单元测试

| 项 | 内容 |
|------|------|
| 目标 | 覆盖 LinearLayout、Margin、UIParser 的核心/边界/异常场景 |
| 影响文件 | `UnitTest/LinearLayoutUnitTest.cpp`（新建） |
| 完成定义 | 所有测试用例通过，覆盖测试计划中列出的场景 |
| 实现要点 | 基于 §5 测试计划设计详细测试用例；优先覆盖 UpdateDesiredSize/DoLayout 垂直+水平 × 四种对齐；Margin 间距计算；LP_None/LP_Absolute 跳过；空子元素/单子元素；UIParser 无参/有参/非法名 |

### T10 - 工程文件更新 + 骨架构建

| 项 | 内容 |
|------|------|
| 目标 | 新增文件加入 `.vcxproj` / `.vcxproj.filters`，确保可编译 |
| 影响文件 | `MegaUI/MegaUI.vcxproj`、`MegaUI/MegaUI.vcxproj.filters` |
| 完成定义 | MSBuild 编译通过 |

## 3. 依赖关系

```
T1 ──┐
T2 ──┼── T6 ── T7 ──┐
T3 ──┤               │
T4 ──┼── T5 ────────┼── T9
     │               │
     └── T8 ─────────┘
T10 (与 T5 串联，随新增文件完成)
```

## 4. 风险任务

| 任务 | 风险 | 缓解 |
|------|------|------|
| T6 | UpdateDesiredSize 中 Margin 计算可能遗漏边界场景 | 详尽测试用例覆盖空子元素、Margin 为 0、Margin 为非零 |
| T7 | 交叉轴对齐四种模式涉及较多条件分支 | 每种对齐单测独立覆盖 |
| T8 | UIParser 中 Funcall 的 ExprNode 提取可能与预期不符 | 参照现有 ParserRectValue 调用 ParserFunction 模式 |
