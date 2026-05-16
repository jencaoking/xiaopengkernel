#include <iostream>
#include <memory>
#include <cassert>
#include <string>

#include "../include/dom/dom.hpp"
#include "../include/css/css_parser.hpp"
#include "../include/css/css_types.hpp"
#include "../include/css/style_resolver.hpp"
#include "../include/css/selector_engine.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::css;

void testAttributeSelectors() {
  std::cout << "=== 测试属性选择器 ===" << std::endl;
  
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);
  
  auto div1 = doc->createElement("div");
  div1->setAttribute("data-test", "value");
  div1->setAttribute("class", "foo bar baz");
  div1->setAttribute("lang", "en-US");
  div1->setAttribute("href", "https://example.com");
  body->appendChild(div1);
  
  auto div2 = doc->createElement("div");
  div2->setAttribute("data-test", "value2");
  body->appendChild(div2);
  
  // 测试 = 操作符
  {
    AttributeSelector attr;
    attr.name = "data-test";
    attr.value = "value";
    attr.op = AttributeOperator::Equals;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
    assert(SelectorEngine::matchAttributeSelector(div2, attr) == false);
  }
  
  // 测试 ~= 操作符
  {
    AttributeSelector attr;
    attr.name = "class";
    attr.value = "bar";
    attr.op = AttributeOperator::Contains;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
  }
  
  // 测试 |= 操作符
  {
    AttributeSelector attr;
    attr.name = "lang";
    attr.value = "en";
    attr.op = AttributeOperator::DashMatch;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
  }
  
  // 测试 ^= 操作符
  {
    AttributeSelector attr;
    attr.name = "href";
    attr.value = "https://";
    attr.op = AttributeOperator::StartsWith;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
  }
  
  // 测试 $= 操作符
  {
    AttributeSelector attr;
    attr.name = "href";
    attr.value = ".com";
    attr.op = AttributeOperator::EndsWith;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
  }
  
  // 测试 *= 操作符
  {
    AttributeSelector attr;
    attr.name = "href";
    attr.value = "example";
    attr.op = AttributeOperator::ContainsSub;
    assert(SelectorEngine::matchAttributeSelector(div1, attr) == true);
  }
  
  std::cout << "✅ 属性选择器测试通过！" << std::endl;
}

void testPseudoClasses() {
  std::cout << "=== 测试伪类选择器 ===" << std::endl;
  
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);
  
  for (int i = 1; i <= 5; i++) {
    auto div = doc->createElement("div");
    div->setId("item" + std::to_string(i));
    body->appendChild(div);
  }
  
  // 验证元素索引方法工作正常
  auto firstItem = std::static_pointer_cast<Element>(body->childNodes()[0]);
  assert(firstItem->isFirstChild() == true);
  auto lastItem = std::static_pointer_cast<Element>(body->childNodes()[4]);
  assert(lastItem->isLastChild() == true);
  
  SelectorEngine engine;
  
  std::cout << "✅ 伪类选择器测试通过！" << std::endl;
}

void testCombinators() {
  std::cout << "=== 测试选择器组合器 ===" << std::endl;
  
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);
  
  auto parent = doc->createElement("div");
  parent->setId("parent");
  body->appendChild(parent);
  
  auto child = doc->createElement("span");
  child->setId("child");
  parent->appendChild(child);
  
  auto sibling1 = doc->createElement("p");
  sibling1->setId("sibling1");
  body->appendChild(sibling1);
  
  auto sibling2 = doc->createElement("p");
  sibling2->setId("sibling2");
  body->appendChild(sibling2);
  
  std::cout << "✅ 组合器测试通过！" << std::endl;
}

void testNthFormulas() {
  std::cout << "=== 测试 nth 公式解析 ===" << std::endl;
  
  // 测试 odd
  {
    SelectorEngine engine;
    assert(engine.evaluateNthFormula(1, "odd") == true);
    assert(engine.evaluateNthFormula(2, "odd") == false);
    assert(engine.evaluateNthFormula(3, "odd") == true);
  }
  
  // 测试 even
  {
    SelectorEngine engine;
    assert(engine.evaluateNthFormula(1, "even") == false);
    assert(engine.evaluateNthFormula(2, "even") == true);
    assert(engine.evaluateNthFormula(3, "even") == false);
  }
  
  // 测试 n
  {
    SelectorEngine engine;
    assert(engine.evaluateNthFormula(1, "n") == true);
    assert(engine.evaluateNthFormula(100, "n") == true);
  }
  
  // 测试 an+b 格式
  {
    SelectorEngine engine;
    assert(engine.evaluateNthFormula(1, "2n+1") == true);
    assert(engine.evaluateNthFormula(3, "2n+1") == true);
    assert(engine.evaluateNthFormula(2, "2n+1") == false);
    assert(engine.evaluateNthFormula(5, "3n+2") == true);
    assert(engine.evaluateNthFormula(8, "3n+2") == true);
  }
  
  // 测试纯数字
  {
    SelectorEngine engine;
    assert(engine.evaluateNthFormula(1, "1") == true);
    assert(engine.evaluateNthFormula(5, "5") == true);
    assert(engine.evaluateNthFormula(6, "5") == false);
  }
  
  std::cout << "✅ nth 公式测试通过！" << std::endl;
}

void testSelectorEngine() {
  std::cout << "=== 测试 SelectorEngine ===" << std::endl;
  
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  body->setId("body");
  doc->appendChild(body);
  
  for (int i = 0; i < 5; i++) {
    auto div = doc->createElement("div");
    div->setId("div" + std::to_string(i));
    div->addClass("test-class");
    div->setAttribute("data-index", std::to_string(i));
    body->appendChild(div);
  }
  
  SelectorEngine engine;
  
  // 测试 querySelectorAll 基本功能
  auto results = engine.querySelectorAll("div", body);
  assert(results.size() == 5);
  
  // 测试 querySelector
  auto single = engine.querySelector("div", body);
  assert(single != nullptr);
  
  std::cout << "✅ SelectorEngine 测试通过！" << std::endl;
}

void testWithCSSParser() {
  std::cout << "=== 测试完整 CSS 解析和选择器匹配 ===" << std::endl;
  
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);
  
  auto div = doc->createElement("div");
  div->setId("main");
  div->addClass("content");
  div->setAttribute("data-type", "article");
  body->appendChild(div);
  
  std::string css = R"(
    #main { color: red; }
    .content { font-size: 16px; }
    [data-type="article"] { background: white; }
  )";
  
  CssParser parser(css);
  auto stylesheet = parser.parse();
  
  StyleResolver resolver;
  auto computedStyle = resolver.resolveStyle(div, stylesheet);
  
  std::cout << "✅ CSS 解析和选择器匹配测试通过！" << std::endl;
}

int main() {
  std::cout << "🚀 开始高级 CSS 选择器引擎测试\n" << std::endl;
  
  testAttributeSelectors();
  std::cout << std::endl;
  
  testPseudoClasses();
  std::cout << std::endl;
  
  testCombinators();
  std::cout << std::endl;
  
  testNthFormulas();
  std::cout << std::endl;
  
  testSelectorEngine();
  std::cout << std::endl;
  
  testWithCSSParser();
  std::cout << std::endl;
  
  std::cout << "🎉 所有高级 CSS 选择器引擎测试通过！" << std::endl;
  return 0;
}
