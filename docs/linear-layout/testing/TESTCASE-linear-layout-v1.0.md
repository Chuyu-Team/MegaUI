# LinearLayout 详细测试用例

> 版本：v1.0
> 状态：草稿
> 最后更新：2026-07-07 18:12
> 关联 Specs：docs/linear-layout/specs/SPEC-linear-layout-v2.0.md
> 关联 Plan：docs/linear-layout/plan/PLAN-linear-layout-v1.0.md

## 1. LinearLayout::UpdateDesiredSize

### 1.1 垂直模式

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-001 | `UpdateDesiredSize_空子元素_返回零尺寸` | 容器无子元素 | Orientation=Vertical, ConstraintSize=(400, -1) | 返回 Size(0, 0) |
| TC-002 | `UpdateDesiredSize_单子元素无Margin_返回子元素尺寸` | 1个子元素DesiredSize=(100,50), Margin=0 | Orientation=Vertical, ConstraintSize=(400, -1) | 返回 Size(100, 50) |
| TC-003 | `UpdateDesiredSize_单子元素有Margin_返回含Margin尺寸` | 1个子元素DesiredSize=(100,50), Margin=(10,20,5,15) | Orientation=Vertical, ConstraintSize=(400, -1) | 返回 Size(115, 85)。宽=100+10+5=115，高=20+50+15=85 |
| TC-004 | `UpdateDesiredSize_三子元素垂直排列_累计高度` | A=(100,50) M=(10,5,5,10); B=(200,80) M=(5,20,5,0); C=(150,40) M=(0,10,5,8) | Orientation=Vertical, ConstraintSize=(400, -1) | 宽=max(100+10+5, 200+5+5, 150+0+5)=max(115,210,155)=210; 高=5+50+max(10,20)+80+max(0,10)+40+8=5+50+20+80+10+40+8=213 |
| TC-005 | `UpdateDesiredSize_全部LP_None_返回零尺寸` | 3个子元素全部 LP_None | Orientation=Vertical, ConstraintSize=(400, -1) | 返回 Size(0, 0) |
| TC-006 | `UpdateDesiredSize_混合LP_Absolute_仅计算LP_Auto` | A(LP_Auto, 100×50), B(LP_Absolute, 200×80), C(LP_Auto, 150×40) | Orientation=Vertical | 仅A和C参与：宽=max(100,150)=150，高=50+40=90 |

### 1.2 水平模式

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-007 | `UpdateDesiredSize_水平单子元素有Margin_返回含Margin尺寸` | 1个子元素DesiredSize=(50,100), Margin=(10,20,5,15) | Orientation=Horizontal, ConstraintSize=(-1, 300) | 宽=10+50+5=65，高=100+20+15=135 |
| TC-008 | `UpdateDesiredSize_水平三子元素_累计宽度` | A=(50,100) M=(10,5,5,0); B=(80,200) M=(5,0,5,10); C=(40,150) M=(0,10,5,8) | Orientation=Horizontal | 宽=10+50+max(5,5)+80+max(5,0)+40+5=10+50+5+80+5+40+5=195; 高=max(100+0+0, 200+0+10, 150+10+8)=max(100,210,168)=210 |
| TC-009 | `UpdateDesiredSize_水平空子元素_返回零尺寸` | 无子元素 | Orientation=Horizontal | 返回 Size(0, 0) |

## 2. LinearLayout::DoLayout

### 2.1 垂直模式 × 交叉轴对齐

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-010 | `DoLayout_垂直Stretch_宽度拉伸填满` | 1个子元素DesiredSize=(100,50), Margin=(10,5,5,15) | oSize=(400,300), CrossAlign=Stretch | PosInLayout=(10, 5), SizeInLayout.Width=max(0, 400-10-5)=385 |
| TC-011 | `DoLayout_垂直Start_左对齐` | 1个子元素DesiredSize=(100,50), Margin=(10,5,30,15) | oSize=(400,300), CrossAlign=Start | PosInLayout=(10, 5), SizeInLayout=(100, 50) |
| TC-012 | `DoLayout_垂直Center_居中` | 1个子元素DesiredSize=(100,50), Margin=(10,5,20,15) | oSize=(400,300), CrossAlign=Center | childX=(400-100)/2=150, 150≥10且150≤400-20-100=280 → PosInLayout=(150, 5) |
| TC-013 | `DoLayout_垂直Center_Margin限制_被推到Margin边界` | 1个子元素DesiredSize=(350,50), Margin=(30,5,20,15) | oSize=(400,300), CrossAlign=Center | childX=(400-350)/2=25, 25<30 → 被推到30 → PosInLayout=(30, 5) |
| TC-014 | `DoLayout_垂直End_右对齐` | 1个子元素DesiredSize=(100,50), Margin=(10,5,20,15) | oSize=(400,300), CrossAlign=End | childX=400-100-20=280, PosInLayout=(280, 5) |
| TC-015 | `DoLayout_垂直三子元素_纵向间距取最大值` | A=(100,50) M=(5,10,5,15); B=(200,80) M=(5,20,5,5); C=(150,40) M=(0,10,5,8), CrossAlign=Start | oSize=(400, 300) | A: Pos=(5,10), Size=(100,50); B: Pos=(5, 10+50+max(15,20)=85), Size=(200,80); C: Pos=(0, 85+80+max(5,10)=175), Size=(150,40) |

### 2.2 水平模式 × 交叉轴对齐

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-016 | `DoLayout_水平Stretch_高度拉伸填满` | 1个子元素DesiredSize=(50,100), Margin=(10,20,5,15) | oSize=(400,300), CrossAlign=Stretch | PosInLayout=(10, 20), SizeInLayout.Height=max(0, 300-20-15)=265 |
| TC-017 | `DoLayout_水平Start_顶对齐` | 1个子元素DesiredSize=(50,100), Margin=(10,20,5,5) | oSize=(400,300), CrossAlign=Start | PosInLayout=(10, 20), SizeInLayout=(50, 100) |
| TC-018 | `DoLayout_水平Center_居中` | 1个子元素DesiredSize=(50,100), Margin=(10,20,5,5) | oSize=(400,300), CrossAlign=Center | childY=(300-100)/2=100, 100≥20且100≤300-5-100=195 → PosInLayout=(10, 100) |
| TC-019 | `DoLayout_水平End_底对齐` | 1个子元素DesiredSize=(50,100), Margin=(10,20,5,5) | oSize=(400,300), CrossAlign=End | childY=300-100-5=195, PosInLayout=(10, 195) |

### 2.3 LP_None / LP_Absolute 过滤

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-020 | `DoLayout_LP_None子元素_位置和尺寸归零` | A(LP_Auto, 100×50), B(LP_None, 200×80), C(LP_Auto, 150×40) | Orientation=Vertical, CrossAlign=Start | B.PosInLayout=(0,0), B.SizeInLayout=(0,0); A和C正常排列 |
| TC-021 | `DoLayout_LP_Absolute子元素_不参与排列` | A(LP_Auto, 100×50), B(LP_Absolute, 200×80), C(LP_Auto, 150×40) | Orientation=Vertical, CrossAlign=Stretch | B不被DoLayout设置；FlushLayout已跳过LP_Absolute的FlushLayout调用，B保持原PosInLayout/SizeInLayout |

## 3. Element::GetDesiredSize

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-022 | `GetDesiredSize_返回缓存期望尺寸` | Element 已完成 UpdateDesiredSize, LocDesiredSize=(120, 60) | 调用 GetDesiredSize() | 返回 Size(120, 60) |

## 4. Margin 属性

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-023 | `GetMargin_默认值为零` | 新建 Element | GetMargin() | 返回 UnitRect(0,0,0,0) |
| TC-024 | `SetMargin_设置后正确读取` | Element | SetMargin(UnitRect(10,20,5,15)) | GetMargin() 返回 (10,20,5,15) |
| TC-025 | `SetMargin_通过SetValue设置_触发PropertyChanged` | Element | SetValue(MarginProp, Rect(5,5,5,5)) | GetMargin() 返回 (5,5,5,5) |

## 5. LookupLayoutParser（UIParser 内部）

> 注：`LookupLayoutParser` 为 UIParser.cpp 内部静态函数，通过 `ParserLayoutValue` 间接验证。

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-026 | `ParserLayoutValue_已注册名称_创建成功` | g_LayoutFactoryTable 含 "LinearLayout" | 解析 "LinearLayout" | 返回 S_OK，创建的 LinearLayout 实例有效 |
| TC-027 | `ParserLayoutValue_未注册名称_返回错误` | - | 解析 "UnknownLayout" | 返回 ERROR_BAD_FORMAT |

## 6. UIParser::ParserLayoutValue

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-028 | `ParserLayoutValue_无参数_创建默认实例` | - | 解析 "LinearLayout" | LinearLayout.Orientation=Vertical, CrossAlign=Stretch |
| TC-029 | `ParserLayoutValue_一个参数_创建指定Orientation` | - | 解析 "LinearLayout(Horizontal)" | LinearLayout.Orientation=Horizontal, CrossAlign=Stretch |
| TC-030 | `ParserLayoutValue_两个参数_创建完整配置` | - | 解析 "LinearLayout(Horizontal, Center)" | LinearLayout.Orientation=Horizontal, CrossAlign=Center |
| TC-031 | `ParserLayoutValue_多余参数_返回错误` | - | 解析 "LinearLayout(Vertical, Stretch, Extra)" | 返回 HRESULT_From_LSTATUS(ERROR_BAD_FORMAT) |
| TC-032 | `ParserLayoutValue_非法名称_返回错误` | - | 解析 "UnknownLayout" | 返回 HRESULT_From_LSTATUS(ERROR_BAD_FORMAT) |

## 7. Element::UpdateDesiredSize Layout 路径

| 编号 | 用例名称 | 前置条件 | 输入 | 预期结果 |
|------|----------|----------|------|----------|
| TC-033 | `UpdateDesiredSize_有Layout_走Layout路径` | Element 设置了 LinearLayout, bSelfLayout=false | UpdateDesiredSize(400, -1) | 调用 LinearLayout::UpdateDesiredSize，非 GetContentSize |
| TC-034 | `UpdateDesiredSize_无Layout_走GetContentSize` | Element 未设置 Layout | UpdateDesiredSize(400, -1) | 走原 GetContentSize 路径，行为不变 |

## 8. 待确认

- 以上用例假设 `UpdateDesiredSize` 可通过构造最小 Element 树测试，具体 fixture 设计在实现阶段确定
- TC-021（LP_Absolute 不参与排列）需确认 Element 框架在 FlushLayout 中是否已跳过 LP_Absolute 子元素的 FlushLayout 调用（当前代码确已跳过）
