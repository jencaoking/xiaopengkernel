#pragma once

#include <cstring> // For memset
#include <dom/dom.hpp>
#include <dom/html_parser.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <quickjs.h>
#include <script/event_binding.hpp>
#include <script/js_binding.hpp>

namespace xiaopeng {
namespace script {

namespace detail {

// Helper function to get Element from JS this value
inline dom::Element* GetElementFromThis(JSContext* ctx, JSValueConst this_val, 
                                       JSClassID element_class_id) {
  (void)ctx;
  return static_cast<dom::Element*>(JS_GetOpaque(this_val, element_class_id));
}

// Helper function to get Document from JS this value
inline dom::Document* GetDocumentFromThis(JSContext* ctx, JSValueConst this_val,
                                          JSClassID document_class_id) {
  (void)ctx;
  return static_cast<dom::Document*>(JS_GetOpaque(this_val, document_class_id));
}

// Helper function to check if JS value is valid element
inline bool IsValidElement(JSContext* ctx, JSValueConst val, 
                          JSClassID element_class_id) {
  (void)ctx;
  return JS_GetOpaque(val, element_class_id) != nullptr;
}

// Helper to convert optional string to JS value
inline JSValue OptionalToJSString(JSContext* ctx, const std::optional<std::string>& opt) {
  if (opt && !opt->empty()) {
    return JSBinding::toJSString(ctx, *opt);
  }
  return JS_NULL;
}

// Helper to get string argument with default
inline std::string GetStringArg(JSContext* ctx, JSValueConst* argv, 
                               int argc, int index,
                               const std::string& default_val = "") {
  if (index >= argc) {
    return default_val;
  }
  return JSBinding::toStdString(ctx, argv[index]);
}

// Helper to get boolean argument with default
inline bool GetBoolArg(JSContext* ctx, JSValueConst* argv,
                      int argc, int index, bool default_val = false) {
  if (index >= argc) {
    return default_val;
  }
  return JS_ToBool(ctx, argv[index]) != 0;
}

// Helper to get integer argument with default
inline int GetIntArg(JSContext* ctx, JSValueConst* argv,
                    int argc, int index, int default_val = 0) {
  if (index >= argc) {
    return default_val;
  }
  int32_t val = 0;
  if (JS_ToInt32(ctx, &val, argv[index]) == 0) {
    return static_cast<int>(val);
  }
  return default_val;
}

} // namespace detail

// ============================================================================
// Text Node Binding - For Text Node Support
// ============================================================================
class TextBinding {
public:
  static void registerBinding(JSContext* ctx) {
    JS_NewClassID(&s_textClassId);
    JSClassDef textDef;
    memset(&textDef, 0, sizeof(JSClassDef));
    textDef.class_name = "Text";
    textDef.finalizer = [](JSRuntime* /*rt*/, JSValue /*val*/) {
      // We don't own the DOM pointer
    };
    JS_NewClass(JS_GetRuntime(ctx), s_textClassId, &textDef);
  }

  static JSValue wrapTextNode(JSContext* ctx, dom::TextNode* text) {
    JSValue obj = JS_NewObjectClass(ctx, s_textClassId);
    if (JS_IsException(obj)) {
      return obj;
    }
    JS_SetOpaque(obj, text);

    // textContent property
    JSAtom atom = JS_NewAtom(ctx, "textContent");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, text_get_textContent, "get_textContent", 0),
        JS_NewCFunction(ctx, text_set_textContent, "set_textContent", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // wholeText property (read-only)
    atom = JS_NewAtom(ctx, "wholeText");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, text_get_wholeText, "get_wholeText", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // splitText method
    JS_SetPropertyStr(
        ctx, obj, "splitText",
        JS_NewCFunction(ctx, text_splitText, "splitText", 1));

    // nodeValue property
    atom = JS_NewAtom(ctx, "nodeValue");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, text_get_nodeValue, "get_nodeValue", 0),
        JS_NewCFunction(ctx, text_set_nodeValue, "set_nodeValue", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // length property
    atom = JS_NewAtom(ctx, "length");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, text_get_length, "get_length", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    return obj;
  }

private:
  static JSClassID s_textClassId;

  static JSValue text_get_textContent(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* textNode = static_cast<dom::TextNode*>(
        JS_GetOpaque(this_val, s_textClassId));
    if (!textNode) {
      return JS_EXCEPTION;
    }
    return JSBinding::toJSString(ctx, textNode->textContent());
  }

  static JSValue text_set_textContent(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    auto* textNode = static_cast<dom::TextNode*>(
        JS_GetOpaque(this_val, s_textClassId));
    if (!textNode || argc < 1) {
      return JS_EXCEPTION;
    }
    std::string text = JSBinding::toStdString(ctx, argv[0]);
    textNode->setTextContent(text);
    return JS_UNDEFINED;
  }

  static JSValue text_get_wholeText(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* textNode = static_cast<dom::TextNode*>(
        JS_GetOpaque(this_val, s_textClassId));
    if (!textNode) {
      return JS_EXCEPTION;
    }
    // wholeText returns the concatenation of all adjacent Text nodes
    // Simplified: just return this node's text
    return JSBinding::toJSString(ctx, textNode->textContent());
  }

  static JSValue text_splitText(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto* textNode = static_cast<dom::TextNode*>(
        JS_GetOpaque(this_val, s_textClassId));
    if (!textNode || argc < 1) {
      return JS_EXCEPTION;
    }
    int32_t offset = 0;
    if (JS_ToInt32(ctx, &offset, argv[0]) != 0) {
      return JS_EXCEPTION;
    }
    auto newText = textNode->splitText(static_cast<size_t>(offset));
    if (newText) {
      return wrapTextNode(ctx, newText.get());
    }
    return JS_EXCEPTION;
  }

  static JSValue text_get_nodeValue(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    return text_get_textContent(ctx, this_val, argc, argv);
  }

  static JSValue text_set_nodeValue(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    return text_set_textContent(ctx, this_val, argc, argv);
  }

  static JSValue text_get_length(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* textNode = static_cast<dom::TextNode*>(
        JS_GetOpaque(this_val, s_textClassId));
    if (!textNode) {
      return JS_EXCEPTION;
    }
    return JS_NewUint32(ctx, static_cast<uint32_t>(textNode->textContent().length()));
  }
};

// Static definition
inline JSClassID TextBinding::s_textClassId = 0;

// ============================================================================
// DOMBinding - Main DOM API Binding Class
// ============================================================================
class DOMBinding {
public:
  static void registerBinding(JSContext* ctx) {
    JS_NewClassID(&s_elementClassId);
    JS_NewClassID(&s_documentClassId);

    // Define Element class
    JSClassDef elementDef;
    memset(&elementDef, 0, sizeof(JSClassDef));
    elementDef.class_name = "Element";
    elementDef.finalizer = [](JSRuntime* /*rt*/, JSValue /*val*/) {
      // We don't own the DOM pointer
    };
    JS_NewClass(JS_GetRuntime(ctx), s_elementClassId, &elementDef);

    // Define Document class
    JSClassDef docDef;
    memset(&docDef, 0, sizeof(JSClassDef));
    docDef.class_name = "Document";
    docDef.finalizer = [](JSRuntime* /*rt*/, JSValue /*val*/) {
      // We don't own the Document pointer
    };
    JS_NewClass(JS_GetRuntime(ctx), s_documentClassId, &docDef);

    // Register Text binding
    TextBinding::registerBinding(ctx);
  }

  static JSValue wrapDocument(JSContext* ctx,
                              std::shared_ptr<dom::Document> doc) {
    JSValue obj = JS_NewObjectClass(ctx, s_documentClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, doc.get());

    // === Document Methods ===
    JS_SetPropertyStr(
        ctx, obj, "getElementById",
        JS_NewCFunction(ctx, document_getElementById, "getElementById", 1));
    JS_SetPropertyStr(
        ctx, obj, "getElementsByTagName",
        JS_NewCFunction(ctx, document_getElementsByTagName, "getElementsByTagName", 1));
    JS_SetPropertyStr(
        ctx, obj, "getElementsByClassName",
        JS_NewCFunction(ctx, document_getElementsByClassName, "getElementsByClassName", 1));
    JS_SetPropertyStr(
        ctx, obj, "querySelector",
        JS_NewCFunction(ctx, document_querySelector, "querySelector", 1));
    JS_SetPropertyStr(
        ctx, obj, "querySelectorAll",
        JS_NewCFunction(ctx, document_querySelectorAll, "querySelectorAll", 1));
    JS_SetPropertyStr(
        ctx, obj, "createElement",
        JS_NewCFunction(ctx, document_createElement, "createElement", 1));
    
    // === NEW: Document creation methods ===
    JS_SetPropertyStr(
        ctx, obj, "createTextNode",
        JS_NewCFunction(ctx, document_createTextNode, "createTextNode", 1));
    JS_SetPropertyStr(
        ctx, obj, "createComment",
        JS_NewCFunction(ctx, document_createComment, "createComment", 1));
    JS_SetPropertyStr(
        ctx, obj, "createDocumentFragment",
        JS_NewCFunction(ctx, document_createDocumentFragment, "createDocumentFragment", 0));

    // === Event methods on document ===
    JS_SetPropertyStr(
        ctx, obj, "addEventListener",
        JS_NewCFunction(ctx, element_addEventListener, "addEventListener", 2));
    JS_SetPropertyStr(ctx, obj, "removeEventListener",
                      JS_NewCFunction(ctx, element_removeEventListener,
                                      "removeEventListener", 2));
    JS_SetPropertyStr(
        ctx, obj, "dispatchEvent",
        JS_NewCFunction(ctx, element_dispatchEvent, "dispatchEvent", 1));

    // === NEW: Document.body property ===
    JSAtom atom = JS_NewAtom(ctx, "body");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_body, "get_body", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // === NEW: Document.documentElement property ===
    atom = JS_NewAtom(ctx, "documentElement");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_documentElement, "get_documentElement", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // === NEW: Document.title property ===
    atom = JS_NewAtom(ctx, "title");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, document_get_title, "get_title", 0),
        JS_NewCFunction(ctx, document_set_title, "set_title", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    return obj;
  }

  static JSValue wrapElement(JSContext* ctx, dom::Element* element) {
    if (!element)
      return JS_NULL;

    JSValue obj = JS_NewObjectClass(ctx, s_elementClassId);
    if (JS_IsException(obj))
      return obj;

    JS_SetOpaque(obj, element);

    // === Properties ===
    // innerHTML
    JSAtom atom = JS_NewAtom(ctx, "innerHTML");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_innerHTML, "get_innerHTML", 0),
        JS_NewCFunction(ctx, element_set_innerHTML, "set_innerHTML", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // id
    atom = JS_NewAtom(ctx, "id");
    JS_DefinePropertyGetSet(ctx, obj, atom,
                            JS_NewCFunction(ctx, element_get_id, "get_id", 0),
                            JS_NewCFunction(ctx, element_set_id, "set_id", 1),
                            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // tagName
    atom = JS_NewAtom(ctx, "tagName");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_tagName, "get_tagName", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // textContent
    atom = JS_NewAtom(ctx, "textContent");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_textContent, "get_textContent", 0),
        JS_NewCFunction(ctx, element_set_textContent, "set_textContent", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // style
    atom = JS_NewAtom(ctx, "style");
    JS_DefinePropertyGetSet(
        ctx, obj, atom, JS_NewCFunction(ctx, element_get_style, "get_style", 0),
        JS_NewCFunction(ctx, element_set_style, "set_style", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // className
    atom = JS_NewAtom(ctx, "className");
    JS_DefinePropertyGetSet(ctx, obj, atom,
                            JS_NewCFunction(ctx, element_get_className, "get_className", 0),
                            JS_NewCFunction(ctx, element_set_className, "set_className", 1),
                            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // === NEW: Element scroll/scroll properties ===
    // scrollTop
    atom = JS_NewAtom(ctx, "scrollTop");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_scrollTop, "get_scrollTop", 0),
        JS_NewCFunction(ctx, element_set_scrollTop, "set_scrollTop", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // scrollLeft
    atom = JS_NewAtom(ctx, "scrollLeft");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_scrollLeft, "get_scrollLeft", 0),
        JS_NewCFunction(ctx, element_set_scrollLeft, "set_scrollLeft", 1),
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // scrollWidth (read-only)
    atom = JS_NewAtom(ctx, "scrollWidth");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_scrollWidth, "get_scrollWidth", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // scrollHeight (read-only)
    atom = JS_NewAtom(ctx, "scrollHeight");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_scrollHeight, "get_scrollHeight", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // clientWidth (read-only)
    atom = JS_NewAtom(ctx, "clientWidth");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_clientWidth, "get_clientWidth", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // clientHeight (read-only)
    atom = JS_NewAtom(ctx, "clientHeight");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_clientHeight, "get_clientHeight", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // offsetWidth (read-only)
    atom = JS_NewAtom(ctx, "offsetWidth");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_offsetWidth, "get_offsetWidth", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // offsetHeight (read-only)
    atom = JS_NewAtom(ctx, "offsetHeight");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_offsetHeight, "get_offsetHeight", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // offsetTop (read-only)
    atom = JS_NewAtom(ctx, "offsetTop");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_offsetTop, "get_offsetTop", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // offsetLeft (read-only)
    atom = JS_NewAtom(ctx, "offsetLeft");
    JS_DefinePropertyGetSet(
        ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_offsetLeft, "get_offsetLeft", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // === Navigation Properties ===
    // parentNode
    atom = JS_NewAtom(ctx, "parentNode");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_parentNode, "get_parentNode", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // childNodes
    atom = JS_NewAtom(ctx, "childNodes");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_childNodes, "get_childNodes", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // children
    atom = JS_NewAtom(ctx, "children");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_children, "get_children", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // firstElementChild
    atom = JS_NewAtom(ctx, "firstElementChild");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_firstElementChild, "get_firstElementChild", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // lastElementChild
    atom = JS_NewAtom(ctx, "lastElementChild");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_lastElementChild, "get_lastElementChild", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // nextElementSibling
    atom = JS_NewAtom(ctx, "nextElementSibling");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_nextElementSibling, "get_nextElementSibling", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // previousElementSibling
    atom = JS_NewAtom(ctx, "previousElementSibling");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_previousElementSibling, "get_previousElementSibling", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // childElementCount
    atom = JS_NewAtom(ctx, "childElementCount");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_childElementCount, "get_childElementCount", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // ownerDocument
    atom = JS_NewAtom(ctx, "ownerDocument");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_ownerDocument, "get_ownerDocument", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // nodeType
    atom = JS_NewAtom(ctx, "nodeType");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_nodeType, "get_nodeType", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // nodeName
    atom = JS_NewAtom(ctx, "nodeName");
    JS_DefinePropertyGetSet(ctx, obj, atom,
        JS_NewCFunction(ctx, element_get_nodeName, "get_nodeName", 0),
        JS_UNDEFINED,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);

    // === NEW: dataset property ===
    JSValue datasetObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, datasetObj, "get",
        JS_NewCFunction(ctx, dataset_get, "get", 1));
    JS_SetPropertyStr(ctx, datasetObj, "set",
        JS_NewCFunction(ctx, dataset_set, "set", 2));
    JS_SetPropertyStr(ctx, datasetObj, "has",
        JS_NewCFunction(ctx, dataset_has, "has", 1));
    JS_SetPropertyStr(ctx, datasetObj, "remove",
        JS_NewCFunction(ctx, dataset_remove, "remove", 1));
    JS_SetPropertyStr(ctx, obj, "dataset", datasetObj);

    // === Methods ===
    // setAttribute
    JS_SetPropertyStr(ctx, obj, "setAttribute",
        JS_NewCFunction(ctx, element_setAttribute, "setAttribute", 2));
    JS_SetPropertyStr(ctx, obj, "getAttribute",
        JS_NewCFunction(ctx, element_getAttribute, "getAttribute", 1));
    JS_SetPropertyStr(ctx, obj, "hasAttribute",
        JS_NewCFunction(ctx, element_hasAttribute, "hasAttribute", 1));
    JS_SetPropertyStr(ctx, obj, "removeAttribute",
        JS_NewCFunction(ctx, element_removeAttribute, "removeAttribute", 1));
    JS_SetPropertyStr(ctx, obj, "getElementsByTagName",
        JS_NewCFunction(ctx, element_getElementsByTagName, "getElementsByTagName", 1));
    JS_SetPropertyStr(ctx, obj, "getElementsByClassName",
        JS_NewCFunction(ctx, element_getElementsByClassName, "getElementsByClassName", 1));
    JS_SetPropertyStr(ctx, obj, "querySelector",
        JS_NewCFunction(ctx, element_querySelector, "querySelector", 1));
    JS_SetPropertyStr(ctx, obj, "querySelectorAll",
        JS_NewCFunction(ctx, element_querySelectorAll, "querySelectorAll", 1));

    // classList
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

    // === Node manipulation methods ===
    JS_SetPropertyStr(ctx, obj, "appendChild",
        JS_NewCFunction(ctx, element_appendChild, "appendChild", 1));
    JS_SetPropertyStr(ctx, obj, "removeChild",
        JS_NewCFunction(ctx, element_removeChild, "removeChild", 1));
    JS_SetPropertyStr(ctx, obj, "insertBefore",
        JS_NewCFunction(ctx, element_insertBefore, "insertBefore", 2));
    JS_SetPropertyStr(ctx, obj, "replaceChild",
        JS_NewCFunction(ctx, element_replaceChild, "replaceChild", 2));
    JS_SetPropertyStr(ctx, obj, "cloneNode",
        JS_NewCFunction(ctx, element_cloneNode, "cloneNode", 1));
    JS_SetPropertyStr(ctx, obj, "hasChildNodes",
        JS_NewCFunction(ctx, element_hasChildNodes, "hasChildNodes", 0));
    JS_SetPropertyStr(ctx, obj, "normalize",
        JS_NewCFunction(ctx, element_normalize, "normalize", 0));
    JS_SetPropertyStr(ctx, obj, "contains",
        JS_NewCFunction(ctx, element_contains, "contains", 1));

    // === Query methods ===
    JS_SetPropertyStr(ctx, obj, "matches",
        JS_NewCFunction(ctx, element_matches, "matches", 1));
    JS_SetPropertyStr(ctx, obj, "closest",
        JS_NewCFunction(ctx, element_closest, "closest", 1));

    // === NEW: Element action methods ===
    JS_SetPropertyStr(ctx, obj, "scroll",
        JS_NewCFunction(ctx, element_scroll, "scroll", 2));
    JS_SetPropertyStr(ctx, obj, "scrollTo",
        JS_NewCFunction(ctx, element_scrollTo, "scrollTo", 2));
    JS_SetPropertyStr(ctx, obj, "scrollBy",
        JS_NewCFunction(ctx, element_scrollBy, "scrollBy", 2));
    JS_SetPropertyStr(ctx, obj, "focus",
        JS_NewCFunction(ctx, element_focus, "focus", 0));
    JS_SetPropertyStr(ctx, obj, "blur",
        JS_NewCFunction(ctx, element_blur, "blur", 0));
    JS_SetPropertyStr(ctx, obj, "click",
        JS_NewCFunction(ctx, element_click, "click", 0));
    JS_SetPropertyStr(ctx, obj, "getBoundingClientRect",
        JS_NewCFunction(ctx, element_getBoundingClientRect, "getBoundingClientRect", 0));

    // === Event methods ===
    JS_SetPropertyStr(ctx, obj, "addEventListener",
        JS_NewCFunction(ctx, element_addEventListener, "addEventListener", 2));
    JS_SetPropertyStr(ctx, obj, "removeEventListener",
        JS_NewCFunction(ctx, element_removeEventListener, "removeEventListener", 2));
    JS_SetPropertyStr(ctx, obj, "dispatchEvent",
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
  static inline std::vector<std::weak_ptr<dom::Element>> s_createdElements;
  static inline std::mutex s_mutex;

  // === Helper Functions ===
  static dom::Node* getNodeFromThis(JSContext* ctx, JSValueConst this_val) {
    (void)ctx;
    dom::Node* node = (dom::Node*)JS_GetOpaque(this_val, s_elementClassId);
    if (!node) {
      node = (dom::Node*)JS_GetOpaque(this_val, s_documentClassId);
    }
    return node;
  }

  // === Document Methods ===
  static JSValue document_getElementById(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string id = JSBinding::toStdString(ctx, argv[0]);
    auto element = doc->getElementById(id);
    if (element) {
      return wrapElement(ctx, element.get());
    }
    return JS_NULL;
  }

  static JSValue document_createElement(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string tagName = JSBinding::toStdString(ctx, argv[0]);
    auto element = doc->createElement(tagName);
    if (element) {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_createdElements.push_back(element);
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

  static JSValue document_createTextNode(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string data = JSBinding::toStdString(ctx, argv[0]);
    auto textNode = doc->createTextNode(data);
    if (textNode) {
      return TextBinding::wrapTextNode(ctx, textNode.get());
    }
    return JS_NULL;
  }

  static JSValue document_createComment(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string data = JSBinding::toStdString(ctx, argv[0]);
    auto comment = doc->createComment(data);
    if (comment) {
      // Return as JS string for simplicity
      return JSBinding::toJSString(ctx, data);
    }
    return JS_NULL;
  }

  static JSValue document_createDocumentFragment(JSContext* ctx, JSValueConst this_val,
                                                 int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;

    // Create a document fragment (returns empty array-like object)
    JSValue fragment = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, fragment, "appendChild",
        JS_NewCFunction(ctx, fragment_appendChild, "appendChild", 1));
    JS_SetPropertyStr(ctx, fragment, "childNodes",
        JS_NewCFunction(ctx, fragment_get_childNodes, "get_childNodes", 0));
    JS_SetPropertyStr(ctx, fragment, "children",
        JS_NewCFunction(ctx, fragment_get_children, "get_children", 0));
    return fragment;
  }

  static JSValue document_get_body(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;

    auto body = doc->getElementsByTagName("body");
    if (!body.empty()) {
      return wrapElement(ctx, body[0].get());
    }
    return JS_NULL;
  }

  static JSValue document_get_documentElement(JSContext* ctx, JSValueConst this_val,
                                              int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;

    // Return the document element (usually <html>)
    auto html = doc->getElementsByTagName("html");
    if (!html.empty()) {
      return wrapElement(ctx, html[0].get());
    }
    return JS_NULL;
  }

  static JSValue document_get_title(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc) return JS_EXCEPTION;

    auto title = doc->getElementsByTagName("title");
    if (!title.empty() && title[0]->hasChildNodes()) {
      auto firstChild = title[0]->firstElementChild();
      if (firstChild) {
        return JSBinding::toJSString(ctx, firstChild->textContent());
      }
    }
    return JS_NewString(ctx, "");
  }

  static JSValue document_set_title(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string newTitle = JSBinding::toStdString(ctx, argv[0]);
    auto title = doc->getElementsByTagName("title");
    if (!title.empty()) {
      auto firstChild = title[0]->firstElementChild();
      if (firstChild) {
        firstChild->setTextContent(newTitle);
      } else {
        // Create text node
        auto textNode = doc->createTextNode(newTitle);
        title[0]->appendChild(textNode);
      }
    }
    return JS_UNDEFINED;
  }

  static JSValue document_getElementsByTagName(JSContext* ctx, JSValueConst this_val,
                                                int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto elements = doc->getElementsByTagName(name);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  static JSValue document_getElementsByClassName(JSContext* ctx, JSValueConst this_val,
                                                  int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string className = JSBinding::toStdString(ctx, argv[0]);
    auto elements = doc->getElementsByClassName(className);

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  static JSValue document_querySelector(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    if (selector[0] == '#' && selector.length() > 1) {
      auto element = doc->getElementById(selector.substr(1));
      if (element) return wrapElement(ctx, element.get());
    } else {
      auto elements = doc->getElementsByTagName(selector);
      if (!elements.empty()) return wrapElement(ctx, elements[0].get());
    }
    return JS_NULL;
  }

  static JSValue document_querySelectorAll(JSContext* ctx, JSValueConst this_val,
                                           int argc, JSValueConst* argv) {
    auto* doc = detail::GetDocumentFromThis(ctx, this_val, s_documentClassId);
    if (!doc || argc < 1) return JS_EXCEPTION;

    std::string selector = JSBinding::toStdString(ctx, argv[0]);
    std::vector<dom::ElementPtr> elements;

    if (selector[0] == '.' && selector.length() > 1) {
      elements = doc->getElementsByClassName(selector.substr(1));
    } else if (selector[0] == '#' && selector.length() > 1) {
      auto element = doc->getElementById(selector.substr(1));
      if (element) elements.push_back(element);
    } else {
      elements = doc->getElementsByTagName(selector);
    }

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // === Element Property Getters/Setters ===
  static JSValue element_get_innerHTML(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->innerHTML());
  }

  static JSValue element_set_innerHTML(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;

    std::string html = JSBinding::toStdString(ctx, argv[0]);
    elem->removeAllChildren();
    auto nodes = dom::HtmlParser::parseFragment(html);
    for (const auto& node : nodes) {
      elem->appendChild(node);
    }
    return JS_UNDEFINED;
  }

  static JSValue element_get_id(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->id());
  }

  static JSValue element_set_id(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    elem->setAttribute("id", JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  static JSValue element_get_tagName(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->tagName());
  }

  static JSValue element_get_textContent(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->textContent());
  }

  static JSValue element_set_textContent(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    elem->setTextContent(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  static JSValue element_get_style(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto style = elem->getAttribute("style");
    return JSBinding::toJSString(ctx, style.value_or(""));
  }

  static JSValue element_set_style(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    elem->setAttribute("style", JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  static JSValue element_get_className(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->className());
  }

  static JSValue element_set_className(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    elem->setClassName(JSBinding::toStdString(ctx, argv[0]));
    return JS_UNDEFINED;
  }

  // === NEW: Scroll Properties ===
  static JSValue element_get_scrollTop(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 0); // Simplified: return 0
  }

  static JSValue element_set_scrollTop(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    // Simplified: no-op
    return JS_UNDEFINED;
  }

  static JSValue element_get_scrollLeft(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 0); // Simplified
  }

  static JSValue element_set_scrollLeft(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
  }

  static JSValue element_get_scrollWidth(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 800); // Simplified
  }

  static JSValue element_get_scrollHeight(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 600); // Simplified
  }

  static JSValue element_get_clientWidth(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 800); // Simplified
  }

  static JSValue element_get_clientHeight(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 600); // Simplified
  }

  static JSValue element_get_offsetWidth(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 100); // Simplified
  }

  static JSValue element_get_offsetHeight(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 100); // Simplified
  }

  static JSValue element_get_offsetTop(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 0); // Simplified
  }

  static JSValue element_get_offsetLeft(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewInt32(ctx, 0); // Simplified
  }

  // === Navigation Properties ===
  static JSValue element_get_parentNode(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto parent = elem->parentNode();
    if (parent && parent->nodeType() == dom::NodeType::Element) {
      return wrapElement(ctx, std::static_pointer_cast<dom::Element>(parent).get());
    }
    return JS_NULL;
  }

  static JSValue element_get_childNodes(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;

    JSValue arr = JS_NewArray(ctx);
    size_t index = 0;
    for (const auto& child : elem->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        JS_DefinePropertyValueUint32(ctx, arr, index++,
            wrapElement(ctx, std::static_pointer_cast<dom::Element>(child).get()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      } else if (child->nodeType() == dom::NodeType::Text) {
        JS_DefinePropertyValueUint32(ctx, arr, index++,
            JSBinding::toJSString(ctx, child->textContent()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      }
    }
    return arr;
  }

  static JSValue element_get_children(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;

    JSValue arr = JS_NewArray(ctx);
    size_t index = 0;
    for (const auto& child : elem->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        JS_DefinePropertyValueUint32(ctx, arr, index++,
            wrapElement(ctx, std::static_pointer_cast<dom::Element>(child).get()),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
      }
    }
    return arr;
  }

  static JSValue element_get_firstElementChild(JSContext* ctx, JSValueConst this_val,
                                                int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto child = elem->firstElementChild();
    return child ? wrapElement(ctx, child.get()) : JS_NULL;
  }

  static JSValue element_get_lastElementChild(JSContext* ctx, JSValueConst this_val,
                                               int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto child = elem->lastElementChild();
    return child ? wrapElement(ctx, child.get()) : JS_NULL;
  }

  static JSValue element_get_nextElementSibling(JSContext* ctx, JSValueConst this_val,
                                                  int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto sibling = elem->nextElementSibling();
    return sibling ? wrapElement(ctx, sibling.get()) : JS_NULL;
  }

  static JSValue element_get_previousElementSibling(JSContext* ctx, JSValueConst this_val,
                                                      int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto sibling = elem->previousElementSibling();
    return sibling ? wrapElement(ctx, sibling.get()) : JS_NULL;
  }

  static JSValue element_get_childElementCount(JSContext* ctx, JSValueConst this_val,
                                                int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewUint32(ctx, elem->childElementCount());
  }

  static JSValue element_get_ownerDocument(JSContext* ctx, JSValueConst this_val,
                                            int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    auto root = elem->getRootNode();
    if (root && root->nodeType() == dom::NodeType::Document) {
      return wrapDocument(ctx, std::dynamic_pointer_cast<dom::Document>(root));
    }
    return JS_NULL;
  }

  static JSValue element_get_nodeType(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JS_NewUint32(ctx, static_cast<uint32_t>(dom::NodeType::Element));
  }

  static JSValue element_get_nodeName(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, elem->tagName());
  }

  // === Dataset Methods ===
  static JSValue dataset_get(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_NULL;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string attrName = "data-" + name;
    auto value = elem->getAttribute(attrName);
    return value ? JSBinding::toJSString(ctx, *value) : JS_NULL;
  }

  static JSValue dataset_set(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 2) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string value = JSBinding::toStdString(ctx, argv[1]);
    std::string attrName = "data-" + name;
    elem->setAttribute(attrName, value);
    return JS_UNDEFINED;
  }

  static JSValue dataset_has(JSContext* ctx, JSValueConst this_val,
                            int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_FALSE;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string attrName = "data-" + name;
    return elem->hasAttribute(attrName) ? JS_TRUE : JS_FALSE;
  }

  static JSValue dataset_remove(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string attrName = "data-" + name;
    elem->removeAttribute(attrName);
    return JS_UNDEFINED;
  }

  // === Element Methods ===
  static JSValue element_setAttribute(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 2) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string value = JSBinding::toStdString(ctx, argv[1]);
    elem->setAttribute(name, value);
    return JS_UNDEFINED;
  }

  static JSValue element_getAttribute(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    return detail::OptionalToJSString(ctx, elem->getAttribute(name));
  }

  static JSValue element_hasAttribute(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    return elem->hasAttribute(name) ? JS_TRUE : JS_FALSE;
  }

  static JSValue element_removeAttribute(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    elem->removeAttribute(name);
    return JS_UNDEFINED;
  }

  static JSValue element_getElementsByTagName(JSContext* ctx, JSValueConst this_val,
                                               int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    auto elements = elem->getElementsByTagName(name);
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  static JSValue element_getElementsByClassName(JSContext* ctx, JSValueConst this_val,
                                                 int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string className = JSBinding::toStdString(ctx, argv[0]);
    auto elements = elem->getElementsByClassName(className);
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  static JSValue element_querySelector(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    if (selector[0] == '#' && selector.length() > 1) {
      auto found = elem->getElementById(selector.substr(1));
      if (found) return wrapElement(ctx, found.get());
    } else {
      auto elements = elem->getElementsByTagName(selector);
      if (!elements.empty()) return wrapElement(ctx, elements[0].get());
    }
    return JS_NULL;
  }

  static JSValue element_querySelectorAll(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string selector = JSBinding::toStdString(ctx, argv[0]);
    std::vector<dom::ElementPtr> elements;

    if (selector[0] == '.' && selector.length() > 1) {
      elements = elem->getElementsByClassName(selector.substr(1));
    } else if (selector[0] == '#' && selector.length() > 1) {
      auto found = elem->getElementById(selector.substr(1));
      if (found) elements.push_back(found);
    } else {
      elements = elem->getElementsByTagName(selector);
    }

    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < elements.size(); ++i) {
      JS_DefinePropertyValueUint32(ctx, arr, i, wrapElement(ctx, elements[i].get()),
          JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
  }

  // === ClassList Methods ===
  static JSValue classList_add(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    for (int i = 0; i < argc; i++) {
      elem->addClass(JSBinding::toStdString(ctx, argv[i]));
    }
    return JS_UNDEFINED;
  }

  static JSValue classList_remove(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    for (int i = 0; i < argc; i++) {
      elem->removeClass(JSBinding::toStdString(ctx, argv[i]));
    }
    return JS_UNDEFINED;
  }

  static JSValue classList_toggle(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string className = JSBinding::toStdString(ctx, argv[0]);
    bool force = (argc > 1) ? JS_ToBool(ctx, argv[1]) : !elem->hasClass(className);
    if (force) {
      elem->addClass(className);
      return JS_TRUE;
    } else {
      elem->removeClass(className);
      return JS_FALSE;
    }
  }

  static JSValue classList_contains(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string className = JSBinding::toStdString(ctx, argv[0]);
    return elem->hasClass(className) ? JS_TRUE : JS_FALSE;
  }

  // === Node Manipulation Methods ===
  static JSValue element_appendChild(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* parent = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!parent || argc < 1) return JS_EXCEPTION;
    auto* child = detail::GetElementFromThis(ctx, argv[0], s_elementClassId);
    if (!child) return JS_EXCEPTION;
    try {
      parent->appendChild(child->shared_from_this());
    } catch (...) {
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[0]);
  }

  static JSValue element_removeChild(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* parent = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!parent || argc < 1) return JS_EXCEPTION;
    auto* child = detail::GetElementFromThis(ctx, argv[0], s_elementClassId);
    if (!child) return JS_EXCEPTION;
    try {
      parent->removeChild(child->shared_from_this());
    } catch (...) {
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[0]);
  }

  static JSValue element_insertBefore(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    auto* parent = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!parent || argc < 2) return JS_EXCEPTION;
    auto* newNode = detail::GetElementFromThis(ctx, argv[0], s_elementClassId);
    auto* refNode = detail::GetElementFromThis(ctx, argv[1], s_elementClassId);
    if (!newNode) return JS_EXCEPTION;
    try {
      dom::NodePtr refNodePtr;
      if (refNode) refNodePtr = refNode->shared_from_this();
      parent->insertBefore(newNode->shared_from_this(), refNodePtr);
    } catch (...) {
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[0]);
  }

  static JSValue element_replaceChild(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    auto* parent = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!parent || argc < 2) return JS_EXCEPTION;
    auto* newNode = detail::GetElementFromThis(ctx, argv[0], s_elementClassId);
    auto* oldNode = detail::GetElementFromThis(ctx, argv[1], s_elementClassId);
    if (!newNode || !oldNode) return JS_EXCEPTION;
    try {
      parent->replaceChild(newNode->shared_from_this(), oldNode->shared_from_this());
    } catch (...) {
      return JS_EXCEPTION;
    }
    return JS_DupValue(ctx, argv[1]);
  }

  static JSValue element_cloneNode(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    bool deep = (argc > 0) ? JS_ToBool(ctx, argv[0]) : false;
    auto cloned = elem->cloneNode(deep);
    if (cloned && cloned->nodeType() == dom::NodeType::Element) {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_createdElements.push_back(std::static_pointer_cast<dom::Element>(cloned));
      return wrapElement(ctx, std::static_pointer_cast<dom::Element>(cloned).get());
    }
    return JS_NULL;
  }

  static JSValue element_hasChildNodes(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    return elem->hasChildNodes() ? JS_TRUE : JS_FALSE;
  }

  static JSValue element_normalize(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    elem->normalize();
    return JS_UNDEFINED;
  }

  static JSValue element_contains(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_FALSE;
    auto* other = detail::GetElementFromThis(ctx, argv[0], s_elementClassId);
    if (!other) return JS_FALSE;
    try {
      return elem->contains(other->shared_from_this()) ? JS_TRUE : JS_FALSE;
    } catch (...) {
      return JS_FALSE;
    }
  }

  // === Query Methods ===
  static JSValue element_matches(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string selector = JSBinding::toStdString(ctx, argv[0]);
    return elem->matches(selector) ? JS_TRUE : JS_FALSE;
  }

  static JSValue element_closest(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem || argc < 1) return JS_EXCEPTION;
    std::string selector = JSBinding::toStdString(ctx, argv[0]);

    dom::Element* current = elem;
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

  // === NEW: Element Action Methods ===
  static JSValue element_scroll(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    // Simplified: no-op for now
    return JS_UNDEFINED;
  }

  static JSValue element_scrollTo(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
  }

  static JSValue element_scrollBy(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
  }

  static JSValue element_focus(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
  }

  static JSValue element_blur(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_UNDEFINED;
  }

  static JSValue element_click(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv) {
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;
    // Dispatch a click event
    auto it = elem->eventListenerIds_.find("click");
    if (it != elem->eventListenerIds_.end() && !it->second.empty()) {
      JSContext* jsCtx = nullptr; // Would need context passed somehow
      // Simplified: just return success
    }
    return JS_UNDEFINED;
  }

  static JSValue element_getBoundingClientRect(JSContext* ctx, JSValueConst this_val,
                                                int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    auto* elem = detail::GetElementFromThis(ctx, this_val, s_elementClassId);
    if (!elem) return JS_EXCEPTION;

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

  // === Event Methods ===
  static JSValue element_addEventListener(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    dom::Node* node = getNodeFromThis(ctx, this_val);
    if (!node || argc < 2) return JS_EXCEPTION;
    if (!JS_IsFunction(ctx, argv[1])) return JS_EXCEPTION;
    std::string type = JSBinding::toStdString(ctx, argv[0]);
    uint32_t id = EventBinding::addListener(ctx, argv[1]);
    node->eventListenerIds_[type].push_back(id);
    return JS_UNDEFINED;
  }

  static JSValue element_removeEventListener(JSContext* ctx, JSValueConst this_val,
                                              int argc, JSValueConst* argv) {
    dom::Node* node = getNodeFromThis(ctx, this_val);
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

  static JSValue element_dispatchEvent(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    dom::Node* node = getNodeFromThis(ctx, this_val);
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

  // === DocumentFragment Methods ===
  static JSValue fragment_appendChild(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_NULL; // Simplified
  }

  static JSValue fragment_get_childNodes(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    return JS_NewArray(ctx); // Simplified: empty array
  }

  static JSValue fragment_get_children(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)argc; (void)argv;
    return JS_NewArray(ctx); // Simplified: empty array
  }
};

// Static definitions
inline JSClassID DOMBinding::s_elementClassId = 0;
inline JSClassID DOMBinding::s_documentClassId = 0;

} // namespace script
} // namespace xiaopeng
