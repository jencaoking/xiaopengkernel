#pragma once

#include <cstring> // For memset
#include <dom/dom.hpp>
#include <dom/html_parser.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <quickjs.h>
#include <script/event_binding.hpp>
#include <script/js_binding.hpp>

namespace xiaopeng {
namespace script {

class DOMBinding {
public:
  static void registerBinding(JSContext *ctx) {
    JS_NewClassID(&s_elementClassId);
    JS_NewClassID(&s_documentClassId);

    // Define Element class
    JSClassDef elementDef;
    memset(&elementDef, 0, sizeof(JSClassDef));
    elementDef.class_name = "Element";
    elementDef.finalizer = [](JSRuntime * /*rt*/, JSValue /*val*/) {
      // We don't own the DOM pointer
    };
    JS_NewClass(JS_GetRuntime(ctx), s_elementClassId, &elementDef);

    // Define Document class
    JSClassDef docDef;
    memset(&docDef, 0, sizeof(JSClassDef));
    docDef.class_name = "Document";
    docDef.finalizer = [](JSRuntime * /*rt*/, JSValue /*val*/) {
      // We don't own the Document pointer
    };
    JS_NewClass(JS_GetRuntime(ctx), s_documentClassId, &docDef);
  }

  // Create a JS object wrapping a DOM Document
  static JSValue wrapDocument(JSContext *ctx,
                              std::shared_ptr<dom::Document> doc) {
    JSValue obj = JS_NewObjectClass(ctx, s_documentClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, doc.get());

    // Add 'getElementById' method
    JS_SetPropertyStr(
        ctx, obj, "getElementById",
        JS_NewCFunction(ctx, document_getElementById, "getElementById", 1));

    // Add 'getElementsByTagName' method
    JS_SetPropertyStr(
        ctx, obj, "getElementsByTagName",
        JS_NewCFunction(ctx, document_getElementsByTagName, "getElementsByTagName", 1));

    // Add 'getElementsByClassName' method
    JS_SetPropertyStr(
        ctx, obj, "getElementsByClassName",
        JS_NewCFunction(ctx, document_getElementsByClassName, "getElementsByClassName", 1));

    // Add 'querySelector' method
    JS_SetPropertyStr(
        ctx, obj, "querySelector",
        JS_NewCFunction(ctx, document_querySelector, "querySelector", 1));

    // Add 'querySelectorAll' method
    JS_SetPropertyStr(
        ctx, obj, "querySelectorAll",
        JS_NewCFunction(ctx, document_querySelectorAll, "querySelectorAll", 1));

    // Add 'createElement' method
    JS_SetPropertyStr(
        ctx, obj, "createElement",
        JS_NewCFunction(ctx, document_createElement, "createElement", 1));

    // Add event methods on document (targeting the document element)
    JS_SetPropertyStr(
        ctx, obj, "addEventListener",
        JS_NewCFunction(ctx, element_addEventListener, "addEventListener", 2));
    JS_SetPropertyStr(ctx, obj, "removeEventListener",
                      JS_NewCFunction(ctx, element_removeEventListener,
                                      "removeEventListener", 2));
    JS_SetPropertyStr(
        ctx, obj, "dispatchEvent",
        JS_NewCFunction(ctx, element_dispatchEvent, "dispatchEvent", 1));

    return obj;
  }

  // Create a JS object wrapping a DOM Element
  static JSValue wrapElement(JSContext *ctx, dom::Element *element) {
    if (!element)
      return JS_NULL;

    JSValue obj = JS_NewObjectClass(ctx, s_elementClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, element);

    // Add properties
    // innerHTML (getter/setter)
    JSAtom atom = JS_NewAtom(ctx, "innerHTML");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_innerHTML, "get_innerHTML", 0),
        JS_NewCFunction(ctx, element_set_innerHTML, "set_innerHTML", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // id (getter)
    atom = JS_NewAtom(ctx, "id");
    JS_DefinePropertyGetSet(ctx, obj, atom,
                            JS_NewCFunction(ctx, element_get_id, "get_id", 0),
                            JS_UNDEFINED, // Read-only
                            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // tagName (getter)
    atom = JS_NewAtom(ctx, "tagName");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_tagName, "get_tagName", 0),
        JS_UNDEFINED, // Read-only
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // textContent (getter/setter)
    atom = JS_NewAtom(ctx, "textContent");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_textContent, "get_textContent", 0),
        JS_NewCFunction(ctx, element_set_textContent, "set_textContent", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // style (getter/setter) - simplified to string
    atom = JS_NewAtom(ctx, "style");
    JS_DefinePropertyGetSet(
        ctx, obj, atom, JS_NewCFunction(ctx, element_get_style, "get_style", 0),
        JS_NewCFunction(ctx, element_set_style, "set_style", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // setAttribute method
  JS_SetPropertyStr(ctx, obj, "setAttribute",
      JS_NewCFunction(ctx, element_setAttribute, "setAttribute", 2));

  // getElementsByTagName method
  JS_SetPropertyStr(ctx, obj, "getElementsByTagName",
      JS_NewCFunction(ctx, element_getElementsByTagName, "getElementsByTagName", 1));

  // getElementsByClassName method
  JS_SetPropertyStr(ctx, obj, "getElementsByClassName",
      JS_NewCFunction(ctx, element_getElementsByClassName, "getElementsByClassName", 1));

  // querySelector method
  JS_SetPropertyStr(ctx, obj, "querySelector",
      JS_NewCFunction(ctx, element_querySelector, "querySelector", 1));

  // querySelectorAll method
  JS_SetPropertyStr(ctx, obj, "querySelectorAll",
      JS_NewCFunction(ctx, element_querySelectorAll, "querySelectorAll", 1));

  // getAttribute method
  JS_SetPropertyStr(ctx, obj, "getAttribute",
      JS_NewCFunction(ctx, element_getAttribute, "getAttribute", 1));

  // hasAttribute method
  JS_SetPropertyStr(ctx, obj, "hasAttribute",
      JS_NewCFunction(ctx, element_hasAttribute, "hasAttribute", 1));

  // removeAttribute method
  JS_SetPropertyStr(ctx, obj, "removeAttribute",
      JS_NewCFunction(ctx, element_removeAttribute, "removeAttribute", 1));

  // classList property (returns a classList object with add, remove, toggle, contains)
  JSValue classListObj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, classListObj, "add",
      JS_NewCFunction(ctx, classList_add, "add", 1));
  JS_SetPropertyStr(ctx, classListObj, "remove",
      JS_NewCFunction(ctx, classList_remove, "remove", 1));
  JS_SetPropertyStr(ctx, classListObj, "toggle",
      JS_NewCFunction(ctx, classList_toggle, "toggle", 1));
  JS_SetPropertyStr(ctx, classListObj, "contains",
      JS_NewCFunction(ctx, classList_contains, "contains", 1));
  JS_SetPropertyStr(ctx, obj, "classList", classListObj);

  // parentNode (getter)
  atom = JS_NewAtom(ctx, "parentNode");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_parentNode, "get_parentNode", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // childNodes (getter)
  atom = JS_NewAtom(ctx, "childNodes");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_childNodes, "get_childNodes", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // children (getter)
  atom = JS_NewAtom(ctx, "children");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_children, "get_children", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // firstElementChild (getter)
  atom = JS_NewAtom(ctx, "firstElementChild");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_firstElementChild, "get_firstElementChild", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // lastElementChild (getter)
  atom = JS_NewAtom(ctx, "lastElementChild");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_lastElementChild, "get_lastElementChild", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // nextElementSibling (getter)
  atom = JS_NewAtom(ctx, "nextElementSibling");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_nextElementSibling, "get_nextElementSibling", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // previousElementSibling (getter)
  atom = JS_NewAtom(ctx, "previousElementSibling");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_previousElementSibling, "get_previousElementSibling", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // childElementCount (getter)
  atom = JS_NewAtom(ctx, "childElementCount");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_childElementCount, "get_childElementCount", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // ownerDocument (getter)
  atom = JS_NewAtom(ctx, "ownerDocument");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_ownerDocument, "get_ownerDocument", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // nodeType (getter)
  atom = JS_NewAtom(ctx, "nodeType");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_nodeType, "get_nodeType", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // nodeName (getter)
  atom = JS_NewAtom(ctx, "nodeName");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_nodeName, "get_nodeName", 0),
      JS_UNDEFINED,
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // className (getter/setter)
  atom = JS_NewAtom(ctx, "className");
  JS_DefinePropertyGetSet(ctx, obj, atom,
      JS_NewCFunction(ctx, element_get_className, "get_className", 0),
      JS_NewCFunction(ctx, element_set_className, "set_className", 1),
      JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
  JS_FreeAtom(ctx, atom);

  // insertBefore method
  JS_SetPropertyStr(ctx, obj, "insertBefore",
      JS_NewCFunction(ctx, element_insertBefore, "insertBefore", 2));

  // replaceChild method
  JS_SetPropertyStr(ctx, obj, "replaceChild",
      JS_NewCFunction(ctx, element_replaceChild, "replaceChild", 2));

  // cloneNode method
  JS_SetPropertyStr(ctx, obj, "cloneNode",
      JS_NewCFunction(ctx, element_cloneNode, "cloneNode", 1));

  // matches method
  JS_SetPropertyStr(ctx, obj, "matches",
      JS_NewCFunction(ctx, element_matches, "matches", 1));

  // closest method
  JS_SetPropertyStr(ctx, obj, "closest",
      JS_NewCFunction(ctx, element_closest, "closest", 1));

  // hasChildNodes method
  JS_SetPropertyStr(ctx, obj, "hasChildNodes",
      JS_NewCFunction(ctx, element_hasChildNodes, "hasChildNodes", 0));

  // normalize method
  JS_SetPropertyStr(ctx, obj, "normalize",
      JS_NewCFunction(ctx, element_normalize, "normalize", 0));

  // contains method
  JS_SetPropertyStr(ctx, obj, "contains",
      JS_NewCFunction(ctx, element_contains, "contains", 1));

  // appendChild method
    JS_SetPropertyStr(
        ctx, obj, "appendChild",
        JS_NewCFunction(ctx, element_appendChild, "appendChild", 1));

    // removeChild method
    JS_SetPropertyStr(
        ctx, obj, "removeChild",
        JS_NewCFunction(ctx, element_removeChild, "removeChild", 1));

    // addEventListener
    JS_SetPropertyStr(
        ctx, obj, "addEventListener",
        JS_NewCFunction(ctx, element_addEventListener, "addEventListener", 2));

    // removeEventListener
    JS_SetPropertyStr(ctx, obj, "removeEventListener",
                      JS_NewCFunction(ctx, element_removeEventListener,
                                      "removeEventListener", 2));

    // dispatchEvent
    JS_SetPropertyStr(
        ctx, obj, "dispatchEvent",
        JS_NewCFunction(ctx, element_dispatchEvent, "dispatchEvent", 1));

    return obj;
  }

  static void cleanup() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_createdElements.clear();
  }

private:
  static JSClassID s_elementClassId;
  static JSClassID s_documentClassId;
  // Store elements created via JS. Use weak_ptr to avoid memory leaks.
  static inline std::vector<std::weak_ptr<dom::Element>> s_createdElements;
  static inline std::mutex s_mutex;

  // document.getElementById(id)
  static JSValue document_getElementById(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string id = JSBinding::toStdString(ctx, argv[0]);
    auto element = doc->getElementById(id);

    if (element) {
      return wrapElement(ctx, element.get());
    }

    return JS_NULL;
  }

  // document.createElement(tagName)
  static JSValue document_createElement(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string tagName = JSBinding::toStdString(ctx, argv[0]);
    auto element = doc->createElement(tagName);
    if (element) {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_createdElements.push_back(element);

      // Periodically clean up expired weak pointers to prevent vector from
      // growing indefinitely
      if (s_createdElements.size() > 100) {
        auto it = s_createdElements.begin();
        while (it != s_createdElements.end()) {
          if (it->expired()) {
            it = s_createdElements.erase(it);
          } else {
            ++it;
          }
        }
      }
      return wrapElement(ctx, element.get());
    }
    return JS_NULL;
  }

  // element.innerHTML getter
  static JSValue element_get_innerHTML(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, libraryElem->innerHTML());
  }

  // element.innerHTML setter
  static JSValue element_set_innerHTML(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    if (argc < 1)
      return JS_EXCEPTION;

    std::string html = JSBinding::toStdString(ctx, argv[0]);

    // Clear existing children
    // TODO: Ideally we should use removeChild or similar to ensure proper
    // cleanup For now we need to access the children list directly or add a
    // method to Element/Node Let's assume we can cast to Node and access
    // children, OR add clearChildren to Node. For now, let's try calling
    // removeAllChildren() which we will add to Node/Element if it doesn't
    // exist. But since we are in header-only, let's rely on adding it to
    // `dom.hpp`.

    // Actually, let's check `dom.hpp` again implicitly.
    // I recall `Element` inherits `Node`.
    // I will add `removeAllChildren` to `Node` in `dom.hpp` in the next step if
    // needed. Here I will use it.

    libraryElem->removeAllChildren();

    auto nodes = dom::HtmlParser::parseFragment(html);

    for (const auto &node : nodes) {
      libraryElem->appendChild(node);
    }

    return JS_UNDEFINED;
  }

  // element.id getter
  static JSValue element_get_id(JSContext *ctx, JSValueConst this_val, int argc,
                                JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, libraryElem->id());
  }

  // element.tagName getter
  static JSValue element_get_tagName(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, libraryElem->tagName());
  }

  // element.style getter (returns string of style attribute)
  static JSValue element_get_style(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    auto style = libraryElem->getAttribute("style");
    return JSBinding::toJSString(ctx, style.value_or(""));
  }

  // element.style setter (takes string)
  static JSValue element_set_style(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    if (argc < 1)
      return JS_EXCEPTION;

    std::string style = JSBinding::toStdString(ctx, argv[0]);
    libraryElem->setAttribute("style", style);

    return JS_UNDEFINED;
  }

  // element.setAttribute(name, value)
  static JSValue element_setAttribute(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    if (argc < 2)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string value = JSBinding::toStdString(ctx, argv[1]);

    libraryElem->setAttribute(name, value);
    return JS_UNDEFINED;
  }

  // element.appendChild(child)
  static JSValue element_appendChild(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    dom::Element *parent =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!parent)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    dom::Element *child =
        (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (!child)
      return JS_EXCEPTION;

    // We need a shared_ptr. Since we don't own it, we create a non-deleting
    // shared_ptr. This is a pragmatic approach: the DOM tree manages lifetime
    // through its own shared_ptrs. We use shared_from_this() if available.
    try {
      auto childPtr = child->shared_from_this();
      parent->appendChild(childPtr);
    } catch (...) {
      std::cout << "[DOM] Warning: appendChild failed - child node not managed "
                   "by shared_ptr"
                << std::endl;
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[0]);
  }

  // element.removeChild(child)
  static JSValue element_removeChild(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    dom::Element *parent =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!parent)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    dom::Element *child =
        (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (!child)
      return JS_EXCEPTION;

    try {
      auto childPtr = child->shared_from_this();
      parent->removeChild(childPtr);
    } catch (...) {
      std::cout << "[DOM] Warning: removeChild failed - child node not managed "
                   "by shared_ptr"
                << std::endl;
      return JS_EXCEPTION;
    }

    return JS_DupValue(ctx, argv[0]);
  }
  // --- Event System ---

  // Get the Node* from either element or document class
  static dom::Node *getNodeFromThis(JSContext *ctx, JSValueConst this_val) {
    (void)ctx;
    // Try element first
    dom::Node *node = (dom::Node *)JS_GetOpaque(this_val, s_elementClassId);
    if (!node) {
      // Try document
      node = (dom::Node *)JS_GetOpaque(this_val, s_documentClassId);
    }
    return node;
  }

  // element.addEventListener(type, callback)
  static JSValue element_addEventListener(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 2)
      return JS_EXCEPTION;
    if (!JS_IsFunction(ctx, argv[1]))
      return JS_EXCEPTION;

    std::string type = JSBinding::toStdString(ctx, argv[0]);
    uint32_t id = EventBinding::addListener(ctx, argv[1]);
    node->eventListenerIds_[type].push_back(id);

    return JS_UNDEFINED;
  }

  // element.removeEventListener(type, callback)
  // Note: In a real browser, this matches by function reference.
  // Here we simplify: remove the LAST listener for that type.
  static JSValue element_removeEventListener(JSContext *ctx,
                                             JSValueConst this_val, int argc,
                                             JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 2)
      return JS_EXCEPTION;

    std::string type = JSBinding::toStdString(ctx, argv[0]);
    auto it = node->eventListenerIds_.find(type);
    if (it != node->eventListenerIds_.end() && !it->second.empty()) {
      uint32_t id = it->second.back();
      EventBinding::removeListener(id);
      it->second.pop_back();
    }

    return JS_UNDEFINED;
  }

  // element.dispatchEvent(eventTypeString)
  // Simplified: takes a string (event type) instead of an Event object.
  static JSValue element_dispatchEvent(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 1)
      return JS_EXCEPTION;

    std::string type = JSBinding::toStdString(ctx, argv[0]);
    auto it = node->eventListenerIds_.find(type);
    if (it != node->eventListenerIds_.end()) {
      JSValue eventObj = EventBinding::createEventObject(ctx, type);
      EventBinding::dispatch(ctx, it->second, eventObj);
      JS_FreeValue(ctx, eventObj);
    }

    return JS_UNDEFINED;
  }

  // document.getElementsByTagName(name)
  static JSValue document_getElementsByTagName(JSContext *ctx, JSValueConst this_val,
                                                int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto elements = doc->getElementsByTagName(name);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // document.getElementsByClassName(className)
  static JSValue document_getElementsByClassName(JSContext *ctx, JSValueConst this_val,
                                                  int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    auto elements = doc->getElementsByClassName(className);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // document.querySelector(selector) - simplified, only supports #id and tagName
  static JSValue document_querySelector(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    // Simplified selector support: only #id for now
    if (selector[0] == '#' && selector.length() > 1) {
      auto element = doc->getElementById(selector.substr(1));
      if (element) {
        return wrapElement(ctx, element.get());
      }
    } else {
      // Try tagName
      auto elements = doc->getElementsByTagName(selector);
      if (!elements.empty()) {
        return wrapElement(ctx, elements[0].get());
      }
    }

    return JS_NULL;
  }

  // document.querySelectorAll(selector) - simplified
  static JSValue document_querySelectorAll(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv) {
    dom::Document *doc =
        (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
    if (!doc)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    std::vector<dom::ElementPtr> elements;

    if (selector[0] == '.' && selector.length() > 1) {
      elements = doc->getElementsByClassName(selector.substr(1));
    } else if (selector[0] == '#' && selector.length() > 1) {
      auto element = doc->getElementById(selector.substr(1));
      if (element) {
        elements.push_back(element);
      }
    } else {
      elements = doc->getElementsByTagName(selector);
    }

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // element.textContent getter
  static JSValue element_get_textContent(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, libraryElem->textContent());
  }

  // element.textContent setter
  static JSValue element_set_textContent(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    dom::Element *libraryElem =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!libraryElem)
      return JS_EXCEPTION;

    if (argc < 1)
      return JS_EXCEPTION;

    std::string text = JSBinding::toStdString(ctx, argv[0]);
    libraryElem->setTextContent(text);

    return JS_UNDEFINED;
  }

  // element.getElementsByTagName(name)
  static JSValue element_getElementsByTagName(JSContext *ctx, JSValueConst this_val,
                                               int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto elements = element->getElementsByTagName(name);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // element.getElementsByClassName(className)
  static JSValue element_getElementsByClassName(JSContext *ctx, JSValueConst this_val,
                                                 int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    auto elements = element->getElementsByClassName(className);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // element.querySelector(selector) - simplified
  static JSValue element_querySelector(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    if (selector[0] == '#' && selector.length() > 1) {
      auto found = element->getElementById(selector.substr(1));
      if (found) {
        return wrapElement(ctx, found.get());
      }
    } else {
      auto elements = element->getElementsByTagName(selector);
      if (!elements.empty()) {
        return wrapElement(ctx, elements[0].get());
      }
    }

    return JS_NULL;
  }

  // element.querySelectorAll(selector) - simplified
  static JSValue element_querySelectorAll(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    std::vector<dom::ElementPtr> elements;

    if (selector[0] == '.' && selector.length() > 1) {
      elements = element->getElementsByClassName(selector.substr(1));
    } else if (selector[0] == '#' && selector.length() > 1) {
      auto found = element->getElementById(selector.substr(1));
      if (found) {
        elements.push_back(found);
      }
    } else {
      elements = element->getElementsByTagName(selector);
    }

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // element.getAttribute(name)
  static JSValue element_getAttribute(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto value = element->getAttribute(name);
    if (value) {
      return JSBinding::toJSString(ctx, *value);
    }
    return JS_NULL;
  }

  // element.hasAttribute(name)
  static JSValue element_hasAttribute(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto value = element->getAttribute(name);
    return JS_NewBool(ctx, value.has_value());
  }

  // element.removeAttribute(name)
  static JSValue element_removeAttribute(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    element->removeAttribute(name);
    return JS_UNDEFINED;
  }

  // classList.add(className)
  static JSValue classList_add(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    for (int i = 0; i < argc; i++) {
      std::string className = JSBinding::toStdString(ctx, argv[i]);
      element->addClass(className);
    }
    return JS_UNDEFINED;
  }

  // classList.remove(className)
  static JSValue classList_remove(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    for (int i = 0; i < argc; i++) {
      std::string className = JSBinding::toStdString(ctx, argv[i]);
      element->removeClass(className);
    }
    return JS_UNDEFINED;
  }

  // classList.toggle(className)
  static JSValue classList_toggle(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    bool force = (argc > 1) ? JS_ToBool(ctx, argv[1]) : !element->hasClass(className);

    if (force) {
      element->addClass(className);
      return JS_TRUE;
    } else {
      element->removeClass(className);
      return JS_FALSE;
    }
  }

  // classList.contains(className)
  static JSValue classList_contains(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    return JS_NewBool(ctx, element->hasClass(className));
  }

  // element.parentNode getter
  static JSValue element_get_parentNode(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto parent = element->parentNode();
    if (parent && parent->nodeType() == dom::NodeType::Element) {
      return wrapElement(ctx, std::static_pointer_cast<dom::Element>(parent).get());
    }
    return JS_NULL;
  }

  // element.childNodes getter (returns all child nodes including text nodes)
  static JSValue element_get_childNodes(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    JSValue arr = JS_NewArray(ctx);
    size_t index = 0;
    for (const auto &child : element->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        JS_DefinePropertyValueUint32(
            ctx, arr, index++,
            wrapElement(ctx, std::static_pointer_cast<dom::Element>(child).get()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      } else if (child->nodeType() == dom::NodeType::Text) {
        JS_DefinePropertyValueUint32(
            ctx, arr, index++,
            JSBinding::toJSString(ctx, child->textContent()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      }
    }
    return arr;
  }

  // element.children getter (returns only element children)
  static JSValue element_get_children(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    JSValue arr = JS_NewArray(ctx);
    size_t index = 0;
    for (const auto &child : element->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        JS_DefinePropertyValueUint32(
            ctx, arr, index++,
            wrapElement(ctx, std::static_pointer_cast<dom::Element>(child).get()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      }
    }
    return arr;
  }

  // element.firstElementChild getter
  static JSValue element_get_firstElementChild(JSContext *ctx, JSValueConst this_val,
                                             int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto child = element->firstElementChild();
    if (child) {
      return wrapElement(ctx, child.get());
    }
    return JS_NULL;
  }

  // element.lastElementChild getter
  static JSValue element_get_lastElementChild(JSContext *ctx, JSValueConst this_val,
                                            int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto child = element->lastElementChild();
    if (child) {
      return wrapElement(ctx, child.get());
    }
    return JS_NULL;
  }

  // element.nextElementSibling getter
  static JSValue element_get_nextElementSibling(JSContext *ctx, JSValueConst this_val,
                                               int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto sibling = element->nextElementSibling();
    if (sibling) {
      return wrapElement(ctx, sibling.get());
    }
    return JS_NULL;
  }

  // element.previousElementSibling getter
  static JSValue element_get_previousElementSibling(JSContext *ctx, JSValueConst this_val,
                                                   int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto sibling = element->previousElementSibling();
    if (sibling) {
      return wrapElement(ctx, sibling.get());
    }
    return JS_NULL;
  }

  // element.childElementCount getter
  static JSValue element_get_childElementCount(JSContext *ctx, JSValueConst this_val,
                                             int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    return JS_NewUint32(ctx, element->childElementCount());
  }

  // element.ownerDocument getter (returns the Document)
  static JSValue element_get_ownerDocument(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    auto root = element->getRootNode();
    if (root && root->nodeType() == dom::NodeType::Document) {
      return wrapDocument(ctx, std::dynamic_pointer_cast<dom::Document>(root));
    }
    return JS_NULL;
  }

  // element.nodeType getter
  static JSValue element_get_nodeType(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    return JS_NewUint32(ctx, static_cast<uint32_t>(dom::NodeType::Element));
  }

  // element.nodeName getter
  static JSValue element_get_nodeName(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, element->tagName());
  }

  // element.className getter
  static JSValue element_get_className(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    return JSBinding::toJSString(ctx, element->className());
  }

  // element.className setter
  static JSValue element_set_className(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    element->setClassName(className);
    return JS_UNDEFINED;
  }

  // element.insertBefore(newNode, refNode)
  static JSValue element_insertBefore(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *parent =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!parent)
      return JS_EXCEPTION;
    if (argc < 2)
      return JS_EXCEPTION;

    dom::Element *newNode =
        (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    dom::Element *refNode =
        (dom::Element *)JS_GetOpaque(argv[1], s_elementClassId);
    if (!newNode)
      return JS_EXCEPTION;

    try {
      auto newNodePtr = newNode->shared_from_this();
      dom::NodePtr refNodePtr;
      if (refNode) {
        refNodePtr = refNode->shared_from_this();
      }
      parent->insertBefore(newNodePtr, refNodePtr);
    } catch (...) {
      std::cout << "[DOM] Warning: insertBefore failed" << std::endl;
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[0]);
  }

  // element.replaceChild(newNode, oldNode)
  static JSValue element_replaceChild(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    dom::Element *parent =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!parent)
      return JS_EXCEPTION;
    if (argc < 2)
      return JS_EXCEPTION;

    dom::Element *newNode =
        (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    dom::Element *oldNode =
        (dom::Element *)JS_GetOpaque(argv[1], s_elementClassId);
    if (!newNode || !oldNode)
      return JS_EXCEPTION;

    try {
      auto newNodePtr = newNode->shared_from_this();
      auto oldNodePtr = oldNode->shared_from_this();
      parent->replaceChild(newNodePtr, oldNodePtr);
    } catch (...) {
      std::cout << "[DOM] Warning: replaceChild failed" << std::endl;
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[1]);
  }

  // element.cloneNode(deep)
  static JSValue element_cloneNode(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    bool deep = (argc > 0) ? JS_ToBool(ctx, argv[0]) : false;
    auto cloned = element->cloneNode(deep);

    if (cloned && cloned->nodeType() == dom::NodeType::Element) {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_createdElements.push_back(
          std::static_pointer_cast<dom::Element>(cloned));
      return wrapElement(ctx, std::static_pointer_cast<dom::Element>(cloned).get());
    }
    return JS_NULL;
  }

  // element.matches(selector)
  static JSValue element_matches(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);
    return JS_NewBool(ctx, element->matches(selector));
  }

  // element.closest(selector)
  static JSValue element_closest(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    dom::Element *current = element;
    while (current) {
      if (current->matches(selector)) {
        return wrapElement(ctx, current);
      }
      auto parent = current->parentNode();
      if (parent && parent->nodeType() == dom::NodeType::Element) {
        current = std::static_pointer_cast<dom::Element>(parent).get();
      } else {
        break;
      }
    }
    return JS_NULL;
  }

  // element.hasChildNodes()
  static JSValue element_hasChildNodes(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    return JS_NewBool(ctx, element->hasChildNodes());
  }

  // element.normalize()
  static JSValue element_normalize(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)argc;
    (void)argv;
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;

    element->normalize();
    return JS_UNDEFINED;
  }

  // element.contains(other)
  static JSValue element_contains(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    dom::Element *element =
        (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!element)
      return JS_EXCEPTION;
    if (argc < 1)
      return JS_EXCEPTION;

    dom::Element *other =
        (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (!other)
      return JS_FALSE;

    try {
      auto otherPtr = other->shared_from_this();
      return JS_NewBool(ctx, element->contains(otherPtr));
    } catch (...) {
      return JS_FALSE;
    }
  }
};

// Static definitions need to be inline for header-only
inline JSClassID DOMBinding::s_elementClassId = 0;
inline JSClassID DOMBinding::s_documentClassId = 0;

} // namespace script
} // namespace xiaopeng
