#include "test_framework.hpp"
#include <css/css_parser.hpp>
#include <iostream>

using namespace xiaopeng::css;

TEST(CssTokenizer_SimpleRules) {
  std::string css = "body { color: red; }";
  CssTokenizer tokenizer(css);
  auto tokens = tokenizer.tokenize();
  EXPECT_FALSE(tokens.empty());
}

TEST(CssParser_SimpleRule) {
  std::string css = "div { width: 100px; height: 50%; }";
  CssParser parser(css);
  auto sheet = parser.parse();

  EXPECT_EQ(sheet.rules.size(), 1);
  if (!sheet.rules.empty()) {
    auto &rule = sheet.rules[0];
    EXPECT_EQ(rule.selectors.size(), 1);
    EXPECT_EQ(rule.declarations.size(), 2);

    EXPECT_EQ(rule.declarations[0].property, "width");
    // Value check: check if it contains expected parts
    EXPECT_TRUE(rule.declarations[0].value.find("100") != std::string::npos);
    EXPECT_TRUE(rule.declarations[0].value.find("px") != std::string::npos);

    EXPECT_EQ(rule.declarations[1].property, "height");
    EXPECT_TRUE(rule.declarations[1].value.find("50") != std::string::npos);
    EXPECT_TRUE(rule.declarations[1].value.find("%") != std::string::npos);
  }
}

TEST(CssParser_MultipleSelectors) {
  std::string css = "h1, h2, h3 { font-weight: bold; }";
  CssParser parser(css);
  auto sheet = parser.parse();

  EXPECT_EQ(sheet.rules.size(), 1);
  if (!sheet.rules.empty()) {
    EXPECT_EQ(sheet.rules[0].selectors.size(), 3);
  }
}

TEST(CssParser_ClassSelector) {
  std::string css = ".container { margin: 0; }";
  CssParser parser(css);
  auto sheet = parser.parse();

  EXPECT_EQ(sheet.rules.size(), 1);
  if (!sheet.rules.empty()) {
    auto &rule = sheet.rules[0];
    EXPECT_EQ(rule.selectors.size(), 1);
    if (!rule.selectors.empty()) {
      auto &sel = rule.selectors[0];
      EXPECT_EQ(sel.parts.size(), 1);
      EXPECT_EQ(sel.parts[0].type, SelectorType::Class);
      EXPECT_EQ(sel.parts[0].value, "container");
    }
  }
}

TEST(CssParser_IdSelector) {
  std::string css = "#main { padding: 10px; }";
  CssParser parser(css);
  auto sheet = parser.parse();

  EXPECT_EQ(sheet.rules.size(), 1);
  if (!sheet.rules.empty()) {
    auto &rule = sheet.rules[0];
    EXPECT_EQ(rule.selectors.size(), 1);
    if (!rule.selectors.empty()) {
      auto &sel = rule.selectors[0];
      EXPECT_EQ(sel.parts.size(), 1);
      EXPECT_EQ(sel.parts[0].type, SelectorType::Id);
      EXPECT_EQ(sel.parts[0].value, "main");
    }
  }
}

TEST(CssParser_ComplexSelector) {
  std::string css = "div.content > p { color: blue; }";
  CssParser parser(css);
  auto sheet = parser.parse();

  EXPECT_EQ(sheet.rules.size(), 1);
  if (!sheet.rules.empty()) {
    auto &rule = sheet.rules[0];
    EXPECT_EQ(rule.selectors.size(), 1);
    auto &sel = rule.selectors[0];

    // Expected: Tag(div) -> None -> Class(content) -> Child -> Tag(p)
    EXPECT_GE(sel.parts.size(), 3);
    EXPECT_EQ(sel.combinators.size(), sel.parts.size() - 1);

    EXPECT_EQ(sel.parts[0].type, SelectorType::Tag);
    EXPECT_EQ(sel.parts[0].value, "div");

    EXPECT_EQ(sel.combinators[0], Combinator::None);

    EXPECT_EQ(sel.parts[1].type, SelectorType::Class);
    EXPECT_EQ(sel.parts[1].value, "content");

    EXPECT_EQ(sel.combinators[1], Combinator::Child);

    EXPECT_EQ(sel.parts[2].type, SelectorType::Tag);
    EXPECT_EQ(sel.parts[2].value, "p");
  }
}
