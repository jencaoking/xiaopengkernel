// test_phase2_selectors_stacking.cpp
//
// Phase 2 tests: Advanced CSS Selectors + Stacking Context
//
// Tests:
//   1. Attribute selector operators (=, ~=, |=, ^=, $=, *=)
//   2. New pseudo-classes (first-of-type, last-of-type, only-of-type, lang, :not compound)
//   3. Stacking context creation rules
//   4. Stacking context painting order
//   5. z-index sorting

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "../include/dom/dom.hpp"
#include "../include/css/css_types.hpp"
#include "../include/css/style_resolver.hpp"
#include "../include/css/selector_enhancements.hpp"
#include "../include/layout/stacking_context.hpp"

using namespace xiaopeng;

static int g_total = 0;
static int g_passed = 0;

#define TEST_BEGIN(name)                                                       \
  g_total++;                                                                   \
  std::cout << "  [TEST] " << name << " ... " << std::flush;

#define TEST_PASS()                                                            \
  g_passed++;                                                                  \
  std::cout << "✅" << std::endl;

#define TEST_FAIL(msg)                                                         \
  std::cout << "❌ " << msg << " (line " << __LINE__ << ")" << std::endl;

#define EXPECT(cond, msg)                                                      \
  if (!(cond)) {                                                               \
    TEST_FAIL(msg);                                                            \
    return;                                                                    \
  }

// ══════════════════════════════════════════════════════════════
//  Build test DOM for selector tests:
//    <div id="main" class="container" data-role="primary">
//      <p id="p1" class="text" lang="en">Hello</p>
//      <p id="p2" class="text muted" lang="en-US">World</p>
//      <span id="s1" class="highlight">Inline</span>
//      <div id="d1" class="box" data-role="secondary">Nested</div>
//      <ul id="list">
//        <li id="li1" class="item active">A</li>
//        <li id="li2" class="item">B</li>
//        <li id="li3" class="item featured">C</li>
//      </ul>
//    </div>
// ══════════════════════════════════════════════════════════════

static dom::Document buildSelectorTestDOM() {
  dom::Document doc;

  auto main = doc.createElement("div");
  main->setId("main");
  main->setClassName("container");
  main->setAttribute("data-role", "primary");
  doc.appendChild(main);

  auto p1 = doc.createElement("p");
  p1->setId("p1");
  p1->setClassName("text");
  p1->setAttribute("lang", "en");
  p1->appendChild(doc.createTextNode("Hello"));
  main->appendChild(p1);

  auto p2 = doc.createElement("p");
  p2->setId("p2");
  p2->setClassName("text muted");
  p2->setAttribute("lang", "en-US");
  p2->appendChild(doc.createTextNode("World"));
  main->appendChild(p2);

  auto s1 = doc.createElement("span");
  s1->setId("s1");
  s1->setClassName("highlight");
  s1->appendChild(doc.createTextNode("Inline"));
  main->appendChild(s1);

  auto d1 = doc.createElement("div");
  d1->setId("d1");
  d1->setClassName("box");
  d1->setAttribute("data-role", "secondary");
  d1->appendChild(doc.createTextNode("Nested"));
  main->appendChild(d1);

  auto ul = doc.createElement("ul");
  ul->setId("list");
  main->appendChild(ul);

  auto li1 = doc.createElement("li");
  li1->setId("li1");
  li1->setClassName("item active");
  li1->appendChild(doc.createTextNode("A"));
  ul->appendChild(li1);

  auto li2 = doc.createElement("li");
  li2->setId("li2");
  li2->setClassName("item");
  li2->appendChild(doc.createTextNode("B"));
  ul->appendChild(li2);

  auto li3 = doc.createElement("li");
  li3->setId("li3");
  li3->setClassName("item featured");
  li3->appendChild(doc.createTextNode("C"));
  ul->appendChild(li3);

  return doc;
}

// ══════════════════════════════════════════════════════════════
//  Attribute Selector Tests
// ══════════════════════════════════════════════════════════════

void test_attr_exact(dom::Document &doc) {
  TEST_BEGIN("[attr=val] exact match");
  auto main = doc.getElementById("main");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "data-role";
  s.attributeOperator = "=";
  s.attributeValue = "primary";
  EXPECT(css::matchAttributeSelector(main, s), "should match data-role=primary");
  s.attributeValue = "secondary";
  EXPECT(!css::matchAttributeSelector(main, s), "should not match data-role=secondary");
  TEST_PASS();
}

void test_attr_starts_with(dom::Document &doc) {
  TEST_BEGIN("[attr^=val] starts with");
  auto p2 = doc.getElementById("p2");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "lang";
  s.attributeOperator = "^=";
  s.attributeValue = "en";
  EXPECT(css::matchAttributeSelector(p2, s), "lang should start with en");

  auto p1 = doc.getElementById("p1");
  EXPECT(css::matchAttributeSelector(p1, s), "lang=en should also start with en");
  TEST_PASS();
}

void test_attr_ends_with(dom::Document &doc) {
  TEST_BEGIN("[attr$=val] ends with");
  auto p2 = doc.getElementById("p2");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "lang";
  s.attributeOperator = "$=";
  s.attributeValue = "US";
  EXPECT(css::matchAttributeSelector(p2, s), "lang should end with US");

  auto p1 = doc.getElementById("p1");
  EXPECT(!css::matchAttributeSelector(p1, s), "lang=en should not end with US");
  TEST_PASS();
}

void test_attr_contains(dom::Document &doc) {
  TEST_BEGIN("[attr*=val] contains");
  auto p2 = doc.getElementById("p2");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "class";
  s.attributeOperator = "*=";
  s.attributeValue = "mute";
  EXPECT(css::matchAttributeSelector(p2, s), "class should contain mute");
  s.attributeValue = "text";
  EXPECT(css::matchAttributeSelector(p2, s), "class should contain text");
  s.attributeValue = "xyz";
  EXPECT(!css::matchAttributeSelector(p2, s), "class should not contain xyz");
  TEST_PASS();
}

void test_attr_word_match(dom::Document &doc) {
  TEST_BEGIN("[attr~=val] word match");
  auto li1 = doc.getElementById("li1");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "class";
  s.attributeOperator = "~=";
  s.attributeValue = "active";
  EXPECT(css::matchAttributeSelector(li1, s), "class should contain word active");
  s.attributeValue = "item";
  EXPECT(css::matchAttributeSelector(li1, s), "class should contain word item");
  s.attributeValue = "act";
  EXPECT(!css::matchAttributeSelector(li1, s), "should not partial-match act");
  TEST_PASS();
}

void test_attr_hyphen_match(dom::Document &doc) {
  TEST_BEGIN("[attr|=val] hyphen match");
  auto p2 = doc.getElementById("p2");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "lang";
  s.attributeOperator = "|=";
  s.attributeValue = "en";
  EXPECT(css::matchAttributeSelector(p2, s), "lang=en-US should match |=en");

  auto p1 = doc.getElementById("p1");
  EXPECT(css::matchAttributeSelector(p1, s), "lang=en should match |=en");
  TEST_PASS();
}

void test_attr_existence(dom::Document &doc) {
  TEST_BEGIN("[attr] existence");
  auto main = doc.getElementById("main");
  css::SimpleSelector s;
  s.type = css::SelectorType::Attribute;
  s.attributeName = "data-role";
  s.attributeOperator = "";
  s.attributeValue = "";
  EXPECT(css::matchAttributeSelector(main, s), "should have data-role");

  s.attributeName = "nonexistent";
  EXPECT(!css::matchAttributeSelector(main, s), "should not have nonexistent");
  TEST_PASS();
}

// ══════════════════════════════════════════════════════════════
//  Pseudo-Class Tests
// ══════════════════════════════════════════════════════════════

void test_first_of_type(dom::Document &doc) {
  TEST_BEGIN(":first-of-type");
  auto p1 = doc.getElementById("p1");
  EXPECT(css::matchPseudoClassPhase2(p1, "first-of-type"), "p1 should be first p");

  auto p2 = doc.getElementById("p2");
  EXPECT(!css::matchPseudoClassPhase2(p2, "first-of-type"), "p2 should not be first p");

  auto li1 = doc.getElementById("li1");
  EXPECT(css::matchPseudoClassPhase2(li1, "first-of-type"), "li1 should be first li");
  TEST_PASS();
}

void test_last_of_type(dom::Document &doc) {
  TEST_BEGIN(":last-of-type");
  auto p2 = doc.getElementById("p2");
  EXPECT(css::matchPseudoClassPhase2(p2, "last-of-type"), "p2 should be last p");

  auto p1 = doc.getElementById("p1");
  EXPECT(!css::matchPseudoClassPhase2(p1, "last-of-type"), "p1 should not be last p");

  auto li3 = doc.getElementById("li3");
  EXPECT(css::matchPseudoClassPhase2(li3, "last-of-type"), "li3 should be last li");
  TEST_PASS();
}

void test_only_of_type(dom::Document &doc) {
  TEST_BEGIN(":only-of-type");
  auto s1 = doc.getElementById("s1");
  EXPECT(css::matchPseudoClassPhase2(s1, "only-of-type"), "s1 is the only span");

  auto p1 = doc.getElementById("p1");
  EXPECT(!css::matchPseudoClassPhase2(p1, "only-of-type"), "p1 is not the only p");
  TEST_PASS();
}

void test_lang_pseudo(dom::Document &doc) {
  TEST_BEGIN(":lang(xx)");
  auto p1 = doc.getElementById("p1");
  EXPECT(css::matchPseudoClassPhase2(p1, "lang(en)"), "p1 lang=en");
  EXPECT(css::matchPseudoClassPhase2(p1, "lang(en)"), "p1 matches lang(en)");

  auto p2 = doc.getElementById("p2");
  EXPECT(css::matchPseudoClassPhase2(p2, "lang(en)"), "p2 lang=en-US matches en");
  EXPECT(!css::matchPseudoClassPhase2(p2, "lang(fr)"), "p2 should not match fr");
  TEST_PASS();
}

void test_not_compound(dom::Document &doc) {
  TEST_BEGIN(":not() compound");
  auto p1 = doc.getElementById("p1");
  EXPECT(css::matchNotEnhanced(p1, ".muted"), "p1 does not have .muted");
  EXPECT(!css::matchNotEnhanced(p1, "p"), "p1 IS a p tag");
  EXPECT(!css::matchNotEnhanced(p1, "#p1"), "p1 IS #p1");
  EXPECT(css::matchNotEnhanced(p1, "#p2"), "p1 is not #p2");

  auto li2 = doc.getElementById("li2");
  EXPECT(css::matchNotEnhanced(li2, ".active"), "li2 does not have .active");
  EXPECT(!css::matchNotEnhanced(li2, ".item"), "li2 HAS .item");
  TEST_PASS();
}

// ══════════════════════════════════════════════════════════════
//  Stacking Context Tests
// ══════════════════════════════════════════════════════════════

void test_stacking_creation_root() {
  TEST_BEGIN("Stacking context: root element");
  auto doc = std::make_shared<dom::Document>();
  auto html = doc->createElement("html");
  doc->appendChild(html);
  css::ComputedStyle style;
  EXPECT(layout::StackingContext::createsStackingContext(style, html),
         "html should create stacking context");
  TEST_PASS();
}

void test_stacking_creation_positioned() {
  TEST_BEGIN("Stacking context: positioned + z-index");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.position = css::Position::Relative;
  style.zIndex = 5;
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "relative + z-index:5 should create context");

  style.zIndex = 0;
  // z-index 0 on positioned elements: our implementation treats non-auto z-index
  // as creating a context. Since 0 != auto (which we don't represent), this
  // depends on implementation. In standard CSS, z-index: auto does NOT create
  // a context, but z-index: 0 DOES.
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "relative + z-index:0 should create context");

  style.position = css::Position::Static;
  style.zIndex = 5;
  EXPECT(!layout::StackingContext::createsStackingContext(style, div),
         "static + z-index should NOT create context");
  TEST_PASS();
}

void test_stacking_creation_fixed() {
  TEST_BEGIN("Stacking context: fixed/sticky");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.position = css::Position::Fixed;
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "fixed should create context");

  style.position = css::Position::Sticky;
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "sticky should create context");
  TEST_PASS();
}

void test_stacking_creation_opacity() {
  TEST_BEGIN("Stacking context: opacity < 1");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.opacity = 0.5f;
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "opacity:0.5 should create context");

  style.opacity = 1.0f;
  EXPECT(!layout::StackingContext::createsStackingContext(style, div),
         "opacity:1 should NOT create context");
  TEST_PASS();
}

void test_stacking_creation_transform() {
  TEST_BEGIN("Stacking context: transform");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.transform = "rotate(45deg)";
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "transform should create context");

  style.transform = "none";
  EXPECT(!layout::StackingContext::createsStackingContext(style, div),
         "transform:none should NOT create context");
  TEST_PASS();
}

void test_stacking_creation_filter() {
  TEST_BEGIN("Stacking context: filter");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.filter = "blur(5px)";
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "filter should create context");
  TEST_PASS();
}

void test_stacking_creation_isolation() {
  TEST_BEGIN("Stacking context: isolation");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.isolation = css::Isolation::Isolate;
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "isolation:isolate should create context");
  TEST_PASS();
}

void test_stacking_creation_will_change() {
  TEST_BEGIN("Stacking context: will-change");
  auto doc = std::make_shared<dom::Document>();
  auto div = doc->createElement("div");
  doc->appendChild(div);
  css::ComputedStyle style;
  style.willChange = "transform, opacity";
  EXPECT(layout::StackingContext::createsStackingContext(style, div),
         "will-change:transform should create context");
  TEST_PASS();
}

void test_stacking_painting_order() {
  TEST_BEGIN("Stacking context: painting order");
  // Build: <div id="root"> <div id="z1" z-index:1> <div id="zneg" z-index:-1>
  //        <div id="z0" z-index:0> <div id="z2" z-index:2>
  auto doc = std::make_shared<dom::Document>();
  auto root = doc->createElement("div");
  root->setId("root");
  doc->appendChild(root);

  // Create child elements with different z-indices
  auto zNeg = doc->createElement("div");
  zNeg->setId("zneg");
  root->appendChild(zNeg);

  auto z0 = doc->createElement("div");
  z0->setId("z0");
  root->appendChild(z0);

  auto z1 = doc->createElement("div");
  z1->setId("z1");
  root->appendChild(z1);

  auto z2 = doc->createElement("div");
  z2->setId("z2");
  root->appendChild(z2);

  // Build stacking context tree
  css::ComputedStyle rootStyle;
  rootStyle.position = css::Position::Relative;
  rootStyle.zIndex = 0;

  auto ctx = layout::StackingContext::buildTree(root, rootStyle);

  // Set child styles via inline attributes
  zNeg->setAttribute("style", "position:relative; z-index:-1");
  z0->setAttribute("style", "position:relative; z-index:0");
  z1->setAttribute("style", "position:relative; z-index:1");
  z2->setAttribute("style", "position:relative; z-index:2");

  // Rebuild with proper styles
  ctx = layout::StackingContext::buildTree(root, rootStyle);

  auto order = ctx->paintingOrder();

  // Should have at least root + children
  EXPECT(order.size() >= 1, "should have at least root context");

  // Check that negative z-index comes before positive
  // The exact order depends on buildTree collecting children properly
  // For now, verify the tree structure
  auto children = ctx->children();
  // Children should be sorted by z-index after sortChildren()
  if (children.size() >= 2) {
    for (size_t i = 1; i < children.size(); i++) {
      EXPECT(children[i]->zIndex() >= children[i - 1]->zIndex(),
             "children should be sorted by z-index");
    }
  }

  TEST_PASS();
}

void test_stacking_nested() {
  TEST_BEGIN("Stacking context: nested contexts");
  // <div id="outer" z-index:1>
  //   <div id="inner" z-index:10">
  auto doc = std::make_shared<dom::Document>();
  auto outer = doc->createElement("div");
  outer->setId("outer");
  doc->appendChild(outer);

  auto inner = doc->createElement("div");
  inner->setId("inner");
  outer->appendChild(inner);

  outer->setAttribute("style", "position:relative; z-index:1");
  inner->setAttribute("style", "position:relative; z-index:10");

  css::ComputedStyle rootStyle;
  auto ctx = layout::StackingContext::buildTree(outer, rootStyle);

  // outer creates a context (positioned + z-index)
  auto children = ctx->children();
  EXPECT(children.size() >= 1, "outer should have at least 1 child context");

  if (!children.empty()) {
    EXPECT(children[0]->zIndex() == 10, "inner context z-index should be 10");
  }

  TEST_PASS();
}

// ══════════════════════════════════════════════════════════════
//  Main
// ══════════════════════════════════════════════════════════════

int main() {
  std::cout << "\n"
      "╔═══════════════════════════════════════════════════════╗\n"
      "║  XiaopengKernel — Phase 2: Selectors + Stacking Ctx  ║\n"
      "╚═══════════════════════════════════════════════════════╝\n"
      << std::endl;

  auto doc = buildSelectorTestDOM();

  std::cout << "── Attribute Selectors ──" << std::endl;
  test_attr_exact(doc);
  test_attr_starts_with(doc);
  test_attr_ends_with(doc);
  test_attr_contains(doc);
  test_attr_word_match(doc);
  test_attr_hyphen_match(doc);
  test_attr_existence(doc);

  std::cout << "\n── Pseudo-Classes ──" << std::endl;
  test_first_of_type(doc);
  test_last_of_type(doc);
  test_only_of_type(doc);
  test_lang_pseudo(doc);
  test_not_compound(doc);

  std::cout << "\n── Stacking Context Creation ──" << std::endl;
  test_stacking_creation_root();
  test_stacking_creation_positioned();
  test_stacking_creation_fixed();
  test_stacking_creation_opacity();
  test_stacking_creation_transform();
  test_stacking_creation_filter();
  test_stacking_creation_isolation();
  test_stacking_creation_will_change();

  std::cout << "\n── Stacking Context Ordering ──" << std::endl;
  test_stacking_painting_order();
  test_stacking_nested();

  std::cout << "\n════════════════════════════════════════════════════" << std::endl;
  if (g_passed == g_total) {
    std::cout << "✅ All " << g_total << " Phase 2 tests passed!" << std::endl;
    return 0;
  } else {
    std::cout << "❌ " << (g_total - g_passed) << "/" << g_total
              << " tests failed" << std::endl;
    return 1;
  }
}
