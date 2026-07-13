# XiaopengKernel (小鹏内核)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green)](https://cmake.org/)

这是一个从零开始构建的自研轻量级浏览器/HTML渲染引擎，使用 C++20 编写，旨在探索和实现现代浏览器引擎的核心技术栈，包括 HTML/CSS 解析、CSS Flexbox/Grid 布局排版、GPU (OpenGL) 与软件渲染引擎、JavaScript 运行时集成以及基于异步多线程的资源加载与解析。

## ✨ 特性

- 🌐 **HTML/CSS 解析** - 完整的标记语言解析器，支持 WHATWG 规范的状态机
- 📐 **CSS 布局引擎** - Block/Inline/Flexbox/Grid 多种布局模式
- 🎨 **层叠上下文** - 完整的 CSS 层叠规则和 z-index 排序
- 🎮 **双渲染后端** - OpenGL GPU 加速 + 软件渲染
- 🔤 **矢量字体支持** - FreeType + HarfBuzz 文本整形
- ⚡ **多线程解析** - 异步边下载边解析
- 🧩 **JavaScript 集成** - QuickJS 轻量级 JS 引擎 + 完整 DOM API 绑定
- 🌍 **网络资源加载** - HTTP/2/3 支持，连接池，缓存，安全策略

## 📁 项目结构

```
xiaopengkernel/
├── include/            # 头文件目录
│   ├── css/           # CSS 解析与样式
│   ├── dom/           # DOM 树构建
│   ├── engine/        # 浏览器引擎核心
│   ├── layout/        # 布局算法
│   ├── loader/        # 资源加载器
│   ├── renderer/      # 渲染引擎
│   ├── script/        # JS 脚本绑定
│   └── window/        # 窗口管理
├── src/               # 源代码
├── tests/             # 单元测试 (90+ 用例)
├── demo/              # 演示页面
├── docs/              # API 文档
└── third_party/       # 第三方依赖
```

## 🔧 构建

### 前置依赖

- CMake 3.15+
- C++20 兼容编译器 (GCC 11+, Clang 14+, MSVC 2019+)
- SDL2 2.28+
- curl
- FreeType
- HarfBuzz
- QuickJS

### 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 生成构建文件
cmake ..

# 编译
cmake --build .

# 运行测试
ctest .
```

## 📊 当前项目进度

### 已完成模块

| 模块 | 状态 | 说明 |
|------|------|------|
| **HTML 解析器** | ✅ 完整 | WHATWG 规范状态机，65 个解析状态 |
| **CSS 解析器** | ✅ 基础 | 选择器解析，属性选择器，伪类支持 |
| **DOM 树** | ✅ 完整 | Element/Document/TextNode，完整 API |
| **布局引擎** | ✅ 基础 | Block/Inline/Flexbox/Grid |
| **层叠上下文** | ✅ 新增 | CSS 层叠规则，z-index 排序 |
| **渲染引擎** | ✅ 基础 | 软件渲染 + OpenGL |
| **JS 引擎** | ✅ 集成 | QuickJS + DOM API 绑定 |
| **网络加载** | ✅ 完整 | HTTP/2/3，连接池，缓存 |
| **事件系统** | ✅ 基础 | W3C 三阶段事件流 |

### 核心模块详解

#### 1. HTML/CSS 解析
- 完整实现 WHATWG 规范的 HTML 词法分析器（65 个状态）
- HTML 树构建器支持所有插入模式
- CSS 选择器支持：Tag, Class, Id, Attribute, Pseudo-class, Pseudo-element
- 属性选择器：`=`, `~=`, `|=`, `^=`, `$=`, `*=` 六种运算符
- 伪类：`:first-child`, `:last-child`, `:nth-child`, `:first-of-type`, `:lang()`, `:not()` 等

#### 2. 布局引擎
- **Block 布局**：完整 Box Model，margin/padding/border 计算
- **Inline 布局**：Line Box Model，文本换行，垂直对齐
- **Flexbox 布局**：flex-direction, justify-content, align-items 等
- **Grid 布局**：grid-template, grid-area（基础实现）
- **层叠上下文**：13 种触发条件，7 步绘制顺序

#### 3. 渲染引擎
- **软件渲染**：基于像素缓冲的 Canvas，支持抗锯齿
- **OpenGL 渲染**：GPU 加速，帧缓冲区管理
- **矢量字体**：FreeType 光栅化 + HarfBuzz 文本整形
- **图层管理**：PaintLayer 支持负/正 z-index 分组

#### 4. JavaScript 集成
- **Document API**：body, head, title, URL, readyState
- **创建方法**：createElement, createTextNode, createDocumentFragment
- **Node 遍历**：parentNode, childNodes, children, nextSibling 等
- **classList API**：add, remove, toggle, contains, replace
- **DOM 操作**：insertBefore, replaceChild, remove, cloneNode
- **匹配方法**：contains, isEqualNode, matches, closest

#### 5. 网络加载
- HTTP/2, HTTP/3 协议支持
- 连接池管理
- 缓存策略（Cache-Control, ETag）
- 安全策略（Same-Origin, CORS）
- 资源优先级调度

## 🧪 测试

项目包含 90+ 测试用例，覆盖：

| 测试文件 | 用例数 | 覆盖范围 |
|----------|--------|----------|
| test_dom_binding.cpp | 40+ | Document/Element/Node/classList 全 API |
| test_phase2_selectors_stacking.cpp | 27 | 属性选择器 + 伪类 + 层叠上下文 |
| test_html_parser.cpp | 5+ | HTML 解析 |
| test_css_parser.cpp | 5+ | CSS 解析 |
| test_flexbox.cpp | 5+ | Flexbox 布局 |
| test_grid.cpp | 3+ | Grid 布局 |
| test_positioning.cpp | 5+ | 定位 |
| test_event.cpp | 3+ | 事件系统 |
| 其他 | 10+ | 渲染/资源/线程 |

## 🚀 未来演进计划

### 阶段 1-2: 已完成 ✅
- 完善 JavaScript DOM API 绑定
- 高级 CSS 选择器引擎与层叠上下文

### 阶段 3: 完整事件流模型与用户交互
- 实现 W3C 标准的 DOM 事件模型（捕获阶段、目标阶段、冒泡阶段）
- 完全接管和分发窗口 UI 事件（Mouse, Keyboard, Scroll, Touch）

### 阶段 4: 渲染树解耦与增量重排/重绘
- 彻底分离 DOM Tree 与 Render Tree
- 建立精确的 "脏标记 (Dirty Bits)" 跟踪机制
- 实施局部增量更新

### 阶段 5: 高级布局支持
- 完善 CSS Grid 网格布局
- Bidi (双向文本) 控制
- 块级格式化上下文 (BFC) 隔离

### 阶段 6: 浏览上下文与网络层
- 完整的 History API 管理
- HTTP/2 Server Push
- Service Worker 支持

### 阶段 7: 事件循环与页面生命周期
- 严格遵守 WHATWG Event Loop 规范
- requestAnimationFrame 对齐 VSync

### 阶段 8: Web API 扩展
- Canvas 2D API
- fetch API / XMLHttpRequest
- Web Workers

### 阶段 9: 硬件加速合成器
- Layer 树概念
- 独立合成器线程
- 瓦片化光栅化

### 阶段 10: 开发者工具
- Chrome DevTools Protocol
- Inspector UI
- Performance 面板

## 🏗️ 架构图

```
┌─────────────────────────────────────────────────────────┐
│                    BrowserEngine                         │
├─────────┬─────────┬─────────┬─────────┬─────────────────┤
│   DOM   │   CSS   │ Layout  │ Render  │     Script      │
│  Parser │ Resolver│ Engine  │  Engine │  (QuickJS)      │
├─────────┴─────────┴─────────┴─────────┴─────────────────┤
│                    Loader Layer                          │
│  HttpClient │ ConnectionPool │ Cache │ Scheduler         │
├─────────────────────────────────────────────────────────┤
│                  Platform Layer                          │
│      SDL2       │    OpenGL    │    FreeType/HarfBuzz    │
└─────────────────────────────────────────────────────────┘
```

## 📝 开发日志

### 2026-07-14
- 新增完整属性选择器支持（=, ~=, |=, ^=, $=, *=）
- 新增伪类：first-of-type, last-of-type, only-of-type, nth-last-of-type, lang()
- 新增 StackingContext 构建器，实现 CSS 2.1 层叠规则
- 新增 PaintLayer 图层管理系统
- 重写 DOM 绑定，实现完整 JS DOM API（40+ 测试用例）
- 新增层叠上下文测试（27 测试用例）

## 📄 许可证

本项目基于 [Apache License 2.0](LICENSE) 开源。
