# XiaopengKernel (小鹏内核)

这是一个从零开始构建的自研轻量级浏览器/HTML渲染引擎，使用 C++20 编写，旨在探索和实现现代浏览器引擎的核心技术栈，包括 HTML/CSS 解析、CSS Flexbox 布局排版、GPU (OpenGL) 与软件渲染引擎、JavaScript 运行时集成以及基于异步多线程的资源加载与解析。

## 当前项目进度 (已完成阶段)

项目已经具备了一个渲染引擎的初步骨架，包含以下核心模块及特性：

1. **基础架构与构建系统**
   - 完善的 CMake 构建系统，支持复杂依赖管理。
   - 集成了网络请求、日志、窗口等多平台基础库（curl, SDL2 等）。
2. **多线程异步解析 (HTML/CSS)**
   - 实现了后台解析线程 (`ParseThread`)，支持边下载边解析，不阻塞主线程，提升了引擎的响应效率。
3. **样式与布局 (Layout Engine)**
   - 实现了计算样式的处理与继承体系。
   - 实现了基于 CSS Box Model 的块级、行内布局。
   - 支持了现代的 CSS Flexbox 布局，具备基础到进阶（`test_flexbox`, `test_flexbox_advanced`）的 Flex 排版能力。
4. **渲染引擎体系 (Rendering)**
   - **双渲染后端支持**：涵盖了基于内存像素缓冲的软件渲染 (`BitmapCanvas`)，以及基于 OpenGL 的 GPU 硬件加速渲染后端 (`OpenGL Canvas`)。
   - **矢量字体与文本整形**：集成了 FreeType 与 HarfBuzz，实现了复杂的矢量文本渲染和正确的文本整形。
   - **图像处理**：实现了对不同格式图片的异步解码。
5. **资源树与依赖管理 (Resource Tree)**
   - 构建了资源树数据结构 (`ResourceTree`)，用于统一调度和缓存 HTML、CSS、图片及 JS 脚本。
6. **JavaScript 引擎初步集成**
   - 引入了轻量级 JS 引擎 (QuickJS)，搭建了 C++ 层与 JS 层交互的基础结构。

---

## 🚀 未来10个阶段演进计划

为了将 `XiaopengKernel` 打造为功能完善的浏览器内核，接下来的开发路线图（Roadmap）分为10个主要演进阶段：

### 阶段 1: 完善 JavaScript DOM API 绑定
- 将核心 DOM 节点 (`Document`, `Element`, `TextNode` 等) 暴露给 QuickJS 环境。
- 实现基础 DOM 操作 API (`getElementById`, `querySelector`, `createElement`, `appendChild` 等)，让脚本能够动态修改页面。

### 阶段 2: 高级 CSS 选择器引擎与层叠上下文
- 支持更复杂的组合 CSS 选择器（伪类、伪元素、兄弟及后代选择器）。
- 完善 CSS 优先级（Specificity）的严谨计算逻辑。
- 正确处理 `z-index` 与层叠上下文 (Stacking Contexts) 的深度排序规则。

### 阶段 3: 完整事件流模型与用户交互
- 实现 W3C 标准的 DOM 事件模型（捕获阶段、目标阶段、冒泡阶段）。
- 完全接管和分发窗口 UI 事件（如 Mouse, Keyboard, Scroll, Touch），将其转化为精确的 DOM Trigger。

### 阶段 4: 渲染树解耦与增量重排/重绘机制 (Dirty Bits)
- 彻底分离 DOM Tree 与 Render Tree (或 LayoutObject Tree)。
- 建立精确的 "脏标记 (Dirty Bits)" 跟踪机制 —— 在 DOM/CSS 修改时，避免每次全局重排 (Reflow)，实施局部增量更新，大幅提升性能。

### 阶段 5: 高级布局支持架构 (Advanced Layout)
- 实现 CSS Grid 网格布局支持。
- 更复杂的文本排版能力，例如 Bidi (双向文本) 控制、复合分词换行规则、以及更为严格的块级格式化上下文 (BFC) 隔离。

### 阶段 6: 浏览上下文与网络层高级特性
- 建立完整的 "浏览上下文 (Browsing Context)" 以及页面间历史导航 (History API) 管理。
- 完善网络层的 HTTP/2、HTTP/3 策略响应与客户端资源缓存机制处理（Cache-Control/ETag）。

### 阶段 7: 事件循环 (Event Loop) 与页面生命周期
- 建立并严格遵守 WHATWG 的标准 Event Loop 模型。
- 区分 Macro Tasks（宏任务，如 setTimeout）和 Micro Tasks（微任务，如 Promise），并将渲染刷新对齐至 60/120Hz 的显示器 VSync 频率。

### 阶段 8: Web API 标准库扩展
- 依据现有渲染器封装并提供 HTML5 Canvas 2D API 供 JS 调用。
- 支持 `fetch` API 或 `XMLHttpRequest` 以提供脚本执行阶段的异步网络数据交互。

### 阶段 9: 硬件加速合成器体系 (Compositor Thread)
- 实现 Layer (层) 树的概念，根据 CSS 属性（如 `transform`, `opacity`）将页面智能划分为多个独立绘制层。
- 在独立的合成器线程 (Compositor Thread) 中调用 GPU 直接操作层图块，实现不阻塞主线程的极端流畅滚动和交互动画。

### 阶段 10: 开发者工具 (DevTools) 支持与高级调试
- 实现并暴露 Chrome DevTools Protocol (CDP) 接口或内嵌简易的 Inspector UI 审查面板。
- 支持页面 DOM 树实时审查、CSS 样式覆盖修改生效、JS 控制台交互及网络瀑布流抓包分析。
