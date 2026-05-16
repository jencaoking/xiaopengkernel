# CSS 引擎 Bug 修复计划

## 概述
本计划针对 CSS 引擎中发现的关键 bug 进行修复，按优先级排序。主要问题包括 Token 空白丢弃、EOF 无限循环、组合子逻辑错误等。

## 修复步骤

### 第一阶段：P0 严重 Bug 修复

#### Bug 1: Token 空白丢弃导致后代组合子无法识别

**问题位置**: `include/css/css_tokenizer.hpp` 第 25-27 行

**问题描述**: 
- Tokenizer 丢弃所有 `Whitespace` 类型的 token
- Parser 中多处依赖 `peek().is(TokenType::Whitespace)` 来检测后代组合子
- 导致 `div span` 等选择器无法正确解析

**修复方案**:
1. 修改 `tokenize()` 方法，不再丢弃空白 token
2. 或者修改 parser 中的后代组合子检测逻辑

**具体步骤**:
1. 在 `css_tokenizer.hpp` 的 `tokenize()` 方法中，移除对 `Whitespace` token 的过滤
2. 验证所有解析器代码能正确处理 `Whitespace` token
3. 添加单元测试验证后代选择器（如 `div span`）能正确解析

---

#### Bug 2: parseDeclaration 中 EOF 无限循环

**问题位置**: `include/css/css_parser.hpp` 的 `parseDeclaration` 方法

**问题描述**:
- 循环条件缺少对 `EndOfFile` 的检查
- 当 `position_ == tokens_.size()` 时，`peek()` 返回 EOF，但 `consume()` 不推进 position
- 导致无限循环

**修复方案**:
1. 在循环条件中添加 `!peek().is(TokenType::EndOfFile)` 检查

**具体步骤**:
1. 找到 `parseDeclaration` 中的 while 循环
2. 修改条件为：
   ```cpp
   while (position_ < tokens_.size() && 
          !peek().is(TokenType::EndOfFile) &&
          !peek().is(TokenType::Semicolon) &&
          !peek().is(TokenType::CloseCurly))
   ```
3. 添加边界检查测试用例

---

### 第二阶段：P1 重要 Bug 修复

#### Bug 3: CSS 变量 var() 未被解析

**问题位置**: `include/css/style_resolver.hpp` 的 `applyDeclarations` 方法

**问题描述**:
- `resolveCSSVariable()` 函数已实现但未被调用
- CSS 变量不会被解析，`var(--foo)` 会保持原样

**修复方案**:
1. 在 `applyDeclarations` 中对每个属性值调用 `resolveCSSVariable()`
2. 确保在解析具体属性值之前先解析 CSS 变量

**具体步骤**:
1. 找到 `applyDeclarations` 方法中处理属性值的代码
2. 在解析前调用 `resolveCSSVariable()` 预处理值
3. 添加测试用例验证 CSS 变量替换功能

---

#### Bug 4: 组合子处理逻辑错误

**问题位置**: `include/css/css_parser.hpp` 的 `parseSelector` 方法

**问题描述**:
- 复合选择器（如 `div.class`）处理时错误地压入 `Combinator::None`
- 导致 `combinators.size() != parts.size() - 1`
- 匹配时可能越界访问

**修复方案**:
1. 重新设计复合选择器的表示方式
2. 确保 `combinators` 向量大小始终正确

**具体步骤**:
1. 分析当前 `parseSelector` 中复合选择器的处理逻辑
2. 修改为：将复合选择器内部的多个简单选择器作为同一个 `parts` 条目
3. 或使用 `CompoundSelector` 结构体替代
4. 添加测试用例验证复合选择器匹配

---

#### Bug 5: 伪类参数丢失

**问题位置**: `include/css/css_parser.hpp` 的 `parseSelector` 方法

**问题描述**:
- 解析函数型伪类（如 `:nth-child(2n+1)`）时只保存函数名
- 函数参数被跳过，导致参数丢失

**修复方案**:
1. 在解析 `TokenType::Function` 时，保存完整的函数调用字符串

**具体步骤**:
1. 在 `parseSelector` 中修改伪类解析逻辑
2. 将函数名和参数拼接为完整字符串，如 `"nth-child(2n+1)"`
3. 添加测试用例验证 `:nth-child`、`:not` 等带参数伪类

---

### 第三阶段：代码质量改进

#### 改进 1: 添加边界检查和断言

**位置**: 多处

**改进内容**:
1. 在 `matchRecursively` 中添加 `combinators` 大小的断言
2. 在 `parseSelector` 后添加验证，确保 `combinators.size() == parts.size() - 1`
3. 添加防御性编程检查

#### 改进 2: 字符串拷贝优化

**位置**: `ComputedStyle` 和相关类

**改进内容**:
1. 考虑使用 `std::string_view` 替代 `std::string` 减少拷贝
2. 使用移动语义优化临时对象

---

### 测试计划

1. **单元测试**:
   - 测试后代选择器 `div span`
   - 测试复合选择器 `div.class`
   - 测试带参数伪类 `:nth-child(2n+1)`
   - 测试 CSS 变量 `var(--color)`
   - 测试 EOF 边界情况

2. **集成测试**:
   - 测试复杂 CSS 规则解析
   - 测试选择器优先级计算

---

## 实施顺序

1. Bug 1 (Token 空白丢弃) - 最高优先级
2. Bug 2 (EOF 无限循环) - 最高优先级
3. Bug 3 (CSS 变量未解析) - 重要
4. Bug 4 (组合子逻辑) - 重要
5. Bug 5 (伪类参数) - 重要
6. 代码质量改进 - 后续优化

## 预期结果

修复完成后：
- ✅ 所有类型的选择器（包括后代、复合、带参数伪类）能正确解析和匹配
- ✅ CSS 变量 `var()` 能正确解析和替换
- ✅ 解析器在遇到 EOF 时能正确退出，不会卡死
- ✅ 代码添加了适当的边界检查，减少潜在崩溃风险
