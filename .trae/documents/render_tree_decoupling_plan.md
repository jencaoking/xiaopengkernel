# 渲染树与 DOM 树解耦实施计划

## 1. 研究总结

### 1.1 当前架构分析

经过代码库研究，发现当前 XiaopengKernel 浏览器引擎具有以下特点：

**现有模块：**
1. **DOM 模块** (`dom/dom.hpp`) - 完整的 DOM 树实现，支持节点、元素、文本等
2. **样式解析器** (`css/`) - CSS 解析、样式计算和样式表处理
3. **布局系统** (`layout/`) - LayoutBox 树和布局算法，支持 Flex、Grid 布局
4. **渲染器** (`renderer/`) - 双后端渲染（软件/OpenGL）
5. **脚本引擎** (`script/`) - QuickJS 集成与 DOM 绑定
6. **浏览器引擎** (`engine/browser_engine.hpp`) - 核心协调器

**关键发现：**
- 已存在 `renderer/render_tree.hpp` 初步实现，但未在构建过程中使用
- 当前流程：`DOM 树 → 布局树 (LayoutBox) → 渲染`
- 缺少独立的 RenderObject 树作为中间层
- DOM 树与布局树通过 LayoutTreeBuilder 直接关联
- 脏标记系统简单：只有 `m_needsLayout` 和 `m_needsPaint` 两个标记

### 1.2 已有的 RenderTree 基础

项目已经有初步的 RenderTree 框架，包含：
- `RenderObject` 类 - 带脏状态管理和 DOM 关联
- `RenderTree` 类 - 树管理和脏收集
- 基础的脏状态枚举（Clean, NeedsPaint, NeedsLayout, NeedsFullLayout）

但存在以下问题：
1. 未与样式解析器集成
2. 未与布局树构建器集成
3. 未在 BrowserEngine 中使用
4. 不支持增量布局和重绘
5. 缺少与绘画算法的集成

---

## 2. 总体设计方案

### 2.1 架构变化

**当前流程：**
```
DOM 树 
   ↓
LayoutTreeBuilder → LayoutBox 树
   ↓
PaintingAlgorithm → Canvas
```

**新流程：**
```
DOM 树 
   ↓
RenderTreeBuilder → RenderObject 树 (绑定样式)
   ↓
LayoutTreeBuilder (从 RenderObject 构建) → LayoutBox 树
   ↓
RenderTreePainter → Canvas
```

### 2.2 核心改进点

1. **RenderObject 增强**：
   - 持有 ComputedStyle（不再从 DOM 树重复计算）
   - 管理自身的 LayoutBox
   - 支持增量更新

2. **RenderTreeBuilder 新增**：
   - 从 DOM 构建 RenderObject 树
   - 集成 StyleResolver
   - 处理 display:none 等

3. **RenderTree 优化**：
   - 从 LayoutBox 获取几何信息
   - 更好的脏标记传播
   - 支持区域重绘

4. **BrowserEngine 集成**：
   - 使用 RenderTree 替代直接的 LayoutRoot
   - 实现增量更新流程

---

## 3. 详细实施步骤

### Phase 1: 完善 RenderObject 和 RenderTree

#### 3.1 增强 RenderObject 类
**文件：** `include/renderer/render_tree.hpp`

**修改内容：**
- 添加 `ComputedStyle` 成员（从 DOM 解耦样式）
- 添加 `firstChild` / `lastChild` / `nextSibling` / `previousSibling` 遍历支持
- 增强构建函数，接收样式
- 添加文本节点渲染对象支持

#### 3.2 实现 RenderTreeBuilder
**文件：** `include/renderer/render_tree_builder.hpp`（新建）
**文件：** `src/renderer/render_tree_builder.cpp`（新建）

**功能：**
- 从 DOM 树构建 RenderObject 树
- 集成 StyleResolver 计算样式
- 处理匿名块创建（inline 在 block 中）
- 跳过 display:none 元素
- 保存 DOM 节点到 RenderObject 的映射

#### 3.3 增强 RenderTree 类
**文件：** `include/renderer/render_tree.hpp`

**新增功能：**
- 集成 RenderTreeBuilder
- 维护 `DOM Node → RenderObject` 映射（快速查找）
- 支持增量 DOM 变更处理
- 从 RenderObject 树同步几何数据到 BoundingBox

---

### Phase 2: 集成布局系统

#### 2.1 新建设计器 (RenderTreeLayoutManager)
**文件：** `include/renderer/render_tree_layout_manager.hpp`（新建）
**文件：** `src/renderer/render_tree_layout_manager.cpp`（新建）

**功能：**
- 从 RenderObject 构建 LayoutBox 树（复用现有 LayoutTreeBuilder 逻辑）
- 执行增量布局（只处理脏 RenderObject）
- 将布局结果同步回 RenderObject 的 BoundingBox

#### 2.2 修改 LayoutTreeBuilder（可选）
**文件：** `include/layout/layout_tree_builder.hpp`

**调整：**
- 可选：修改为从 RenderObject 构建而不是 DOM
- 或者保留原有逻辑，但新增从 RenderObject 构建的重载

---

### Phase 3: 集成渲染系统

#### 3.1 新建 RenderTreePainter
**文件：** `include/renderer/render_tree_painter.hpp`（新建）
**文件：** `src/renderer/render_tree_painter.cpp`（新建）

**功能：**
- 从 RenderObject 树执行绘画
- 支持增量重绘（只画脏区域）
- 复用现有 PaintingAlgorithm 的功能

#### 3.2 可选：重构 PaintingAlgorithm
**文件：** `include/renderer/painting_algorithm.hpp`

**调整：**
- 提取核心绘制逻辑供 RenderTreePainter 使用
- 保留向后兼容的 API（可选）

---

### Phase 4: 集成到 BrowserEngine

#### 4.1 修改 BrowserEngine
**文件：** `include/engine/browser_engine.hpp`
**文件：** `src/engine/browser_engine.cpp`

**变更：**
- 将 `layout::LayoutBoxPtr m_layoutRoot` 替换为 `renderer::RenderTree m_renderTree`
- 更新 `buildLayout()` → `buildRenderTree()` + `layoutRenderTree()`
- 更新渲染循环以使用 RenderTreePainter
- 更新 DOM 变更回调以通知 RenderTree

#### 4.2 更新事件循环
**文件：** `src/engine/browser_engine.cpp`

**新流程：**
```cpp
if (m_needsLayout) {
    m_renderTree.layout(); // 增量布局
    m_needsLayout = false;
}
if (m_needsPaint) {
    m_renderTree.paint(canvas); // 增量重绘
    m_needsPaint = false;
}
```

---

### Phase 5: 更新构建系统

#### 5.1 更新 CMakeLists.txt
**文件：** `CMakeLists.txt`

**新增源文件：**
- `src/renderer/render_tree_builder.cpp`
- `src/renderer/render_tree_layout_manager.cpp`
- `src/renderer/render_tree_painter.cpp`
- （如果有 render_tree.cpp 的实现也要添加）

---

### Phase 6: 测试和验证

#### 6.1 新增测试
**文件：** `tests/test_render_tree.cpp`（新建）

**测试内容：**
- RenderObject 树构建
- 脏标记传播
- 增量布局
- 增量重绘

#### 6.2 更新现有测试
根据需要更新测试以适应新 API

---

## 4. 文件修改清单

### 新增文件：
1. `include/renderer/render_tree_builder.hpp`
2. `src/renderer/render_tree_builder.cpp`
3. `include/renderer/render_tree_layout_manager.hpp`
4. `src/renderer/render_tree_layout_manager.cpp`
5. `include/renderer/render_tree_painter.hpp`
6. `src/renderer/render_tree_painter.cpp`
7. `tests/test_render_tree.cpp`

### 修改文件：
1. `include/renderer/render_tree.hpp` - 增强 RenderObject 和 RenderTree
2. `include/engine/browser_engine.hpp` - 集成 RenderTree
3. `src/engine/browser_engine.cpp` - 更新实现
4. `CMakeLists.txt` - 添加新源文件

---

## 5. 风险与注意事项

### 5.1 主要风险

1. **向后兼容性**：需确保现有功能不被破坏
2. **性能影响**：新增层级可能带来微小的开销，但通过增量更新会得到更多补偿
3. **API 变化**：内部 API 变化可能影响其他模块（测试等）

### 5.2 缓解措施

1. 保留原有的 LayoutBox 和 PaintingAlgorithm 作为底层实现
2. 分阶段实施，每阶段都可独立测试
3. 充分的单元测试覆盖
4. 使用现有的示例页面作为集成测试

### 5.3 注意事项

1. 正确处理 RenderObject 与 DOM 的生命周期关系（使用 weak_ptr）
2. 正确处理匿名渲染对象（不对应 DOM 节点的）
3. 确保样式计算只在必要时发生
4. 脏标记向上传播的正确性

---

## 6. 实施优先级

**高优先级（核心功能）：**
1. 增强现有 render_tree.hpp
2. 创建 render_tree_builder.hpp/cpp
3. 集成到 BrowserEngine 的基本流程

**中优先级（增强功能）：**
4. 创建 render_tree_layout_manager
5. 创建 render_tree_painter
6. 增量布局和重绘

**低优先级（优化）：**
7. 性能优化
8. 更精细的脏标记

---

## 7. 验收标准

- [ ] 所有现有测试通过
- [ ] 渲染树正确地从 DOM 构建
- [ ] 样式正确地与 RenderObject 关联
- [ ] 布局正确执行
- [ ] 渲染与之前结果一致
- [ ] DOM 变更能正确触发展染树更新
