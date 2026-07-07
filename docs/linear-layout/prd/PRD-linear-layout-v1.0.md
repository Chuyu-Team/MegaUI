# LinearLayout PRD

> 版本：v1.0
> 状态：草稿
> 最后更新：2026-07-06 17:24

## 1. 背景与目标

### 1.1 背景
MegaUI 已设计 `Layout` 抽象基类（`Include\MegaUI\Core\Layout.h`），定义了 `DoLayout`、`UpdateDesiredSize`、`OnAdd`、`OnRemove` 等完整虚函数接口，Element 也具备 `Layout` 属性（`ValueType::Layout`）。但目前**没有任何具体布局实现**，所有子元素必须手动指定绝对位置（`LP_Absolute`），无法实现自动排列。

### 1.2 目标
- 实现 `LinearLayout`，支持子元素沿垂直或水平方向线性排列
- 新增 `Margin` 属性到 `Element`，为布局提供子元素间距控制
- 完善 `UIParser` 对 `Layout` 类型属性的解析支持，使 XML 中可以直接指定 `Layout="LinearLayout"`
- 布局逻辑不依赖 Windows 特有 API，保持跨平台能力

### 1.3 非目标
- 不实现换行/折行（Wrap）功能

---

## 2. 问题定义

- **核心问题**：当前所有 Element 的子元素位置必须手动计算和指定，无法实现自动线性排列
- **当前痛点**：每新增一个子元素都需要手动计算其 X/Y 坐标，开发效率低、易出错
- **影响范围**：所有使用 MegaUI 构建 UI 界面的场景

---

## 3. 使用场景 / 用户故事

- **用户故事 1**：开发者创建一个垂直布局容器，依次添加 3 个子元素，子元素自动从上到下排列，间距由各自的 Margin 控制
- **用户故事 2**：开发者切换 LinearLayout 的 Orientation 为 Horizontal，子元素自动从左到右排列
- **用户故事 3**：某个子元素设置 `LayoutPos = LP_Absolute`，LinearLayout 跳过该子元素，该子元素保持自由定位
- **用户故事 4**：某个子元素设置 `LayoutPos = LP_None`，LinearLayout 忽略该子元素

---

## 4. 功能需求

### 4.1 LinearLayout

| 编号 | 功能 | 描述 |
|------|------|------|
| F1 | Orientation 属性 | 支持 `Vertical`（垂直）和 `Horizontal`（水平）两种排列方向。默认 `Vertical` |
| F2 | 垂直排列 | Orientation=Vertical 时，子元素按添加顺序从上到下排列。子元素宽度默认拉伸填满容器可用宽度（当 Width=-1 时），高度按内容自适应 |
| F3 | 水平排列 | Orientation=Horizontal 时，子元素按添加顺序从左到右排列。子元素高度默认拉伸填满容器可用高度（当 Height=-1 时），宽度按内容自适应 |
| F4 | 自动排列子元素 | `LayoutPos = LP_Auto` 的子元素按添加顺序参与排列 |
| F5 | 跳过绝对定位元素 | `LayoutPos = LP_Absolute` 的子元素被跳过，保持其自由定位 |
| F6 | 忽略隐藏元素 | `LayoutPos = LP_None` 的子元素被忽略，不占位 |
| F7 | DesiredSize 计算 | 垂直模式：宽度 = 子元素最大宽度，高度 = 所有参与排列子元素高度之和 + 子元素间 Margin 间距之和。水平模式：宽度 = 所有参与排列子元素宽度之和 + 子元素间 Margin 间距之和，高度 = 子元素最大高度 |
| F8 | DoLayout 布局 | 为每个参与排列的子元素计算 PosInLayout 和 SizeInLayout |
| F9 | 交叉轴对齐 | 垂直模式支持 Left/Center/Right/Stretch；水平模式支持 Top/Middle/Bottom/Stretch。默认 Stretch |
| F10 | 动态增删 | 子元素增删时正确 invalide 缓存并触发重新布局 |

### 4.2 Margin 属性

| 编号 | 功能 | 描述 |
|------|------|------|
| F10 | Margin 属性 | 新增 Element 级 `Margin` 属性，类型为 `UnitRect`，默认值为 0 |
| F11 | PropertyGroup | `PG_AffectsDesiredSize | PG_AffectsLayout | PG_AffectsParentLayout` |
| F12 | PropertyFlag | `PF_TriLevel | PF_Cascade`（参照 Padding） |
| F13 | 间距计算 | 相邻两子元素间距 = `max(上一个.Margin.Bottom, 下一个.Margin.Top)` |
| F14 | 首尾间距 | 首个元素顶部间距 = 其 Margin.Top；末元素底部间距 = 其 Margin.Bottom |

### 4.3 UIParser 支持

| 编号 | 功能 | 描述 |
|------|------|------|
| F15 | UIParser 解析 Layout | 在 `UIParser::ParserValue()` 中新增 `case ValueType::Layout` 解析分支（`ppValidValues` 已由宏自动生成，无需修改） |
| F16 | ParserValue 分支 | 在 `UIParser::ParserValue()` 中新增 `case ValueType::Layout` 解析分支 |
| F17 | 布局名称→工厂映射 | 建立布局类型字符串（如 `"LinearLayout"`）到工厂函数的注册/查找机制 |

---

## 5. 非功能需求

- **性能**：布局计算应避免不必要的堆分配，尽量复用栈上变量
- **可靠性**：子元素增删时布局状态正确更新，不残留脏数据
- **兼容性**：C++14/17/20 均可编译；Windows x86/x64/ARM64
- **跨平台**：布局核心逻辑不调用 Windows 特有 API

---

## 6. 约束与边界

- 继承自现有 `Layout` 基类，遵循其接口契约
- 遵循 `CONTRIBUTING.md` 中定义的编码规范（命名、SAL、summary 注释等）
- 不修改 `Layout` 基类的虚函数签名
- `Margin` 属性遵循现有属性系统约定（`UnitRect`、tri-level、cascade）

---

## 7. 风险与权衡

| 风险 | 影响 | 缓解 |
|------|------|------|
| Margin 属性增加导致 Element 内存增大 | 每个 Element 增加约 28 字节（UnitRect + Rect cache） | Element 已有 Padding 可参考，增幅可接受 |
| Orientation 切换时布局行为差异 | 垂直与水平排列在 Width/Height 拉伸方向不同，可能引起混淆 | 在 Specs 和文档中明确各方向的默认拉伸规则 |
| 布局计算与现有 SelfLayout 路径交互 | 行为可能不一致 | 在 FlushLayout 中已验证 Layout 路径正确触发 |

---

## 8. 里程碑建议

1. **M1 - Margin 属性落地**：Element 新增 Margin 属性，含完整属性绑定、缓存
2. **M2 - LinearLayout 垂直+水平布局**：实现 DoLayout + UpdateDesiredSize 垂直与水平模式
3. **M3 - UIParser 支持**：完善 UIParser 对 Layout 属性的解析，建立工厂注册机制
4. **M4 - 单元测试**：覆盖垂直和水平布局的正常、边界、异常路径，覆盖 UIParser 解析

---

## 9. 验收标准

- `LinearLayout` 继承自 `Layout`，可被 `Element::SetValue(LayoutProp, ...)` 设置
- 设置 Orientation=Vertical 后，添加 3 个子元素，子元素自动从上到下排列；子元素 Width=-1 时自动拉伸至容器可用宽度
- 设置 Orientation=Horizontal 后，添加 3 个子元素，子元素自动从左到右排列；子元素 Height=-1 时自动拉伸至容器可用高度
- `LayoutPos = LP_Absolute` 的子元素不参与排列
- `LayoutPos = LP_None` 的子元素被忽略
- `Margin` 属性的 Get/Set 正常，间距按最大值规则计算（垂直模式取上下 Margin 最大值，水平模式取左右 Margin 最大值）
- XML 中可通过 `Layout="LinearLayout"` 或 `Layout="LinearLayout(Orientation=Vertical)"` 指定布局
- 构建通过、单元测试全部通过、静态分析无高置信度告警

---

## 10. 待确认事项

- Margin 四方向间距取最大值的行为已确认
- 水平与垂直布局均已要求完整实现
