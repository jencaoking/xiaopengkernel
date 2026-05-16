# JavaScript DOM API 绑定完善计划

## 1. 研究总结

### 1.1 当前实现分析

通过分析 [dom_binding.hpp](file:///workspace/include/script/dom_binding.hpp)，当前已实现的 DOM API 绑定包括：

**Element 属性 (已实现)**
- ✅ innerHTML (读写)
- ✅ id, tagName, className, textContent
- ✅ style (简化实现)
- ✅ parentNode, childNodes, children
- ✅ firstElementChild, lastElementChild
- ✅ nextElementSibling, previousElementSibling
- ✅ childElementCount, ownerDocument, nodeType, nodeName

**Element 方法 (已实现)**
- ✅ setAttribute, getAttribute, hasAttribute, removeAttribute
- ✅ getElementsByTagName, getElementsByClassName
- ✅ querySelector, querySelectorAll
- ✅ appendChild, removeChild, insertBefore, replaceChild
- ✅ cloneNode, matches, closest, hasChildNodes
- ✅ normalize, contains
- ✅ addEventListener, removeEventListener, dispatchEvent

**Element 特殊对象 (已实现)**
- ✅ classList (add, remove, toggle, contains)

**Document 方法 (已实现)**
- ✅ getElementById, createElement
- ✅ getElementsByTagName, getElementsByClassName
- ✅ querySelector, querySelectorAll

### 1.2 当前代码质量问题

根据 C++ 开发标准分析，当前实现存在以下问题：

1. **内存管理不规范** - 使用裸指针 `Element*` 而非智能指针
2. **错误处理不完善** - 缺乏统一的错误类型
3. **命名不一致** - 混合使用下划线和驼峰命名
4. **类型安全不足** - 大量使用 `void*` 和 C 风格转换
5. **代码重复严重** - 大量相似的 getter/setter 实现
6. **缺少现代 C++ 特性** - 未使用 `std::optional`, `std::variant` 等

---

## 2. 总体设计方案

### 2.1 设计目标

1. **完善缺失的 DOM API** - 实现关键的 Web API
2. **遵循 C++ 现代标准** - 应用 C++11/14/17/20 最佳实践
3. **提高代码质量** - 减少重复、增强类型安全
4. **改善错误处理** - 使用统一的错误报告机制

### 2.2 新增 API 优先级

**高优先级 (核心功能)**
1. Element.dataset 属性
2. Element.scrollTop, scrollLeft, scrollWidth, scrollHeight
3. Element.clientWidth, clientHeight, offsetWidth, offsetHeight
4. Text 节点支持 (splitText, wholeText)
5. Document.createTextNode, createComment
6. DocumentFragment 支持

**中优先级 (常用功能)**
1. Element.scroll, scrollTo, scrollBy 方法
2. Element.blur, focus 方法
3. Element.click 方法
4. Element.offsetParent, offsetTop, offsetLeft
5. Element.getBoundingClientRect

**低优先级 (边缘功能)**
1. 命名空间支持 (getAttributeNS, etc.)
2. MutationObserver
3. Range/Selection API

---

## 3. 详细实施步骤

### Phase 1: 代码重构 - 建立基础框架

#### 1.1 创建辅助函数减少重复代码
**文件：** `include/script/dom_binding.hpp`

**新增内容：**
```cpp
namespace detail {
    // 获取 Element 的辅助函数
    template<typename Func>
    JSValue executeIfValid(JSContext* ctx, JSValueConst this_val, 
                         JSClassID class_id, Func&& func) {
        auto* element = static_cast<dom::Element*>(
            JS_GetOpaque(this_val, class_id));
        if (!element) {
            return JS_EXCEPTION;
        }
        return func(element);
    }
    
    // 获取可选参数的辅助函数
    template<typename T>
    std::optional<T> getOptionalArg(JSContext* ctx, JSValueConst* argv, 
                                    int argc, int index) {
        if (index >= argc) {
            return std::nullopt;
        }
        // 实现类型转换逻辑
    }
}
```

#### 1.2 添加命名空间别名
**文件：** `include/script/dom_binding.hpp`

```cpp
namespace script = xiaopeng::script;
namespace dom = xiaopeng::dom;
using JSContextPtr = JSContext*;
using JSValuePtr = JSValue;
```

---

### Phase 2: 实现 Element.dataset 属性

#### 2.1 添加 dataset 辅助类
**文件：** `include/script/dom_binding.hpp`

```cpp
class DatasetBinding {
public:
    static JSValue create(JSContext* ctx, dom::Element* element) {
        auto obj = JS_NewObject(ctx);
        // 实现 data-* 属性的 getter/setter
        return obj;
    }
};
```

#### 2.2 在 wrapElement 中注册 dataset
**文件：** `include/script/dom_binding.hpp`

```cpp
// dataset property
JSValue datasetObj = DatasetBinding::create(ctx, libraryElem);
JS_SetPropertyStr(ctx, obj, "dataset", datasetObj);
```

---

### Phase 3: 实现尺寸相关属性

#### 3.1 实现 scroll 相关属性
**文件：** `include/script/dom_binding.hpp`

```cpp
// element.scrollTop (getter/setter)
atom = JS_NewAtom(ctx, "scrollTop");
JS_DefinePropertyGetSet(
    ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_scrollTop, "get_scrollTop", 0),
    JS_NewCFunction(ctx, element_set_scrollTop, "set_scrollTop", 1),
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// element.scrollLeft (getter/setter)
atom = JS_NewAtom(ctx, "scrollLeft");
// ... similar implementation
```

#### 3.2 实现 client/offset 相关属性
**文件：** `include/script/dom_binding.hpp`

```cpp
// element.clientWidth (read-only)
atom = JS_NewAtom(ctx, "clientWidth");
JS_DefinePropertyGetSet(
    ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_clientWidth, "get_clientWidth", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// element.clientHeight (read-only)
// element.offsetWidth (read-only)
// element.offsetHeight (read-only)
```

---

### Phase 4: 实现 Text 节点支持

#### 4.1 创建 Text 节点类绑定
**文件：** `include/script/dom_binding.hpp`

```cpp
class TextBinding {
public:
    static void registerBinding(JSContext* ctx) {
        JS_NewClassID(&s_textClassId);
        JSClassDef textDef;
        memset(&textDef, 0, sizeof(JSClassDef));
        textDef.class_name = "Text";
        JS_NewClass(JS_GetRuntime(ctx), s_textClassId, &textDef);
    }
    
    static JSValue wrapText(JSContext* ctx, dom::TextNode* text) {
        JSValue obj = JS_NewObjectClass(ctx, s_textClassId);
        JS_SetOpaque(obj, text);
        
        // textContent property
        // splitText method
        // wholeText property (if supported)
        
        return obj;
    }
    
private:
    static JSClassID s_textClassId;
};
```

#### 4.2 在 DOMBinding 中注册 Text
**文件：** `include/script/dom_binding.hpp`

```cpp
static void registerBinding(JSContext* ctx) {
    DOMBinding::registerBinding(ctx);  // Element, Document
    TextBinding::registerBinding(ctx);  // NEW: Text
}
```

---

### Phase 5: 实现 Document.createTextNode 等

#### 5.1 在 Document 绑定中添加方法
**文件：** `include/script/dom_binding.hpp`

```cpp
// document.createTextNode(data)
JS_SetPropertyStr(
    ctx, obj, "createTextNode",
    JS_NewCFunction(ctx, document_createTextNode, "createTextNode", 1));

// document.createComment(data)
JS_SetPropertyStr(
    ctx, obj, "createComment",
    JS_NewCFunction(ctx, document_createComment, "createComment", 1));

// document.createDocumentFragment()
JS_SetPropertyStr(
    ctx, obj, "createDocumentFragment",
    JS_NewCFunction(ctx, document_createDocumentFragment, "createDocumentFragment", 0));
```

#### 5.2 实现创建函数
**文件：** `include/script/dom_binding.hpp`

```cpp
static JSValue document_createTextNode(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    auto* doc = static_cast<dom::Document*>(
        JS_GetOpaque(this_val, s_documentClassId));
    if (!doc || argc < 1) {
        return JS_EXCEPTION;
    }
    
    std::string data = JSBinding::toStdString(ctx, argv[0]);
    auto textNode = doc->createTextNode(data);
    
    return TextBinding::wrapText(ctx, textNode.get());
}
```

---

### Phase 6: 实现 Element 尺寸方法

#### 6.1 添加 scroll/scrollTo/scrollBy 方法
**文件：** `include/script/dom_binding.hpp`

```cpp
// element.scroll(x, y)
JS_SetPropertyStr(
    ctx, obj, "scroll",
    JS_NewCFunction(ctx, element_scroll, "scroll", 2));

// element.scrollTo(x, y) 
JS_SetPropertyStr(
    ctx, obj, "scrollTo",
    JS_NewCFunction(ctx, element_scrollTo, "scrollTo", 2));

// element.scrollBy(x, y)
JS_SetPropertyStr(
    ctx, obj, "scrollBy",
    JS_NewCFunction(ctx, element_scrollBy, "scrollBy", 2));
```

#### 6.2 添加 focus/blur/click 方法
**文件：** `include/script/dom_binding.hpp`

```cpp
// element.focus()
JS_SetPropertyStr(
    ctx, obj, "focus",
    JS_NewCFunction(ctx, element_focus, "focus", 0));

// element.blur()
JS_SetPropertyStr(
    ctx, obj, "blur",
    JS_NewCFunction(ctx, element_blur, "blur", 0));

// element.click()
JS_SetPropertyStr(
    ctx, obj, "click",
    JS_NewCFunction(ctx, element_click, "click", 0));
```

---

### Phase 7: 实现 getBoundingClientRect

#### 7.1 添加 getBoundingClientRect 方法
**文件：** `include/script/dom_binding.hpp`

```cpp
// element.getBoundingClientRect()
JS_SetPropertyStr(
    ctx, obj, "getBoundingClientRect",
    JS_NewCFunction(ctx, element_getBoundingClientRect, "getBoundingClientRect", 0));
```

#### 7.2 实现返回 DOMRect 对象
**文件：** `include/script/dom_binding.hpp`

```cpp
static JSValue element_getBoundingClientRect(JSContext* ctx, JSValueConst this_val,
                                            int argc, JSValueConst* argv) {
    auto* element = static_cast<dom::Element*>(
        JS_GetOpaque(this_val, s_elementClassId));
    if (!element) {
        return JS_EXCEPTION;
    }
    
    // 创建 DOMRect 对象
    JSValue rect = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, rect, "x", JS_NewFloat64(ctx, 0));
    JS_SetPropertyStr(ctx, rect, "y", JS_NewFloat64(ctx, 0));
    JS_SetPropertyStr(ctx, rect, "width", JS_NewFloat64(ctx, 100));
    JS_SetPropertyStr(ctx, rect, "height", JS_NewFloat64(ctx, 100));
    JS_SetPropertyStr(ctx, rect, "top", JS_NewFloat64(ctx, 0));
    JS_SetPropertyStr(ctx, rect, "right", JS_NewFloat64(ctx, 100));
    JS_SetPropertyStr(ctx, rect, "bottom", JS_NewFloat64(ctx, 100));
    JS_SetPropertyStr(ctx, rect, "left", JS_NewFloat64(ctx, 0));
    
    return rect;
}
```

---

### Phase 8: 代码清理和优化

#### 8.1 移除代码重复
**文件：** `include/script/dom_binding.hpp`

提取公共模式到辅助函数：
- `getElementFromThis()`
- `checkArguments()`
- `createReturnValue()`

#### 8.2 添加 const 正确性
**文件：** `include/script/dom_binding.hpp`

确保所有只读方法标记为 const：
```cpp
static JSValue element_get_id(JSContext* ctx, JSValueConst this_val,
                              int /*argc*/, JSValueConst* /*argv*/) const {
    // ...
}
```

---

## 4. 文件修改清单

### 修改文件：
1. `include/script/dom_binding.hpp` - 主要修改，添加新 API 和重构

### 新增文件（可选）：
1. `include/script/dom_rect.hpp` - DOMRect/DOMRectList 实现
2. `include/script/text_binding.hpp` - Text 节点绑定（可选分离）

---

## 5. C++ 编码标准合规性检查

### 5.1 现代 C++ 特性
- [x] 使用 `std::make_shared` 和 `std::make_unique`
- [ ] 使用 `std::optional` 处理可选值
- [ ] 使用 `std::variant` 处理类型联合
- [ ] 使用 `std::string_view` 减少字符串拷贝
- [ ] 使用范围 for 循环
- [ ] 使用结构化绑定

### 5.2 命名规范
- [x] 类名：PascalCase
- [ ] 变量名：snake_case
- [ ] 私有成员：m_ 前缀
- [ ] 常量：UPPER_SNAKE_CASE

### 5.3 内存管理
- [x] 使用智能指针管理 DOM 节点
- [ ] 使用 RAII 管理 QuickJS 值
- [ ] 避免裸指针

### 5.4 错误处理
- [ ] 使用异常处理错误
- [ ] 统一错误返回机制
- [ ] 记录错误日志

---

## 6. 风险与注意事项

### 6.1 主要风险

1. **向后兼容性** - 新增 API 可能与现有代码冲突
2. **性能影响** - 过度封装可能导致性能下降
3. **QuickJS 限制** - 某些 Web API 可能需要模拟实现

### 6.2 缓解措施

1. 分阶段实施，每阶段独立测试
2. 保持现有 API 签名不变
3. 提供向后兼容的包装器

### 6.3 注意事项

1. 确保线程安全（QuickJS 不是线程安全的）
2. 正确处理 JS 值的生命周期
3. 避免内存泄漏

---

## 7. 实施优先级

**Phase 1: 基础框架 (1-2天)**
1. 创建辅助函数
2. 添加命名空间别名
3. 减少代码重复

**Phase 2: 核心 API (2-3天)**
1. Element.dataset
2. Element.scrollTop/Left/Width/Height
3. Element.clientWidth/Height, offsetWidth/Height

**Phase 3: Text 节点支持 (1-2天)**
1. TextBinding 类
2. Document.createTextNode/createComment
3. Text.splitText

**Phase 4: 尺寸方法 (1天)**
1. Element.scroll/scrollTo/scrollBy
2. Element.focus/blur/click
3. Element.getBoundingClientRect

**Phase 5: 代码清理 (1天)**
1. 重构和优化
2. 添加测试
3. 文档更新

---

## 8. 验收标准

- [ ] 所有现有测试通过
- [ ] 新增 API 正确实现
- [ ] 代码符合 C++ 编码标准
- [ ] 性能无明显下降
- [ ] 文档更新

## 9. 参考资料

- [MDN Web Docs - Element](https://developer.mozilla.org/en-US/docs/Web/API/Element)
- [MDN Web Docs - Document](https://developer.mozilla.org/en-US/docs/Web/API/Document)
- [MDN Web Docs - Text](https://developer.mozilla.org/en-US/docs/Web/API/Text)
- [QuickJS Documentation](https://bellard.org/quickjs/quickjs.html)
