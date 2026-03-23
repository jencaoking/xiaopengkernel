#include "test_framework.hpp"
#include <css/css_parser.hpp>
#include <css/style_resolver.hpp>
#include <dom/html_tree_builder.hpp>

using namespace xiaopeng::css;
using namespace xiaopeng::dom;

TEST(StyleResolver_SimpleTag) {
  HtmlTreeBuilder builder;
  auto doc = builder.build("<div></div>");
  auto div = doc->getElementsByTagName("div")[0];

  StyleSheet sheet;
  Rule rule;
  Selector sel;
  sel.parts.push_back({SelectorType::Tag, "div", "", "", ""});
  rule.selectors.push_back(sel);
  rule.declarations.push_back({"color", "red", false});
  sheet.rules.push_back(rule);

  StyleResolver resolver;
  auto style =
      resolver.resolveStyle(std::static_pointer_cast<Element>(div), sheet);

  EXPECT_EQ(style.color.r, 255); // Red
  EXPECT_EQ(style.color.g, 0);
}

TEST(StyleResolver_Class) {
  HtmlTreeBuilder builder;
  auto doc = builder.build("<div class='test'></div>");
  auto div = doc->getElementsByTagName("div")[0];

  StyleSheet sheet;
  Rule rule;
  Selector sel;
  sel.parts.push_back({SelectorType::Class, "test", "", "", ""});
  rule.selectors.push_back(sel);
  rule.declarations.push_back({"width", "100px", false});
  sheet.rules.push_back(rule);

  StyleResolver resolver;
  auto style =
      resolver.resolveStyle(std::static_pointer_cast<Element>(div), sheet);

  EXPECT_EQ((int)style.width.value, 100);
  EXPECT_TRUE(style.width.unit == Length::Unit::Px);
}

TEST(StyleResolver_Descendant) {
  HtmlTreeBuilder builder;
  auto doc = builder.build("<div id='p'><span id='c'></span></div>");
  auto span = doc->getElementById("c");

  StyleSheet sheet;
  Rule rule;
  Selector sel;
  // div span
  sel.parts.push_back({SelectorType::Tag, "div", "", "", ""});
  sel.combinators.push_back(Combinator::Descendant);
  sel.parts.push_back({SelectorType::Tag, "span", "", "", ""});

  rule.selectors.push_back(sel);
  rule.declarations.push_back({"display", "block", false});
  sheet.rules.push_back(rule);

  StyleResolver resolver;
  auto style =
      resolver.resolveStyle(std::static_pointer_cast<Element>(span), sheet);

  EXPECT_TRUE(style.display == Display::Block);
}

TEST(StyleResolver_Specificity) {
  HtmlTreeBuilder builder;
  auto doc = builder.build("<div id='myid' class='myclass'></div>");
  auto div = doc->getElementById("myid");

  StyleSheet sheet;

  // Rule 1: .myclass { color: blue }
  Rule r1;
  Selector s1;
  s1.parts.push_back({SelectorType::Class, "myclass", "", "", ""});
  r1.selectors.push_back(s1);
  r1.declarations.push_back({"color", "blue", false});
  sheet.rules.push_back(r1);

  // Rule 2: #myid { color: red }
  Rule r2;
  Selector s2;
  s2.parts.push_back({SelectorType::Id, "myid", "", "", ""});
  r2.selectors.push_back(s2);
  r2.declarations.push_back({"color", "red", false});
  sheet.rules.push_back(r2);

  StyleResolver resolver;
  auto style =
      resolver.resolveStyle(std::static_pointer_cast<Element>(div), sheet);

  // ID should win over Class
  EXPECT_EQ(style.color.r, 255); // Red
  EXPECT_EQ(style.color.b, 0);
}

TEST(StyleResolver_Order) {
  HtmlTreeBuilder builder;
  auto doc = builder.build("<div></div>");
  auto div = doc->getElementsByTagName("div")[0];

  StyleSheet sheet;

  // Rule 1: div { color: red }
  Rule r1;
  Selector s1;
  s1.parts.push_back({SelectorType::Tag, "div", "", "", ""});
  r1.selectors.push_back(s1);
  r1.declarations.push_back({"color", "red", false});
  sheet.rules.push_back(r1);

  // Rule 2: div { color: blue }
  Rule r2;
  Selector s2;
  s2.parts.push_back({SelectorType::Tag, "div", "", "", ""});
  r2.selectors.push_back(s2);
  r2.declarations.push_back({"color", "blue", false});
  sheet.rules.push_back(r2);

  StyleResolver resolver;
  auto style =
      resolver.resolveStyle(std::static_pointer_cast<Element>(div), sheet);

  // Last rule should win
  EXPECT_EQ(style.color.b, 255); // Blue
  EXPECT_EQ(style.color.r, 0);
}
