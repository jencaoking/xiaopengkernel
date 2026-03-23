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

    // style (getter/setter) - simplified to string
    atom = JS_NewAtom(ctx, "style");
    JS_DefinePropertyGetSet(
        ctx, obj, atom, JS_NewCFunction(ctx, element_get_style, "get_style", 0),
        JS_NewCFunction(ctx, element_set_style, "set_style", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // setAttribute method
    JS_SetPropertyStr(
        ctx, obj, "setAttribute",
        JS_NewCFunction(ctx, element_setAttribute, "setAttribute", 2));

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
};

// Static definitions need to be inline for header-only
inline JSClassID DOMBinding::s_elementClassId = 0;
inline JSClassID DOMBinding::s_documentClassId = 0;

} // namespace script
} // namespace xiaopeng
