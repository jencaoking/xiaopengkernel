# JavaScript DOM API Binding Enhancement Plan

## 1. Current State Analysis

### Existing Bindings (dom_binding.hpp)

**Document API:**
- ✅ `getElementById(id)` - Full support
- ✅ `getElementsByTagName(name)` - Full support (returns array)
- ✅ `getElementsByClassName(className)` - Full support (returns array)
- ✅ `querySelector(selector)` - Limited (supports `#id`, tag name, `.class`)
- ✅ `querySelectorAll(selector)` - Limited (same as querySelector)
- ✅ `createElement(tagName)` - Full support
- ✅ Event methods (addEventListener, removeEventListener, dispatchEvent)

**Element API:**
- ✅ `innerHTML` - getter/setter
- ✅ `id` - getter (read-only, no setter)
- ✅ `tagName` - getter
- ✅ `textContent` - getter/setter
- ✅ `style` - getter/setter (string)
- ✅ `setAttribute(name, value)`
- ✅ `getAttribute(name)`
- ✅ `hasAttribute(name)`
- ⚠️ `removeAttribute(name)` - Implemented in DOM but not exposed
- ⚠️ `className` - Not exposed as property
- ⚠️ `classList` - Not implemented
- ✅ `getElementsByTagName(name)`
- ✅ `getElementsByClassName(className)`
- ✅ `querySelector(selector)`
- ✅ `querySelectorAll(selector)`
- ✅ `appendChild(child)`
- ✅ `removeChild(child)`
- ✅ Event methods

**Missing Bindings:**
1. Element.id setter
2. Element.className property
3. Element.classList methods (add, remove, toggle, contains, replace, etc.)
4. Element.removeAttribute
5. Element.dataset (data-* attributes)
6. Element.children / childElementCount
7. Element.contains() method
8. Element.closest() method
9. Element.innerText property
10. Element.outerHTML (getter only exists in C++)
11. Element.clientWidth / clientHeight
12. Element.offsetWidth / offsetHeight
13. Element.parentElement / parentNode
14. Element.nextElementSibling / previousElementSibling
15. Document.body (property)
16. Document.documentElement (property)
17. Document.title (property)
18. Window binding (location, navigator, etc.)
19. Node.normalize()
20. Event.currentTarget

---

## 2. Implementation Plan

### Phase 1: Core Element Property Enhancements

**Task 1.1: Add Element.id setter**

File: `include/script/dom_binding.hpp`

```cpp
// Add after element_get_id
JSAtom atom = JS_NewAtom(ctx, "id");
JS_DefinePropertyGetSet(ctx, obj, atom,
                        JS_NewCFunction(ctx, element_get_id, "get_id", 0),
                        JS_NewCFunction(ctx, element_set_id, "set_id", 1),
                        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// Add new function
static JSValue element_set_id(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    dom::Element *elem = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;
    
    std::string id = JSBinding::toStdString(ctx, argv[0]);
    elem->setId(id);
    return JS_UNDEFINED;
}
```

**Task 1.2: Add Element.className property**

File: `include/script/dom_binding.hpp`

```cpp
// Add in wrapElement()
atom = JS_NewAtom(ctx, "className");
JS_DefinePropertyGetSet(
    ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_className, "get_className", 0),
    JS_NewCFunction(ctx, element_set_className, "set_className", 1),
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// Add new functions
static JSValue element_get_className(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv);
static JSValue element_set_className(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv);
```

**Task 1.3: Add Element.removeAttribute**

File: `include/script/dom_binding.hpp`

Update `element_removeAttribute` function (currently TODO):

```cpp
static JSValue element_removeAttribute(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Element *element = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    element->removeAttribute(name);  // Now implemented in dom.hpp
    return JS_UNDEFINED;
}
```

### Phase 2: Element Navigation Properties

**Task 2.1: Add Element.children and Element.childElementCount**

File: `include/script/dom_binding.hpp`

```cpp
// Add in wrapElement()
atom = JS_NewAtom(ctx, "children");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_children, "get_children", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

atom = JS_NewAtom(ctx, "childElementCount");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_childElementCount, "get_childElementCount", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// Add new functions
static JSValue element_get_children(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    dom::Element *elem = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    
    auto children = elem->firstElementChild();
    JSValue arr = JS_NewArray(ctx);
    size_t idx = 0;
    for (auto child = children; child; child = child->nextElementSibling()) {
        JS_DefinePropertyValueUint32(ctx, arr, idx++, wrapElement(ctx, child.get()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
}

static JSValue element_get_childElementCount(JSContext *ctx, JSValueConst this_val,
                                             int argc, JSValueConst *argv) {
    dom::Element *elem = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt64(ctx, elem->childElementCount());
}
```

**Task 2.2: Add Element.parentElement**

File: `include/script/dom_binding.hpp`

```cpp
// Add in wrapElement()
atom = JS_NewAtom(ctx, "parentElement");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_parentElement, "get_parentElement", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

static JSValue element_get_parentElement(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Element *elem = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    
    auto parent = elem->parentNode();
    if (parent && parent->nodeType() == NodeType::Element) {
        return wrapElement(ctx, static_cast<dom::Element*>(parent.get()));
    }
    return JS_NULL;
}
```

### Phase 3: Document Property Enhancements

**Task 3.1: Add Document.body, Document.documentElement, Document.title**

File: `include/script/dom_binding.hpp`

Update `wrapDocument()` function to add properties:

```cpp
// Add after existing property setups
atom = JS_NewAtom(ctx, "body");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, document_get_body, "get_body", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

atom = JS_NewAtom(ctx, "documentElement");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, document_get_documentElement, "get_documentElement", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

atom = JS_NewAtom(ctx, "title");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, document_get_title, "get_title", 0),
    JS_NewCFunction(ctx, document_set_title, "set_title", 1),
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);

// Add new Document functions
static JSValue document_get_body(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv);
static JSValue document_get_documentElement(JSContext *ctx, JSValueConst this_val,
                                            int argc, JSValueConst *argv);
static JSValue document_get_title(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv);
static JSValue document_set_title(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv);
```

Add implementations:

```cpp
static JSValue document_get_body(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    dom::Document *doc = (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;
    auto body = doc->body();
    return body ? wrapElement(ctx, body.get()) : JS_NULL;
}

static JSValue document_get_documentElement(JSContext *ctx, JSValueConst this_val,
                                             int argc, JSValueConst *argv) {
    dom::Document *doc = (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;
    auto elem = doc->documentElement();
    return elem ? wrapElement(ctx, elem.get()) : JS_NULL;
}

static JSValue document_get_title(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Document *doc = (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, doc->titleText());
}

static JSValue document_set_title(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Document *doc = (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;
    if (argc < 1) return JS_EXCEPTION;
    std::string title = JSBinding::toStdString(ctx, argv[0]);
    doc->setTitleText(title);
    return JS_UNDEFINED;
}
```

### Phase 4: classList Implementation

**Task 4.1: Create ClassList binding helper**

File: `include/script/dom_binding.hpp`

Create a classList wrapper object with methods: add, remove, toggle, contains, replace, item, length

```cpp
// Add classList support - create a helper class
class ClassListBinding {
public:
    static JSValue create(JSContext *ctx, dom::Element *element) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetOpaque(obj, element);
        
        JS_SetPropertyStr(ctx, obj, "add",
            JS_NewCFunction(ctx, classList_add, "add", 1));
        JS_SetPropertyStr(ctx, obj, "remove",
            JS_NewCFunction(ctx, classList_remove, "remove", 1));
        JS_SetPropertyStr(ctx, obj, "toggle",
            JS_NewCFunction(ctx, classList_toggle, "toggle", 1));
        JS_SetPropertyStr(ctx, obj, "contains",
            JS_NewCFunction(ctx, classList_contains, "contains", 1));
        JS_SetPropertyStr(ctx, obj, "replace",
            JS_NewCFunction(ctx, classList_replace, "replace", 2));
        JS_SetPropertyStr(ctx, obj, "item",
            JS_NewCFunction(ctx, classList_item, "item", 1));
        
        // length property
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, classList_get_length, "get_length", 0),
            JS_UNDEFINED,
            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
        JS_FreeAtom(ctx, atom);
        
        return obj;
    }
    
    // Implementation functions...
};
```

Add in wrapElement():

```cpp
atom = JS_NewAtom(ctx, "classList");
JS_DefinePropertyGetSet(ctx, obj, atom,
    JS_NewCFunction(ctx, element_get_classList, "get_classList", 0),
    JS_UNDEFINED,
    JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
JS_FreeAtom(ctx, atom);
```

### Phase 5: Enhanced Event System

**Task 5.1: Add Event object creation with full properties**

File: `include/script/event_binding.hpp`

Update `createEventObject`:

```cpp
static JSValue createEventObject(JSContext *ctx, const std::string &type,
                                 bool bubbles = false, bool cancelable = false,
                                 JSValue target = JS_NULL) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "type", JS_NewString(ctx, type.c_str()));
    JS_SetPropertyStr(ctx, obj, "target", JS_IsNull(target) ? JS_NULL : JS_DupValue(ctx, target));
    JS_SetPropertyStr(ctx, obj, "currentTarget", JS_NULL);
    JS_SetPropertyStr(ctx, obj, "bubbles", JS_NewBool(ctx, bubbles));
    JS_SetPropertyStr(ctx, obj, "cancelable", JS_NewBool(ctx, cancelable));
    JS_SetPropertyStr(ctx, obj, "defaultPrevented", JS_FALSE);
    JS_SetPropertyStr(ctx, obj, "returnValue", JS_TRUE);
    JS_SetPropertyStr(ctx, obj, "eventPhase", JS_NewInt64(ctx, 0));
    JS_SetPropertyStr(ctx, obj, "timeStamp", JS_NewInt64(ctx, 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()));
    return obj;
}
```

**Task 5.2: Add EventTarget interface for Window binding**

File: `include/script/dom_binding.hpp`

Create WindowBinding class that wraps Window as EventTarget:

```cpp
class WindowBinding {
public:
    static JSClassID s_windowClassId;
    
    static void registerBinding(JSContext *ctx) {
        JS_NewClassID(&s_windowClassId);
        JSClassDef def;
        memset(&def, 0, sizeof(JSClassDef));
        def.class_name = "Window";
        JS_NewClass(JS_GetRuntime(ctx), s_windowClassId, &def);
    }
    
    static JSValue create(JSContext *ctx) {
        JSValue obj = JS_NewObjectClass(ctx, s_windowClassId);
        // Add window properties and methods
        JS_SetPropertyStr(ctx, obj, "document", ...);
        JS_SetPropertyStr(ctx, obj, "location", ...);
        JS_SetPropertyStr(ctx, obj, "navigator", ...);
        JS_SetPropertyStr(ctx, obj, "addEventListener", ...);
        JS_SetPropertyStr(ctx, obj, "removeEventListener", ...);
        JS_SetPropertyStr(ctx, obj, "dispatchEvent", ...);
        return obj;
    }
};
```

### Phase 6: CSSOM Enhancements

**Task 6.1: Add Element.style as CSSStyleDeclaration object**

File: `include/script/css_style_binding.hpp` (new file)

Create proper CSSStyleDeclaration binding:

```cpp
class CSSStyleDeclarationBinding {
public:
    static JSValue create(JSContext *ctx, dom::Element *element) {
        JSValue obj = JS_NewObject(ctx);
        JS_SetOpaque(obj, element);
        
        // Add common CSS properties as getter/setter
        JS_SetPropertyStr(ctx, obj, "color",
            JS_NewCFunction(ctx, style_get_color, "get_color", 0));
        JS_SetPropertyStr(ctx, obj, "backgroundColor", ...);
        JS_SetPropertyStr(ctx, obj, "display", ...);
        JS_SetPropertyStr(ctx, obj, "width", ...);
        JS_SetPropertyStr(ctx, obj, "height", ...);
        // ... more properties
        
        // cssText property
        JSAtom atom = JS_NewAtom(ctx, "cssText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, style_get_cssText, "get_cssText", 0),
            JS_NewCFunction(ctx, style_set_cssText, "set_cssText", 1),
            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
        JS_FreeAtom(ctx, atom);
        
        return obj;
    }
};
```

---

## 3. C++ Standards Compliance

Following `cpp-dev-standards` guidelines:

### Naming Conventions
- Functions: `element_get_id`, `element_set_id` (snake_case with prefix)
- Classes: `DOMBinding`, `EventBinding` (PascalCase)
- Member variables: Not applicable (header-only static methods)

### Modern C++ Features
- Use `std::string_view` for read-only string parameters
- Use `std::optional` for nullable returns
- RAII via `JS_DupValue`/`JS_FreeValue` for JS resource management

### Error Handling
- Return `JS_EXCEPTION` on invalid opaque access
- Return `JS_NULL` for not found results
- Return `JS_UNDEFINED` for void operations

### Code Organization
- Keep all DOM bindings in `dom_binding.hpp`
- Keep event bindings in `event_binding.hpp`
- New CSS style bindings in `css_style_binding.hpp`
- Use `#pragma once` for all headers

---

## 4. Testing Plan

Update `tests/test_dom_binding.cpp`:

```cpp
// Test new properties
int test_element_properties() {
    // Test id setter
    // Test className getter/setter
    // Test children property
    // Test parentElement property
}

int test_document_properties() {
    // Test body property
    // Test documentElement property
    // Test title getter/setter
}

int test_classlist_api() {
    // Test add/remove/toggle/contains
}

int test_query_selector_enhanced() {
    // Test complex selectors
}
```

---

## 5. Implementation Order

1. **Phase 1** - Core Element Properties (Tasks 1.1, 1.2, 1.3)
2. **Phase 2** - Element Navigation (Tasks 2.1, 2.2)
3. **Phase 3** - Document Properties (Task 3.1)
4. **Phase 4** - classList Implementation (Task 4.1)
5. **Phase 5** - Event Enhancements (Tasks 5.1, 5.2)
6. **Phase 6** - CSSOM (Task 6.1)
7. **Testing** - Update test file
8. **Build verification** - Ensure compilation and tests pass

---

## 6. Files to Modify

| File | Changes |
|------|---------|
| `include/script/dom_binding.hpp` | Add id setter, className, classList, navigation properties, document properties |
| `include/script/event_binding.hpp` | Enhance Event object, add currentTarget |
| `include/script/css_style_binding.hpp` | NEW: CSSStyleDeclaration binding |
| `include/script/window_binding.hpp` | NEW: Window/EventTarget binding |
| `tests/test_dom_binding.cpp` | Add comprehensive tests for new APIs |
