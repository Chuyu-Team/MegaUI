# LinearLayout 详细测试用例

> 版本：v1.0
> 状态：草稿
> 最后更新：2026-07-08 11:21
> 关联 Specs：docs/linear-layout/specs/SPEC-linear-layout-v2.0.md
> 关联 Plan：docs/linear-layout/plan/PLAN-linear-layout-v1.1.md
> 关联测试计划：docs/linear-layout/testing/TESTPLAN-linear-layout-v1.0.md

## 1. 测试用例总览

| 分组 | 编号范围 | 用例数 |
|---|---|---|
| Create 工厂 + Getter | TC-001 ~ TC-005 | 5 |
| Margin 属性 | TC-006 ~ TC-010 | 5 |
| Element::GetDesiredSize | TC-011 | 1 |
| UpdateDesiredSize - 垂直 | TC-012 ~ TC-018 | 7 |
| UpdateDesiredSize - 水平 | TC-019 ~ TC-024 | 6 |
| DoLayout - 垂直 | TC-025 ~ TC-032 | 8 |
| DoLayout - 水平 | TC-033 ~ TC-040 | 8 |
| LP_None / LP_Absolute | TC-041 ~ TC-043 | 3 |
| UIParser LookupLayoutParser | TC-044 ~ TC-045 | 2 |
| UIParser ParserLayoutValue | TC-046 ~ TC-049 | 4 |
| Value::CreateLayout | TC-050 ~ TC-051 | 2 |
| 集成测试 | TC-052 ~ TC-053 | 2 |

> 总计：53 个测试用例

---

## 2. 详细测试用例

### 2.1 Create 工厂 + Getter（TC-001 ~ TC-005）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-001 | `Create_默认参数_创建成功且Getter返回值一致` | `Create` / `GetOrientation` / `GetCrossAxisAlignment` | 无 | `Vertical, Stretch` | 调用 `Create(Vertical, Stretch, &pLayout)` | 返回 `S_OK`；`GetOrientation() == Vertical`；`GetCrossAxisAlignment() == Stretch` | 单元测试 |
| TC-002 | `Create_Horizontal方向_返回Horizontal` | `Create` | 无 | `Horizontal, Stretch` | 调用 `Create(Horizontal, Stretch, &pLayout)` | 返回 `S_OK`；`GetOrientation() == Horizontal` | 单元测试 |
| TC-003 | `Create_交叉轴Start对齐_返回Start` | `Create` | 无 | `Vertical, Start` | 调用 `Create(Vertical, Start, &pLayout)` | 返回 `S_OK`；`GetCrossAxisAlignment() == Start` | 单元测试 |
| TC-004 | `Create_交叉轴Center对齐_返回Center` | `Create` | 无 | `Vertical, Center` | 调用 `Create(Vertical, Center, &pLayout)` | 返回 `S_OK`；`GetCrossAxisAlignment() == Center` | 单元测试 |
| TC-005 | `Create_ppLayout为nullptr_返回E_POINTER` | `Create` | 无 | `nullptr` | 调用 `Create(Vertical, Stretch, nullptr)` | 返回 `E_POINTER` | 单元测试 |

### 2.2 Margin 属性（TC-006 ~ TC-010）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-006 | `Margin_默认值_全为0` | `GetValue(Margin)` | Element 已初始化 | 无 | 读取 Margin 属性值 | `GetValue(Margin).GetRect() == UnitRect(0,0,0,0)` | 单元测试 |
| TC-007 | `Margin_SetValue后GetValue_返回设置值` | `SetValue` / `GetValue` | Element 已初始化 | `UnitRect(1,2,3,4)` | 设置后读取 | `GetValue(Margin).GetRect() == UnitRect(1,2,3,4)` | 单元测试 |
| TC-008 | `Margin_Cascade_子元素继承父元素值` | Margin PF_Cascade | 父子 Element | 父 Margin=5 | 父设置 Margin=5，子读取 | 子元素 Margin == `UnitRect(5,5,5,5)` | 单元测试 |
| TC-009 | `Margin_DPI变化_CmpMargin自动重算` | CmpMargin | Element DPI=96 | 像素值 10 | DPI 切换为 192 (2x) | `CmpMargin == Rect(20,20,20,20)` | 集成测试 |
| TC-010 | `Margin_CmpMargin像素值与SpecMargin对应` | CmpMargin | Element 已初始化 | 像素 (10,20,5,8) | 设置 SpecMargin 后触发 FlushDesiredSize | `CmpMargin == Rect(10,20,5,8)` | 单元测试 |

### 2.3 Element::GetDesiredSize（TC-011）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-011 | `GetDesiredSize_返回LocDesiredSize` | `GetDesiredSize()` | Element 已完成 FlushDesiredSize | 无 | 触发 UpdateDesiredSize 后调用 | 返回值等于 `LocDesiredSize`，`Width >= 0, Height >= 0` | 单元测试 |

### 2.4 UpdateDesiredSize - 垂直模式（TC-012 ~ TC-018）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-012 | `UpdateDesiredSize_垂直空子元素_返回SizeZero` | `UpdateDesiredSizeVertical` | LinearLayout Vertical, 无子元素 | `(400,-1)` | 调用 UpdateDesiredSize | 返回 `Size(0, 0)` | 单元测试 |
| TC-013 | `UpdateDesiredSize_垂直单子无Margin_返回子元素DesiredSize` | 同上 | 1 子 DesiredSize=(100,50) Margin=0 | `(400,-1)` | 调用 UpdateDesiredSize | `Size(100, 50)` | 单元测试 |
| TC-014 | `UpdateDesiredSize_垂直单子带Margin_宽高含Margin` | 同上 | 1 子 DesiredSize=(100,50) Margin=(5,10,5,10) | `(400,-1)` | 调用 | W=110 H=70 | 单元测试 |
| TC-015 | `UpdateDesiredSize_垂直三子带Margin_高度累加取max间距` | 同上 | A:h=50 mb=5; B:h=80 mt=15; C:h=40 mt=0 mb=8 | `(400,-1)` | 调用 | H=10+50+max(5,15)+80+max(0,0)+40+8=203 | 单元测试 |
| TC-016 | `UpdateDesiredSize_垂直子元素LP_None全部跳过_返回SizeZero` | 同上 | 3 子全 LP_None | `(400,-1)` | 调用 | `Size(0,0)` | 单元测试 |
| TC-017 | `UpdateDesiredSize_垂直混合LP_Absolute_仅LP_Auto参与` | 同上 | A LP_Auto h=50, B LP_Absolute h=100, C LP_Auto h=30 | `(400,-1)` | 调用 | H=80 (B被跳过) | 单元测试 |
| TC-018 | `UpdateDesiredSize_垂直ConstraintSize限制宽度_不限制高度` | 同上 | 1 子 DesiredSize=(500,50) | `(200,-1)` | 调用 | W=500 H=50 | 单元测试 |

### 2.5 UpdateDesiredSize - 水平模式（TC-019 ~ TC-024）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-019 | `UpdateDesiredSize_水平空子元素_返回SizeZero` | `UpdateDesiredSizeHorizontal` | LinearLayout Horizontal, 无子元素 | `(-1,300)` | 调用 | `Size(0,0)` | 单元测试 |
| TC-020 | `UpdateDesiredSize_水平单子无Margin_返回子元素DesiredSize` | 同上 | 1 子 (100,50) Margin=0 | `(-1,300)` | 调用 | `Size(100,50)` | 单元测试 |
| TC-021 | `UpdateDesiredSize_水平单子带Margin_宽高含Margin` | 同上 | 1 子 (100,50) Margin=(10,5,10,5) | `(-1,300)` | 调用 | W=120 H=60 | 单元测试 |
| TC-022 | `UpdateDesiredSize_水平三子带Margin_宽度累加取max间距` | 同上 | A:w=50 mr=5; B:w=80 ml=15; C:w=40 mr=8 | `(-1,300)` | 调用 | W=10+50+max(5,15)+80+max(0,0)+40+8=203 | 单元测试 |
| TC-023 | `UpdateDesiredSize_水平全部LP_None跳过_返回SizeZero` | 同上 | 3 子全 LP_None | `(-1,300)` | 调用 | `Size(0,0)` | 单元测试 |
| TC-024 | `UpdateDesiredSize_水平ConstraintSize限制高度_不限制宽度` | 同上 | 1 子 (50,500) | `(-1,200)` | 调用 | W=50 H=500 | 单元测试 |

### 2.6 DoLayout - 垂直模式（TC-025 ~ TC-032）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-025 | `DoLayout_垂直Stretch对齐_宽度拉伸填满Margin内空间` | `DoLayoutVertical` CrossAlign=Stretch | 1 子 (50,30) Margin=(10,0,5,0) 容器W=400 | `ContainerSize(400,200)` | 调用 | PosInLayout.X=10; SizeInLayout.W=385 | 单元测试 |
| TC-026 | `DoLayout_垂直Start对齐_靠左贴MarginLeft` | 同上 CrossAlign=Start | 1 子 (50,30) Margin=(10,0,5,0) 容器W=400 | `(400,200)` | 调用 | PosInLayout.X=10; SizeInLayout.W=50 | 单元测试 |
| TC-027 | `DoLayout_垂直Center对齐_X在容器中水平居中` | 同上 CrossAlign=Center | 1 子 (100,30) Margin=0 容器W=400 | `(400,200)` | 调用 | PosInLayout.X=150 | 单元测试 |
| TC-028 | `DoLayout_垂直Center对齐_Margin约束边界` | 同上 | 1 子 (100,30) Margin=(50,0,0,0) 容器W=200 | `(200,200)` | 调用 | X>=50 且 X<=100 | 单元测试 |
| TC-029 | `DoLayout_垂直End对齐_靠右贴MarginRight` | 同上 CrossAlign=End | 1 子 (100,30) Margin=(0,0,20,0) 容器W=400 | `(400,200)` | 调用 | PosInLayout.X=280 | 单元测试 |
| TC-030 | `DoLayout_垂直三子Margin间距_位置正确累加` | 同上 CrossAlign=Start | A:h=50 mb=5; B:h=80 mt=15; C:h=40 mt=0 | `(400,300)` | 调用 | A.Y=mt; B.Y=A.Y+A.h+max(A.mb,B.mt); C.Y=B.Y+B.h+max(B.mb,C.mt) | 单元测试 |
| TC-031 | `DoLayout_垂直Stretch_Margin超过容器时宽度为0` | 同上 CrossAlign=Stretch | Margin=(300,0,200,0) 容器W=400 | `(400,200)` | 调用 | SizeInLayout.W=max(0,400-300-200)=0 | 单元测试 |
| TC-032 | `DoLayout_垂直空子元素_无操作不崩溃` | 同上 | 无子元素 | `(400,200)` | 调用 | 不崩溃 | 单元测试 |

### 2.7 DoLayout - 水平模式（TC-033 ~ TC-040）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-033 | `DoLayout_水平Stretch对齐_高度拉伸填满Margin内空间` | `DoLayoutHorizontal` CrossAlign=Stretch | 1 子 (50,30) Margin=(0,10,0,5) 容器H=300 | `(200,300)` | 调用 | PosInLayout.Y=10; SizeInLayout.H=285 | 单元测试 |
| TC-034 | `DoLayout_水平Start对齐_靠顶贴MarginTop` | 同上 CrossAlign=Start | 1 子 (50,30) Margin=(0,10,0,5) 容器H=300 | `(200,300)` | 调用 | PosInLayout.Y=10; SizeInLayout.H=30 | 单元测试 |
| TC-035 | `DoLayout_水平Center对齐_Y在容器中垂直居中` | 同上 CrossAlign=Center | 1 子 (50,100) Margin=0 容器H=300 | `(200,300)` | 调用 | PosInLayout.Y=100 | 单元测试 |
| TC-036 | `DoLayout_水平Center对齐_Margin约束到Top边界` | 同上 | 1 子 (50,100) Margin=(0,120,0,0) 容器H=200 | `(200,200)` | 调用 | Y>=120 且 Y<=100→钳位到120 | 单元测试 |
| TC-037 | `DoLayout_水平End对齐_靠底贴MarginBottom` | 同上 CrossAlign=End | 1 子 (50,100) Margin=(0,0,0,20) 容器H=300 | `(200,300)` | 调用 | PosInLayout.Y=180 | 单元测试 |
| TC-038 | `DoLayout_水平三子Margin间距_位置正确累加` | 同上 CrossAlign=Start | A:w=50 mr=5; B:w=80 ml=15; C:w=40 ml=0 | `(400,200)` | 调用 | A.X=ml; B.X=A.X+A.w+max(A.mr,B.ml); C.X=B.X+B.w+max(B.mr,C.ml) | 单元测试 |
| TC-039 | `DoLayout_水平Stretch_Margin超过容器时高度为0` | 同上 CrossAlign=Stretch | Margin=(0,200,0,150) 容器H=300 | `(200,300)` | 调用 | SizeInLayout.H=max(0,300-200-150)=0 | 单元测试 |
| TC-040 | `DoLayout_水平空子元素_无操作不崩溃` | 同上 | 无子元素 | `(200,300)` | 调用 | 不崩溃 | 单元测试 |

### 2.8 LP_None / LP_Absolute 跳过逻辑（TC-041 ~ TC-043）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-041 | `DoLayout_LP_None跳过_子元素不在布局结果中` | `DoLayout` | A LP_Auto h=50, B LP_None h=30 | `(400,200)` Vertical | 调用 | B 的 PosInLayout 未被更新 | 单元测试 |
| TC-042 | `DoLayout_LP_Absolute跳过_子元素位置保持不变` | `DoLayout` | A LP_Auto, B LP_Absolute Pos=(99,99) | `(400,200)` Vertical | 调用 | B 的 PosInLayout 仍为 (99,99) | 单元测试 |
| TC-043 | `UpdateDesiredSize_混合LP_None和LP_Absolute全部跳过_返回SizeZero` | `UpdateDesiredSize` | A LP_None, B LP_Absolute | `(400,-1)` Vertical | 调用 | `Size(0,0)` | 单元测试 |

### 2.9 UIParser LookupLayoutParser（TC-044 ~ TC-045）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-044 | `LookupLayoutParser_已注册名称LinearLayout_返回非nullptr` | `LookupLayoutParser` | g_LayoutFactoryTable 初始化 | `"LinearLayout"` | 调用 | 返回非 nullptr | 单元测试 |
| TC-045 | `LookupLayoutParser_未注册名称UnknownLayout_返回nullptr` | `LookupLayoutParser` | 同上 | `"UnknownLayout"` | 调用 | 返回 nullptr | 单元测试 |

### 2.10 UIParser ParserLayoutValue（TC-046 ~ TC-049）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-046 | `ParserLayoutValue_无参默认_创建VerticalStretch实例` | `ParserLayoutValue` | UIParser 初始化 | ExprNode `"LinearLayout"` | 调用 | Orientation=Vertical, CrossAlign=Stretch | 单元测试 |
| TC-047 | `ParserLayoutValue_单参Horizontal_创建HorizontalStretch实例` | 同上 | 同上 | ExprNode `"LinearLayout(Horizontal)"` | 调用 | Orientation=Horizontal, CrossAlign=Stretch | 单元测试 |
| TC-048 | `ParserLayoutValue_双参VerticalCenter_创建VerticalCenter实例` | 同上 | 同上 | ExprNode `"LinearLayout(Vertical, Center)"` | 调用 | Orientation=Vertical, CrossAlign=Center | 单元测试 |
| TC-049 | `ParserLayoutValue_未知布局名_返回BAD_FORMAT` | 同上 | 同上 | ExprNode `"UnknownLayout"` | 调用 | `HRESULT_From_LSTATUS(ERROR_BAD_FORMAT)` | 单元测试 |

### 2.11 Value::CreateLayout（TC-050 ~ TC-051）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-050 | `CreateLayout_非nullptr输入_GetType为Layout且指针可恢复` | `Value::CreateLayout` | 有效 `Layout*` | `pLayout` | 创建 Value 后验证 | `GetType()==Layout`；RawBuffer 指针等于 pLayout | 单元测试 |
| TC-051 | `CreateLayout_nullptr输入_Type为Layout指针为nullptr` | 同上 | 无 | `nullptr` | 创建 Value 后验证 | `GetType()==Layout`；指针为 nullptr | 单元测试 |

### 2.12 集成测试（TC-052 ~ TC-053）

| 用例编号 | 用例名称 | 测试对象 | 前置条件 | 输入 | 执行步骤 | 预期结果 | 测试层次 |
|---|---|---|---|---|---|---|---|
| TC-052 | `集成_垂直布局_FlushDesiredSize后FlushLayout位置正确` | 完整流水线 | Element Layout=LinearLayout(Vertical,Stretch) 3 子 Margin=0 | `(400,-1)` | FlushDesiredSize → FlushLayout → 读取子元素 PosInLayout | 子元素 Y 依次累加；X=0 宽度拉伸到 400 | 集成测试 |
| TC-053 | `集成_水平布局_FlushDesiredSize后FlushLayout位置正确` | 完整流水线 | Element Layout=LinearLayout(Horizontal,Stretch) 3 子 Margin=0 | `(-1,300)` | 同上 | 子元素 X 依次累加；Y=0 高度拉伸到 300 | 集成测试 |

---

## 3. 用例与 Plan 任务映射

| Plan 任务 | 关联用例 |
|---|---|
| T1 Margin 属性落地 | TC-006 ~ TC-010 |
| T2 GetDesiredSize + 启用 Layout 路径 | TC-011 |
| T3 Value::CreateLayout | TC-050 ~ TC-051 |
| T4 UIParser 布局解析基础设施 | TC-044 ~ TC-045 |
| T5 LinearLayout 骨架 + 头文件 | TC-001 ~ TC-005 |
| T6 LinearLayout UpdateDesiredSize 实现 | TC-012 ~ TC-024 |
| T7 LinearLayout DoLayout 实现 | TC-025 ~ TC-040 |
| T8 UIParser 布局解析适配 | TC-046 ~ TC-049 |
| T9 单元测试 | TC-001 ~ TC-053 全量 |
| LP_None / LP_Absolute | TC-041 ~ TC-043 |
| 集成测试 | TC-052 ~ TC-053 |
