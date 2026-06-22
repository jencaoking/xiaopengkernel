# XiaopengKernel 项目深度分析报告

> [!NOTE]
> 本报告基于对 `xiaopengkernel` 源码库（约 2 万行核心 C++ 代码，不含 QuickJS 等第三方依赖）的静态分析。评估目标为“距离一个可用的正式版 (v1.0) 还差多少”。

## 1. 核心架构与现状 (The Good)

目前项目已经搭建了一个非常完整且规范的现代浏览器引擎骨架。作为一个个人或小型团队发起的项目，其架构深度令人惊叹：

* **多线程架构 (Pipeline)：** 拥有主线程（事件循环/UI渲染）和后台解析线程 (`ParseThread`)，这是现代浏览器防止白屏假死的基础。
* **DOM & CSS 解析：** 手写了词法/语法分析器 (`HtmlParser`, `CssParser`)，支持基础的选择器匹配 (`StyleResolver`) 和层叠优先级处理。
* **布局引擎 (Layout Engine)：** 实现了 BFC (块级格式化上下文)、部分 IFC (内联)，并开始探索现代化的 `Flexbox` 和 `Grid` 布局。这是一个巨大的里程碑。
* **渲染管线 (Graphics)：** 提供了软件渲染 (`BitmapCanvas`) 和基于 OpenGL 的硬件加速渲染 (`OpenGLCanvas`) 双引擎支持，集成了 FreeType 字体渲染引擎。
* **JavaScript 绑定 (QuickJS)：** 引入了 QuickJS，并已经成功桥接了基础的 DOM 操作 API (`dom_binding.cpp`) 和异步机制 (`promise_binding.cpp`)。
* **网络与资源获取 (Networking)：** 搭建了 `Loader` 模块，包含连接池、HTTP 客户端和初步的安全机制设计。

## 2. 距离“正式版”的核心差距 (The Gap to v1.0)

尽管骨架完备，但要达到能渲染大部分现代网页（如简单的 Vue/React 站点）的“正式版”，还需要填补以下深坑：

### 🚨 高优先级 (Blockers for v1.0)

1. **内联排版系统的重构 (IFC 完善)：**
   * *现状：* 目前的 `layoutInline` 被极大简化，跳过了非文本的内联元素（如 `<span>` 中嵌套图片或其他格式）。
   * *目标：* 需要实现完整的线盒模型 (Line Box Model)，处理断行 (Line Breaking)、内联块 (`inline-block`) 以及垂直对齐 (`vertical-align`)。这是文字和图片混排正常显示的核心。
2. **盒模型与浮动 (Floats & Stacking Context)：**
   * *现状：* 支持绝对定位，但缺乏完善的层叠上下文 (z-index) 深度排序和脱离文档流的 `float` 支持。
   * *目标：* 完善层叠树 (`RenderTree` / `PaintTree`) 的构建，按 z-index 顺序绘制。
3. **DOM API 完善与事件冒泡机制：**
   * *现状：* DOM API 停留在基础的树遍历和内容读取。
   * *目标：* 实现标准 DOM Event 规范（捕获、目标、冒泡三个阶段），完成 `addEventListener` 在 C++ 和 QuickJS 之间的完美生命周期管理（解决我们之前讨论的循环引用）。

### ⚠️ 中优先级 (Features for Beta)

1. **CSS 复杂选择器与继承：**
   * *现状：* 解析了基础规则，但对伪类 (`:hover`, `:active`)、伪元素 (`::before`) 和一些继承属性 (`inherit`) 支持不够。
   * *目标：* 完善选择器引擎，处理样式重算 (Style Recalculation) 的增量更新。
2. **异步网络流 (Streaming Fetch)：**
   * *现状：* 目前看似是一次性把 HTML 加载完再解析。
   * *目标：* 现代浏览器是边下载、边解析、边渲染的 (Streaming Parsing)。需要把 Loader 与 HtmlParser 改造成基于流 (Chunk) 的驱动模式。
3. **Web API 补全：**
   * *现状：* 只有 DOM 绑定和 Promise。
   * *目标：* 需要实现 `fetch` API，`setTimeout/setInterval` 完美接入主线程 EventLoop，以及 `XMLHttpRequest`。

## 3. 路线图预估 (Roadmap to 1.0)

保守估计，如果你是一个人全职开发：

| 阶段 | 目标里程碑 | 预估工期 |
| :--- | :--- | :--- |
| **Alpha 版** | **Acid1 测试及格。** 完善 IFC (文本排版、图文混排)、支持层叠上下文、完成 `display: flex` 的剩余用例。 | 1.5 - 2 个月 |
| **Beta 版** | **可交互。** 完善 JS 闭包垃圾回收与 DOM 事件流引擎，能够跑通原生的计数器 (Counter) 等微型交互式 JS 代码。 | 1 - 2 个月 |
| **v1.0 Release** | **框架支持。** 引入 Fetch API 和微任务队列，能够加载并渲染极简版的 Vue 3 (不带复杂 CSS 动画的静态模板编译版)。 | 2 - 3 个月 |

**总结：** 距离一个“极简但在真实世界能跑”的正式版，大约还有 **5 - 7 个月** 的集中开发量。你的底子打得非常扎实（特别是架构的分层极其清晰），坚持下去，这绝对是一个极具技术含量的史诗级开源项目！
