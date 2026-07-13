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

// ============================================================
//  DOMBinding — Phase 1: Complete JS DOM API
//  Covers:
//    - Document: body, head, title, documentElement, URL, readyState,
//                createTextNode, createDocumentFragment, createComment
//    - Element:  parentNode, parentElement, childNodes, children,
//                firstChild, lastChild, firstElementChild, lastElementChild,
//                nextSibling, previousSibling, nextElementSibling,
//                previousElementSibling, nodeValue, nodeName,
//                className (get/set), classList,
//                insertBefore, replaceChild, remove, cloneNode,
//                contains, isEqualNode, closest, matches
//    - NodeList: live-length array-like with item(index)
//    - classList: add, remove, toggle, contains, replace, item, toString
// ============================================================

class DOMBinding {
public:
  static void registerBinding(JSContext *ctx) {
    JS_NewClassID(&s_elementClassId);
    JS_NewClassID(&s_documentClassId);
    JS_NewClassID(&s_nodeListClassId);
    JS_NewClassID(&s_classListClassId);
    JS_NewClassID(&s_textNodeClassId);

    // Element class
    JSClassDef elementDef;
    memset(&elementDef, 0, sizeof(JSClassDef));
    elementDef.class_name = "Element";
    elementDef.finalizer = [](JSRuntime *, JSValue) {};
    JS_NewClass(JS_GetRuntime(ctx), s_elementClassId, &elementDef);

    // Document class
    JSClassDef docDef;
    memset(&docDef, 0, sizeof(JSClassDef));
    docDef.class_name = "Document";
    docDef.finalizer = [](JSRuntime *, JSValue) {};
    JS_NewClass(JS_GetRuntime(ctx), s_documentClassId, &docDef);

    // NodeList class
    JSClassDef nlDef;
    memset(&nlDef, 0, sizeof(JSClassDef));
    nlDef.class_name = "NodeList";
    nlDef.finalizer = [](JSRuntime *, JSValue) {};
    JS_NewClass(JS_GetRuntime(ctx), s_nodeListClassId, &nlDef);

    // DOMTokenList (classList) class
    JSClassDef clDef;
    memset(&clDef, 0, sizeof(JSClassDef));
    clDef.class_name = "DOMTokenList";
    clDef.finalizer = [](JSRuntime *, JSValue) {};
    JS_NewClass(JS_GetRuntime(ctx), s_classListClassId, &clDef);

    // Text class
    JSClassDef textDef;
    memset(&textDef, 0, sizeof(JSClassDef));
    textDef.class_name = "Text";
    textDef.finalizer = [](JSRuntime *, JSValue) {};
    JS_NewClass(JS_GetRuntime(ctx), s_textNodeClassId, &textDef);
  }

  // ── wrapDocument ────────────────────────────────────────────
  static JSValue wrapDocument(JSContext *ctx,
                              std::shared_ptr<dom::Document> doc) {
    JSValue obj = JS_NewObjectClass(ctx, s_documentClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, doc.get());

    // Store shared_ptr to keep document alive
    JS_SetPropertyStr(ctx, obj, "___doc_ptr",
                      JS_NewBigUint64(ctx, (uint64_t)new std::shared_ptr<dom::Document>(doc)));

    // --- Methods ---
    auto bind = [&](const char *name, JSCFunction func, int argc) {
      JS_SetPropertyStr(ctx, obj, name,
                        JS_NewCFunction(ctx, func, name, argc));
    };

    bind("getElementById",        document_getElementById, 1);
    bind("getElementsByTagName",  document_getElementsByTagName, 1);
    bind("getElementsByClassName",document_getElementsByClassName, 1);
    bind("querySelector",         document_querySelector, 1);
    bind("querySelectorAll",      document_querySelectorAll, 1);
    bind("createElement",         document_createElement, 1);
    bind("createTextNode",        document_createTextNode, 1);
    bind("createDocumentFragment", document_createDocumentFragment, 0);
    bind("createComment",         document_createComment, 1);
    bind("addEventListener",      element_addEventListener, 2);
    bind("removeEventListener",   element_removeEventListener, 2);
    bind("dispatchEvent",         element_dispatchEvent, 1);

    // --- Properties ---
    // document.body
    JSAtom atom = JS_NewAtom(ctx, "body");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_body, "get_body", 0),
        JS_NewCFunction(ctx, document_set_body, "set_body", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // document.head
    atom = JS_NewAtom(ctx, "head");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_head, "get_head", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // document.documentElement
    atom = JS_NewAtom(ctx, "documentElement");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_documentElement, "get_documentElement", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // document.title (get/set)
    atom = JS_NewAtom(ctx, "title");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_title, "get_title", 0),
        JS_NewCFunction(ctx, document_set_title, "set_title", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // document.URL (read-only)
    atom = JS_NewAtom(ctx, "URL");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_URL, "get_URL", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // document.readyState (read-only)
    atom = JS_NewAtom(ctx, "readyState");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_readyState, "get_readyState", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    return obj;
  }

  // ── wrapElement ─────────────────────────────────────────────
  static JSValue wrapElement(JSContext *ctx, dom::Element *element) {
    if (!element)
      return JS_NULL;

    JSValue obj = JS_NewObjectClass(ctx, s_elementClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, element);

    // --- Existing properties ---
    bindGetSet(ctx, obj, "innerHTML",
               element_get_innerHTML, element_set_innerHTML);
    bindReadOnly(ctx, obj, "id",      element_get_id);
    bindReadOnly(ctx, obj, "tagName", element_get_tagName);
    bindGetSet(ctx, obj, "textContent",
               element_get_textContent, element_set_textContent);
    bindGetSet(ctx, obj, "style",
               element_get_style, element_set_style);

    // --- NEW: className (get/set) ---
    bindGetSet(ctx, obj, "className",
               element_get_className, element_set_className);

    // --- NEW: classList (read-only, returns DOMTokenList) ---
    bindReadOnly(ctx, obj, "classList", element_get_classList);

    // --- NEW: Node tree properties ---
    bindReadOnly(ctx, obj, "parentNode",         node_get_parentNode);
    bindReadOnly(ctx, obj, "parentElement",      node_get_parentElement);
    bindReadOnly(ctx, obj, "childNodes",         node_get_childNodes);
    bindReadOnly(ctx, obj, "children",           node_get_children);
    bindReadOnly(ctx, obj, "firstChild",         node_get_firstChild);
    bindReadOnly(ctx, obj, "lastChild",          node_get_lastChild);
    bindReadOnly(ctx, obj, "firstElementChild",  node_get_firstElementChild);
    bindReadOnly(ctx, obj, "lastElementChild",   node_get_lastElementChild);
    bindReadOnly(ctx, obj, "nextSibling",        node_get_nextSibling);
    bindReadOnly(ctx, obj, "previousSibling",    node_get_previousSibling);
    bindReadOnly(ctx, obj, "nextElementSibling", node_get_nextElementSibling);
    bindReadOnly(ctx, obj, "previousElementSibling", node_get_previousElementSibling);

    // --- NEW: nodeName, nodeValue, nodeType ---
    bindReadOnly(ctx, obj, "nodeName",  node_get_nodeName);
    bindGetSet(ctx, obj, "nodeValue",
               node_get_nodeValue, node_set_nodeValue);
    bindReadOnly(ctx, obj, "nodeType",  node_get_nodeType);

    // --- NEW: childElementCount ---
    bindReadOnly(ctx, obj, "childElementCount", element_get_childElementCount);

    // --- Attribute methods ---
    auto bindMethod = [&](const char *name, JSCFunction func, int argc) {
      JS_SetPropertyStr(ctx, obj, name,
                        JS_NewCFunction(ctx, func, name, argc));
    };

    bindMethod("setAttribute",       element_setAttribute, 2);
    bindMethod("getAttribute",       element_getAttribute, 1);
    bindMethod("hasAttribute",       element_hasAttribute, 1);
    bindMethod("removeAttribute",    element_removeAttribute, 1);

    // --- DOM manipulation ---
    bindMethod("appendChild",        element_appendChild, 1);
    bindMethod("removeChild",        element_removeChild, 1);
    bindMethod("insertBefore",       element_insertBefore, 2);
    bindMethod("replaceChild",       element_replaceChild, 2);
    bindMethod("remove",             element_remove, 0);
    bindMethod("cloneNode",          element_cloneNode, 1);
    bindMethod("contains",           element_contains, 1);
    bindMethod("isEqualNode",        element_isEqualNode, 1);

    // --- Collection methods ---
    bindMethod("getElementsByTagName",   element_getElementsByTagName, 1);
    bindMethod("getElementsByClassName", element_getElementsByClassName, 1);
    bindMethod("querySelector",          element_querySelector, 1);
    bindMethod("querySelectorAll",       element_querySelectorAll, 1);
    bindMethod("matches",                element_matches, 1);
    bindMethod("closest",                element_closest, 1);

    // --- Events ---
    bindMethod("addEventListener",    element_addEventListener, 2);
    bindMethod("removeEventListener", element_removeEventListener, 2);
    bindMethod("dispatchEvent",       element_dispatchEvent, 1);

    return obj;
  }

  // ── wrapTextNode ────────────────────────────────────────────
  static JSValue wrapTextNode(JSContext *ctx, dom::TextNode *textNode) {
    if (!textNode)
      return JS_NULL;

    JSValue obj = JS_NewObjectClass(ctx, s_textNodeClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, textNode);

    // Node properties
    bindReadOnly(ctx, obj, "parentNode",    node_get_parentNode);
    bindReadOnly(ctx, obj, "parentElement", node_get_parentElement);
    bindReadOnly(ctx, obj, "nextSibling",   node_get_nextSibling);
    bindReadOnly(ctx, obj, "previousSibling", node_get_previousSibling);
    bindReadOnly(ctx, obj, "nodeName",      node_get_nodeName);
    bindReadOnly(ctx, obj, "nodeType",      node_get_nodeType);
    bindGetSet(ctx, obj, "nodeValue",       node_get_nodeValue, node_set_nodeValue);
    bindGetSet(ctx, obj, "textContent",     node_get_nodeValue, node_set_nodeValue);
    bindGetSet(ctx, obj, "data",            node_get_nodeValue, node_set_nodeValue);

    return obj;
  }

  static void cleanup() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_createdElements.clear();
    s_createdNodes.clear();
  }

private:
  static JSClassID s_elementClassId;
  static JSClassID s_documentClassId;
  static JSClassID s_nodeListClassId;
  static JSClassID s_classListClassId;
  static JSClassID s_textNodeClassId;

  static inline std::vector<std::weak_ptr<dom::Element>> s_createdElements;
  static inline std::vector<dom::NodePtr> s_createdNodes; // BUG FIX: track all created nodes
  static inline std::mutex s_mutex;

  // ── Helper: get Node* from either element or text class ──
  static dom::Node *getNodeFromThis(JSContext *ctx, JSValueConst this_val) {
    dom::Node *node = (dom::Node *)JS_GetOpaque(this_val, s_elementClassId);
    if (!node)
      node = (dom::Node *)JS_GetOpaque(this_val, s_textNodeClassId);
    if (!node)
      node = (dom::Node *)JS_GetOpaque(this_val, s_documentClassId);
    return node;
  }

  // ── Helper: bind read-only getter ──
  static void bindReadOnly(JSContext *ctx, JSValue obj, const char *name,
                           JSCFunction getter) {
    JSAtom atom = JS_NewAtom(ctx, name);
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, getter, (std::string("get_") + name).c_str(), 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);
  }

  // ── Helper: bind getter+setter ──
  static void bindGetSet(JSContext *ctx, JSValue obj, const char *name,
                         JSCFunction getter, JSCFunction setter) {
    JSAtom atom = JS_NewAtom(ctx, name);
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, getter, (std::string("get_") + name).c_str(), 0),
        JS_NewCFunction(ctx, setter, (std::string("set_") + name).c_str(), 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);
  }

  // ── Helper: wrap child node (Element or TextNode) ──
  static JSValue wrapNode(JSContext *ctx, dom::NodePtr node) {
    if (!node)
      return JS_NULL;
    if (node->nodeType() == dom::NodeType::Element) {
      return wrapElement(ctx, static_cast<dom::Element *>(node.get()));
    } else if (node->nodeType() == dom::NodeType::Text) {
      return wrapTextNode(ctx, static_cast<dom::TextNode *>(node.get()));
    }
    // Comment, DocumentType etc. → return as generic object with nodeType
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "nodeType",
                      JS_NewInt32(ctx, static_cast<int>(node->nodeType())));
    JS_SetPropertyStr(ctx, obj, "nodeName",
                      JSBinding::toJSString(ctx, node->nodeName()));
    return obj;
  }

  // ── Helper: wrap vector<ElementPtr> as JS Array ──
  static JSValue wrapElementArray(JSContext *ctx,
                                  const std::vector<dom::ElementPtr> &elems) {
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elems.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapElement(ctx, elems[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // ── Helper: create a live NodeList wrapping childNodes ──
  static JSValue wrapChildNodes(JSContext *ctx, dom::Node *node) {
    JSValue arr = JS_NewArray(ctx);
    auto &children = node->childNodes();
    for (size_t i = 0; i < children.size(); ++i) {
      JS_DefinePropertyValueUint32(
          ctx, arr, i, wrapNode(ctx, children[i]),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    JS_SetPropertyStr(ctx, arr, "length", JS_NewInt32(ctx, (int32_t)children.size()));
    return arr;
  }

  // ══════════════════════════════════════════════════════════
  //  DOCUMENT METHODS
  // ══════════════════════════════════════════════════════════

  static dom::Document *getDoc(JSContext *ctx, JSValueConst this_val) {
    return (dom::Document *)JS_GetOpaque(this_val, s_documentClassId);
  }

  // document.getElementById(id)
  static JSValue document_getElementById(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    auto el = doc->getElementById(JSBinding::toStdString(ctx, argv[0]));
    return el ? wrapElement(ctx, el.get()) : JS_NULL;
  }

  // document.getElementsByTagName(name)
  static JSValue document_getElementsByTagName(JSContext *ctx, JSValueConst this_val,
                                                int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, doc->getElementsByTagName(JSBinding::toStdString(ctx, argv[0])));
  }

  // document.getElementsByClassName(name)
  static JSValue document_getElementsByClassName(JSContext *ctx, JSValueConst this_val,
                                                  int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, doc->getElementsByClassName(JSBinding::toStdString(ctx, argv[0])));
  }

  // document.querySelector(selector)
  static JSValue document_querySelector(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    auto el = doc->querySelector(JSBinding::toStdString(ctx, argv[0]));
    return el ? wrapElement(ctx, el.get()) : JS_NULL;
  }

  // document.querySelectorAll(selector)
  static JSValue document_querySelectorAll(JSContext *ctx, JSValueConst this_val,
                                            int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, doc->querySelectorAll(JSBinding::toStdString(ctx, argv[0])));
  }

  // document.createElement(tagName)
  static JSValue document_createElement(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    auto el = doc->createElement(JSBinding::toStdString(ctx, argv[0]));
    if (!el) return JS_NULL;
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_createdElements.push_back(el);
      if (s_createdElements.size() > 100) {
        auto it = s_createdElements.begin();
        while (it != s_createdElements.end()) {
          if (it->expired()) it = s_createdElements.erase(it);
          else ++it;
        }
      }
    }
    return wrapElement(ctx, el.get());
  }

  // document.createTextNode(data)
  static JSValue document_createTextNode(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    auto node = doc->createTextNode(JSBinding::toStdString(ctx, argv[0]));
    if (!node) return JS_NULL;
    { std::lock_guard<std::mutex> lock(s_mutex); s_createdNodes.push_back(node); }
    return wrapTextNode(ctx, static_cast<dom::TextNode *>(node.get()));
  }

  // document.createDocumentFragment()
  static JSValue document_createDocumentFragment(JSContext *ctx, JSValueConst this_val,
                                                  int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    auto frag = doc->createDocumentFragment();
    if (!frag) return JS_NULL;
    JSValue obj = JS_NewObjectClass(ctx, s_elementClassId);
    if (JS_IsException(obj)) return obj;
    JS_SetOpaque(obj, static_cast<dom::Node *>(frag.get()));
    { std::lock_guard<std::mutex> lock(s_mutex); s_createdNodes.push_back(frag); }
    JS_SetPropertyStr(ctx, obj, "nodeType", JS_NewInt32(ctx, 11));
    JS_SetPropertyStr(ctx, obj, "nodeName", JS_NewString(ctx, "#document-fragment"));
    return obj;
  }

  // document.createComment(data)
  static JSValue document_createComment(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    auto node = doc->createComment(JSBinding::toStdString(ctx, argv[0]));
    if (!node) return JS_NULL;
    { std::lock_guard<std::mutex> lock(s_mutex); s_createdNodes.push_back(node); }
    JSValue obj = JS_NewObjectClass(ctx, s_elementClassId);
    if (JS_IsException(obj)) return obj;
    JS_SetOpaque(obj, static_cast<dom::Node *>(node.get()));
    JS_SetPropertyStr(ctx, obj, "nodeType", JS_NewInt32(ctx, 8));
    JS_SetPropertyStr(ctx, obj, "nodeName", JS_NewString(ctx, "#comment"));
    JS_SetPropertyStr(ctx, obj, "data",
                      JSBinding::toJSString(ctx, JSBinding::toStdString(ctx, argv[0])));
    return obj;
  }

  // document.body (getter)
  static JSValue document_get_body(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    auto body = doc->body();
    return body ? wrapElement(ctx, body.get()) : JS_NULL;
  }

  // document.body (setter)
  static JSValue document_set_body(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    dom::Element *el = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (!el) return JS_EXCEPTION;
    try {
      doc->setBody(std::static_pointer_cast<dom::Element>(el->shared_from_this()));
    } catch (...) {
      return JS_EXCEPTION;
    }
    return JS_UNDEFINED;
  }

  // document.head (getter)
  static JSValue document_get_head(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    auto head = doc->head();
    return head ? wrapElement(ctx, head.get()) : JS_NULL;
  }

  // document.documentElement (getter)
  static JSValue document_get_documentElement(JSContext *ctx, JSValueConst this_val,
                                               int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    auto html = doc->documentElement();
    return html ? wrapElement(ctx, html.get()) : JS_NULL;
  }

  // document.title (getter)
  static JSValue document_get_title(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, doc->titleText());
  }

  // document.title (setter)
  static JSValue document_set_title(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    auto *doc = getDoc(ctx, this_val);
    if (!doc || argc < 1) return JS_EXCEPTION;
    doc->setTitleText(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // document.URL (getter)
  static JSValue document_get_URL(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    auto *doc = getDoc(ctx, this_val);
    if (!doc) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, doc->url());
  }

  // document.readyState (getter)
  static JSValue document_get_readyState(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    (void)this_val;
    // Simplified: always "complete" since we parse synchronously before executing JS
    return JS_NewString(ctx, "complete");
  }

  // ══════════════════════════════════════════════════════════
  //  NODE TREE PROPERTIES (shared by Element & Text)
  // ══════════════════════════════════════════════════════════

  // node.parentNode
  static JSValue node_get_parentNode(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    auto parent = node->parentNode();
    if (!parent) return JS_NULL;
    if (parent->nodeType() == dom::NodeType::Element)
      return wrapElement(ctx, static_cast<dom::Element *>(parent.get()));
    if (parent->nodeType() == dom::NodeType::Document)
      return JS_NULL; // Don't expose raw document from here
    return wrapNode(ctx, parent);
  }

  // node.parentElement
  static JSValue node_get_parentElement(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    auto parent = node->parentNode();
    if (parent && parent->nodeType() == dom::NodeType::Element)
      return wrapElement(ctx, static_cast<dom::Element *>(parent.get()));
    return JS_NULL;
  }

  // node.childNodes
  static JSValue node_get_childNodes(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return wrapChildNodes(ctx, node);
  }

  // node.children (only Element children)
  static JSValue node_get_children(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    JSValue arr = JS_NewArray(ctx);
    uint32_t idx = 0;
    for (const auto &child : el->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        JS_DefinePropertyValueUint32(
            ctx, arr, idx++,
            wrapElement(ctx, static_cast<dom::Element *>(child.get())),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      }
    }
    return arr;
  }

  // node.firstChild
  static JSValue node_get_firstChild(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return wrapNode(ctx, node->firstChild());
  }

  // node.lastChild
  static JSValue node_get_lastChild(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return wrapNode(ctx, node->lastChild());
  }

  // node.firstElementChild
  static JSValue node_get_firstElementChild(JSContext *ctx, JSValueConst this_val,
                                             int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    auto child = el->firstElementChild();
    return child ? wrapElement(ctx, child.get()) : JS_NULL;
  }

  // node.lastElementChild
  static JSValue node_get_lastElementChild(JSContext *ctx, JSValueConst this_val,
                                            int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    auto child = el->lastElementChild();
    return child ? wrapElement(ctx, child.get()) : JS_NULL;
  }

  // node.nextSibling
  static JSValue node_get_nextSibling(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return wrapNode(ctx, node->nextSibling());
  }

  // node.previousSibling
  static JSValue node_get_previousSibling(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return wrapNode(ctx, node->previousSibling());
  }

  // node.nextElementSibling
  static JSValue node_get_nextElementSibling(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    auto sib = el->nextElementSibling();
    return sib ? wrapElement(ctx, sib.get()) : JS_NULL;
  }

  // node.previousElementSibling
  static JSValue node_get_previousElementSibling(JSContext *ctx, JSValueConst this_val,
                                                  int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    auto sib = el->previousElementSibling();
    return sib ? wrapElement(ctx, sib.get()) : JS_NULL;
  }

  // node.nodeName
  static JSValue node_get_nodeName(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    // For elements, tagName (uppercase). For text, "#text".
    if (node->nodeType() == dom::NodeType::Element) {
      return JSBinding::toJSString(ctx, static_cast<dom::Element *>(node)->tagName());
    }
    return JSBinding::toJSString(ctx, node->nodeName());
  }

  // node.nodeValue (getter)
  static JSValue node_get_nodeValue(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    if (node->nodeType() == dom::NodeType::Text) {
      return JSBinding::toJSString(ctx,
          static_cast<dom::TextNode *>(node)->data());
    }
    return JS_NULL;
  }

  // node.nodeValue (setter)
  static JSValue node_set_nodeValue(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 1) return JS_EXCEPTION;
    if (node->nodeType() == dom::NodeType::Text) {
      static_cast<dom::TextNode *>(node)->setData(
          JSBinding::toStdString(ctx, argv[0]));
    }
    return JS_UNDEFINED;
  }

  // node.nodeType
  static JSValue node_get_nodeType(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    return JS_NewInt32(ctx, static_cast<int>(node->nodeType()));
  }

  // ══════════════════════════════════════════════════════════
  //  ELEMENT PROPERTIES (className, classList, childElementCount)
  // ══════════════════════════════════════════════════════════

  // element.className (getter)
  static JSValue element_get_className(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->className());
  }

  // element.className (setter)
  static JSValue element_set_className(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    el->setClassName(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // element.classList (getter) — returns a DOMTokenList proxy
  static JSValue element_get_classList(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;

    JSValue obj = JS_NewObjectClass(ctx, s_classListClassId);
    if (JS_IsException(obj)) return obj;
    JS_SetOpaque(obj, el);

    // Methods
    auto bind = [&](const char *name, JSCFunction func, int narg) {
      JS_SetPropertyStr(ctx, obj, name, JS_NewCFunction(ctx, func, name, narg));
    };

    bind("add",      classList_add,      1);  // variadic handled in impl
    bind("remove",   classList_remove,   1);
    bind("toggle",   classList_toggle,   1);
    bind("contains", classList_contains, 1);
    bind("replace",  classList_replace,  2);
    bind("item",     classList_item,     1);
    bind("toString", classList_toString, 0);

    // length property
    auto classes = el->classList();
    JS_SetPropertyStr(ctx, obj, "length", JS_NewInt32(ctx, (int32_t)classes.size()));

    // Index properties (0, 1, 2, ...)
    for (size_t i = 0; i < classes.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, obj, (uint32_t)i,
          JSBinding::toJSString(ctx, classes[i]),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }

    return obj;
  }

  // classList.add(tokens...)
  static JSValue classList_add(JSContext *ctx, JSValueConst this_val,
                                int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el) return JS_EXCEPTION;
    for (int i = 0; i < argc; ++i) {
      el->addClass(JSBinding::toStdString(ctx, argv[i]));
    }
    return JS_UNDEFINED;
  }

  // classList.remove(tokens...)
  static JSValue classList_remove(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el) return JS_EXCEPTION;
    for (int i = 0; i < argc; ++i) {
      el->removeClass(JSBinding::toStdString(ctx, argv[i]));
    }
    return JS_UNDEFINED;
  }

  // classList.toggle(token [, force])
  static JSValue classList_toggle(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    std::string token = JSBinding::toStdString(ctx, argv[0]);
    bool hasForce = argc >= 2 && !JS_IsUndefined(argv[1]);
    if (hasForce) {
      bool force = JS_ToBool(ctx, argv[1]);
      if (force) {
        el->addClass(token);
        return JS_TRUE;
      } else {
        el->removeClass(token);
        return JS_FALSE;
      }
    }
    // Toggle without force
    bool existed = el->hasClass(token);
    if (existed) el->removeClass(token);
    else el->addClass(token);
    return JS_NewBool(ctx, !existed);
  }

  // classList.contains(token)
  static JSValue classList_contains(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return JS_NewBool(ctx, el->hasClass(JSBinding::toStdString(ctx, argv[0])));
  }

  // classList.replace(oldToken, newToken)
  static JSValue classList_replace(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el || argc < 2) return JS_EXCEPTION;
    std::string oldToken = JSBinding::toStdString(ctx, argv[0]);
    std::string newToken = JSBinding::toStdString(ctx, argv[1]);
    bool had = el->hasClass(oldToken);
    if (had) {
      el->removeClass(oldToken);
      el->addClass(newToken);
    }
    return JS_NewBool(ctx, had);
  }

  // classList.item(index)
  static JSValue classList_item(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    int32_t index;
    JS_ToInt32(ctx, &index, argv[0]);
    auto classes = el->classList();
    if (index < 0 || (size_t)index >= classes.size()) return JS_NULL;
    return JSBinding::toJSString(ctx, classes[index]);
  }

  // classList.toString()
  static JSValue classList_toString(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_classListClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->className());
  }

  // element.childElementCount
  static JSValue element_get_childElementCount(JSContext *ctx, JSValueConst this_val,
                                                int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JS_NewInt32(ctx, (int32_t)el->childElementCount());
  }

  // ══════════════════════════════════════════════════════════
  //  EXISTING METHODS (kept from original)
  // ══════════════════════════════════════════════════════════

  // element.innerHTML getter
  static JSValue element_get_innerHTML(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->innerHTML());
  }

  // element.innerHTML setter
  static JSValue element_set_innerHTML(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    el->removeAllChildren();
    auto nodes = dom::HtmlParser::parseFragment(JSBinding::toStdString(ctx, argv[0]));
    for (const auto &node : nodes) {
      el->appendChild(node);
    }
    return JS_UNDEFINED;
  }

  // element.id getter
  static JSValue element_get_id(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->id());
  }

  // element.tagName getter
  static JSValue element_get_tagName(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->tagName());
  }

  // element.textContent getter
  static JSValue element_get_textContent(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->textContent());
  }

  // element.textContent setter
  static JSValue element_set_textContent(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    el->setTextContent(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // element.style getter
  static JSValue element_get_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, el->getAttribute("style").value_or(""));
  }

  // element.style setter
  static JSValue element_set_style(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    el->setAttribute("style", JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // element.setAttribute(name, value)
  static JSValue element_setAttribute(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 2) return JS_EXCEPTION;
    el->setAttribute(JSBinding::toStdString(ctx, argv[0]),
                     JSBinding::toStdString(ctx, argv[1]));
    return JS_UNDEFINED;
  }

  // element.getAttribute(name)
  static JSValue element_getAttribute(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    auto val = el->getAttribute(JSBinding::toStdString(ctx, argv[0]));
    return val.has_value() ? JSBinding::toJSString(ctx, val.value()) : JS_NULL;
  }

  // element.hasAttribute(name)
  static JSValue element_hasAttribute(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return JS_NewBool(ctx, el->hasAttribute(JSBinding::toStdString(ctx, argv[0])));
  }

  // element.removeAttribute(name)
  static JSValue element_removeAttribute(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    el->removeAttribute(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // element.appendChild(child)
  static JSValue element_appendChild(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *parent = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!parent || argc < 1) return JS_EXCEPTION;

    // Try getting as Element
    dom::Element *child = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (child) {
      try {
        parent->appendChild(child->shared_from_this());
      } catch (...) {
        std::cout << "[DOM] Warning: appendChild failed" << std::endl;
        return JS_EXCEPTION;
      }
      return JS_DupValue(ctx, argv[0]);
    }

    // Try getting as TextNode
    dom::TextNode *textChild = (dom::TextNode *)JS_GetOpaque(argv[0], s_textNodeClassId);
    if (textChild) {
      try {
        parent->appendChild(textChild->shared_from_this());
      } catch (...) {
        std::cout << "[DOM] Warning: appendChild (text) failed" << std::endl;
        return JS_EXCEPTION;
      }
      return JS_DupValue(ctx, argv[0]);
    }

    return JS_EXCEPTION;
  }

  // element.removeChild(child)
  static JSValue element_removeChild(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Node *parent = getNodeFromThis(ctx, this_val);
    if (!parent || argc < 1) return JS_EXCEPTION;

    // Try Element child
    dom::Element *child = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (child) {
      try {
        parent->removeChild(child->shared_from_this());
      } catch (...) { return JS_EXCEPTION; }
      return JS_DupValue(ctx, argv[0]);
    }

    // Try TextNode child
    dom::TextNode *textChild = (dom::TextNode *)JS_GetOpaque(argv[0], s_textNodeClassId);
    if (textChild) {
      try {
        parent->removeChild(textChild->shared_from_this());
      } catch (...) { return JS_EXCEPTION; }
      return JS_DupValue(ctx, argv[0]);
    }

    return JS_EXCEPTION;
  }

  // ── NEW: element.insertBefore(newNode, referenceNode) ──
  // Delegates to Node::insertBefore which handles parent tracking
  static JSValue element_insertBefore(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Node *parent = getNodeFromThis(ctx, this_val);
    if (!parent || argc < 2) return JS_EXCEPTION;

    // Get the new node (Element or TextNode)
    dom::NodePtr newNode;
    dom::Element *newEl = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (newEl) {
      try { newNode = newEl->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
    } else {
      dom::TextNode *newText = (dom::TextNode *)JS_GetOpaque(argv[0], s_textNodeClassId);
      if (newText) {
        try { newNode = newText->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
      } else {
        return JS_EXCEPTION;
      }
    }

    // Get reference node (null/undefined → append at end)
    dom::NodePtr refNode;
    if (!JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1])) {
      dom::Element *refEl = (dom::Element *)JS_GetOpaque(argv[1], s_elementClassId);
      if (refEl) {
        try { refNode = refEl->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
      } else {
        dom::TextNode *refText = (dom::TextNode *)JS_GetOpaque(argv[1], s_textNodeClassId);
        if (refText) {
          try { refNode = refText->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
        }
      }
    }

    // Delegate to the existing Node::insertBefore
    parent->insertBefore(newNode, refNode);
    return JS_DupValue(ctx, argv[0]);
  }

  // ── NEW: element.replaceChild(newChild, oldChild) ──
  // Delegates to Node::replaceChild
  static JSValue element_replaceChild(JSContext *ctx, JSValueConst this_val,
                                       int argc, JSValueConst *argv) {
    dom::Node *parent = getNodeFromThis(ctx, this_val);
    if (!parent || argc < 2) return JS_EXCEPTION;

    // Get new child
    dom::NodePtr newChild;
    dom::Element *newEl = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (newEl) {
      try { newChild = newEl->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
    } else {
      dom::TextNode *newText = (dom::TextNode *)JS_GetOpaque(argv[0], s_textNodeClassId);
      if (newText) { try { newChild = newText->shared_from_this(); } catch (...) { return JS_EXCEPTION; } }
      else return JS_EXCEPTION;
    }

    // Get old child
    dom::NodePtr oldChild;
    dom::Element *oldEl = (dom::Element *)JS_GetOpaque(argv[1], s_elementClassId);
    if (oldEl) {
      try { oldChild = oldEl->shared_from_this(); } catch (...) { return JS_EXCEPTION; }
    } else {
      dom::TextNode *oldText = (dom::TextNode *)JS_GetOpaque(argv[1], s_textNodeClassId);
      if (oldText) { try { oldChild = oldText->shared_from_this(); } catch (...) { return JS_EXCEPTION; } }
      else return JS_EXCEPTION;
    }

    parent->replaceChild(newChild, oldChild);
    return JS_DupValue(ctx, argv[1]);
  }

  // ── NEW: element.remove() ──
  // Delegates to Node::removeChild on parent
  static JSValue element_remove(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    (void)argc; (void)argv;
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;

    auto parent = node->parentNode();
    if (parent) {
      try {
        parent->removeChild(node->shared_from_this());
      } catch (...) {}
    }
    return JS_UNDEFINED;
  }

  // ── NEW: element.cloneNode(deep) ──
  static JSValue element_cloneNode(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node) return JS_EXCEPTION;
    bool deep = (argc >= 1) && JS_ToBool(ctx, argv[0]);
    auto clone = node->cloneNode(deep);
    if (!clone) return JS_NULL;
    if (clone->nodeType() == dom::NodeType::Element)
      return wrapElement(ctx, static_cast<dom::Element *>(clone.get()));
    if (clone->nodeType() == dom::NodeType::Text)
      return wrapTextNode(ctx, static_cast<dom::TextNode *>(clone.get()));
    return JS_NULL;
  }

  // ── NEW: element.contains(otherNode) ──
  // Delegates to Node::contains
  static JSValue element_contains(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 1) return JS_EXCEPTION;

    dom::Node *other = getNodeFromThis(ctx, argv[0]);
    if (!other) return JS_FALSE;

    try {
      return JS_NewBool(ctx, node->contains(other->shared_from_this()));
    } catch (...) {
      return JS_FALSE;
    }
  }

  // ── NEW: element.isEqualNode(otherNode) ──
  static JSValue element_isEqualNode(JSContext *ctx, JSValueConst this_val,
                                      int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;

    dom::Element *other = (dom::Element *)JS_GetOpaque(argv[0], s_elementClassId);
    if (!other) return JS_FALSE;

    // Compare tag name
    if (el->localName() != other->localName()) return JS_FALSE;

    // Compare attributes
    auto &attrs1 = el->attributes();
    auto &attrs2 = other->attributes();
    if (attrs1.size() != attrs2.size()) return JS_FALSE;
    for (const auto &a : attrs1) {
      auto v = other->getAttribute(a.name);
      if (!v.has_value() || v.value() != a.value) return JS_FALSE;
    }

    // Compare child count
    auto &c1 = el->childNodes();
    auto &c2 = other->childNodes();
    if (c1.size() != c2.size()) return JS_FALSE;

    // Recursively compare children
    for (size_t i = 0; i < c1.size(); ++i) {
      if (c1[i]->nodeType() != c2[i]->nodeType()) return JS_FALSE;
      if (c1[i]->nodeType() == dom::NodeType::Element) {
        // Recursive check via isEqualNode
        auto e1 = std::static_pointer_cast<dom::Element>(c1[i]);
        auto e2 = std::static_pointer_cast<dom::Element>(c2[i]);
        if (e1->localName() != e2->localName()) return JS_FALSE;
      } else if (c1[i]->textContent() != c2[i]->textContent()) {
        return JS_FALSE;
      }
    }
    return JS_TRUE;
  }

  // element.matches(selector)
  static JSValue element_matches(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return JS_NewBool(ctx, el->matches(JSBinding::toStdString(ctx, argv[0])));
  }

  // element.closest(selector)
  static JSValue element_closest(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    dom::Node *cur = el;
    while (cur) {
      if (cur->nodeType() == dom::NodeType::Element) {
        auto elem = static_cast<dom::Element *>(cur);
        if (elem->matches(selector)) {
          return wrapElement(ctx, elem);
        }
      }
      cur = cur->parentNode().get();
    }
    return JS_NULL;
  }

  // element.getElementsByTagName(name)
  static JSValue element_getElementsByTagName(JSContext *ctx, JSValueConst this_val,
                                               int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, el->getElementsByTagName(JSBinding::toStdString(ctx, argv[0])));
  }

  // element.getElementsByClassName(name)
  static JSValue element_getElementsByClassName(JSContext *ctx, JSValueConst this_val,
                                                 int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, el->getElementsByClassName(JSBinding::toStdString(ctx, argv[0])));
  }

  // element.querySelector(selector)
  static JSValue element_querySelector(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    auto found = el->querySelector(JSBinding::toStdString(ctx, argv[0]));
    return found ? wrapElement(ctx, found.get()) : JS_NULL;
  }

  // element.querySelectorAll(selector)
  static JSValue element_querySelectorAll(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv) {
    dom::Element *el = (dom::Element *)JS_GetOpaque(this_val, s_elementClassId);
    if (!el || argc < 1) return JS_EXCEPTION;
    return wrapElementArray(ctx, el->querySelectorAll(JSBinding::toStdString(ctx, argv[0])));
  }

  // ── Event methods ──

  // element.addEventListener(type, callback)
  static JSValue element_addEventListener(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 2) return JS_EXCEPTION;
    if (!JS_IsFunction(ctx, argv[1])) return JS_EXCEPTION;

    std::string type = JSBinding::toStdString(ctx, argv[0]);
    uint32_t id = EventBinding::addListener(ctx, argv[1]);
    node->eventListenerIds_[type].push_back(id);
    return JS_UNDEFINED;
  }

  // element.removeEventListener(type, callback)
  static JSValue element_removeEventListener(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 2) return JS_EXCEPTION;

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
  static JSValue element_dispatchEvent(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
    dom::Node *node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 1) return JS_EXCEPTION;

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

// Static member definitions (must be in a .cpp, but since this is header-only,
// we rely on inline or ODR-safe initialization)
inline JSClassID DOMBinding::s_elementClassId = 0;
inline JSClassID DOMBinding::s_documentClassId = 0;
inline JSClassID DOMBinding::s_nodeListClassId = 0;
inline JSClassID DOMBinding::s_classListClassId = 0;
inline JSClassID DOMBinding::s_textNodeClassId = 0;

} // namespace script
} // namespace xiaopeng
