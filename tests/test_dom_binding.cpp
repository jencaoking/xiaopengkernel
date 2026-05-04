#include <gtest/gtest.h>
#include <dom/dom.hpp>
#include <script/dom_binding.hpp>
#include <script/js_binding.hpp>
#include <quickjs.h>

TEST(DOMBinding, WrapAndGetElementById) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.getElementById('test-div');
    elem !== null;
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, CreateElementAndAppendChild) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let div = document.createElement('div');
    div.id = 'new-div';
    let body = document.getElementsByTagName('body')[0];
    body.appendChild(div);
    let found = document.getElementById('new-div');
    found !== null;
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, TextContentProperty) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.getElementById('test-div');
    elem.textContent = 'Hello, DOM Binding!';
    elem.textContent === 'Hello, DOM Binding!';
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  EXPECT_EQ(div->textContent(), "Hello, DOM Binding!");

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, StyleProperty) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.getElementById('test-div');
    elem.style = 'color: red; background: blue;';
    elem.style.includes('color: red');
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  auto style = div->getAttribute("style");
  EXPECT_TRUE(style.has_value());
  EXPECT_TRUE(style->find("color: red") != std::string::npos);

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, InnerHTMLProperty) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.getElementById('test-div');
    elem.innerHTML = '<p>Hello <strong>World</strong>!</p>';
    elem.innerHTML.includes('Hello');
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  EXPECT_TRUE(div->innerHTML().find("Hello") != std::string::npos);

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, GetElementsByTagName) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div1 = doc->createElement("div");
  auto div2 = doc->createElement("div");
  auto div3 = doc->createElement("div");
  body->appendChild(div1);
  body->appendChild(div2);
  body->appendChild(div3);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let divs = document.getElementsByTagName('div');
    divs.length === 3;
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, QuerySelector) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("my-element");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.querySelector('#my-element');
    elem !== null && elem.tagName === 'div';
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, GetElementsByClassName) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div1 = doc->createElement("div");
  div1->setClassName("my-class");
  auto div2 = doc->createElement("div");
  div2->setClassName("my-class");
  body->appendChild(div1);
  body->appendChild(div2);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elems = document.getElementsByClassName('my-class');
    elems.length === 2;
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}

TEST(DOMBinding, SetAndGetAttribute) {
  auto doc = std::make_shared<xiaopeng::dom::Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  JSRuntime* rt = JS_NewRuntime();
  JSContext* ctx = JS_NewContext(rt);

  xiaopeng::script::DOMBinding::registerBinding(ctx);
  xiaopeng::script::JSBinding::registerBindings(ctx);

  JSValue docObj = xiaopeng::script::DOMBinding::wrapDocument(ctx, doc);
  JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);

  const char* code = R"(
    let elem = document.getElementById('test-div');
    elem.setAttribute('data-custom', 'hello');
    elem.getAttribute('data-custom') === 'hello' && elem.hasAttribute('data-custom');
  )";

  JSValue result = JS_Eval(ctx, code, strlen(code), "test.js", 0);
  EXPECT_TRUE(JS_IsBool(result));
  EXPECT_TRUE(JS_ToBool(ctx, result));

  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
}
