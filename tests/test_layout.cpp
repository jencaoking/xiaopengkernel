
#include "test_framework.hpp"
#include <dom/dom.hpp>
#include <iostream>
#include <layout/layout_algorithm.hpp>
#include <layout/layout_tree_builder.hpp>


using namespace xiaopeng::layout;
using namespace xiaopeng::dom;
using namespace xiaopeng::css;

// Helper to create a basic layout box
LayoutBoxPtr createBlock(float width = 0) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  if (width > 0) {
    style.width = Length::Px(width);
  }
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr createText(const std::string &content) {
  auto node = std::make_shared<TextNode>(content);
  ComputedStyle style;
  style.display = Display::Inline;
  return std::make_shared<LayoutBox>(BoxType::InlineNode, style, node);
}

TEST(Layout_BlockWidthCalculation) {
  auto root = createBlock();
  Dimensions viewport;
  viewport.content.width = 800;

  LayoutAlgorithm algo;
  algo.layout(root, viewport);

  // Should inherit width (simplified behavior)
  EXPECT_EQ(root->dimensions().content.width, 800);
}

TEST(Layout_PercentageWidth) {
  auto root = createBlock(0); // Auto width

  auto child = createBlock(0);

  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Percent(50);
  auto childBox = std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);

  root->addChild(childBox);

  Dimensions viewport;
  viewport.content.width = 800;

  LayoutAlgorithm algo;
  algo.layout(root, viewport);

  EXPECT_EQ(childBox->dimensions().content.width, 400);
}

TEST(Layout_TextWrapping) {
  auto root = createBlock(100); // 100px width

  // "Hello World"
  // Char width is approx 8px.
  // "Hello" = 5 * 8 = 40px
  // Space = 5px
  // "World" = 5 * 8 = 40px
  // Total = 40+5+40 = 85px < 100px. Should fit on one line.

  auto text = createText("Hello World");
  root->addChild(text);

  Dimensions viewport;
  viewport.content.width = 1000; // Large viewport, but root is restricted

  LayoutAlgorithm algo;
  algo.layout(root, viewport);

  EXPECT_EQ(root->lineBoxes().size(), 1);
}

TEST(Layout_TextWrapping_ForceWrap) {
  auto root = createBlock(60); // 60px width

  // "Hello World"
  // "Hello" (40px) fits.
  // Space (5px).
  // "World" (40px). 40 + 5 + 40 = 85px > 60px.
  // Should wrap.

  auto text = createText("Hello World");
  root->addChild(text);

  Dimensions viewport;
  viewport.content.width = 1000;

  LayoutAlgorithm algo;
  algo.layout(root, viewport);

  EXPECT_EQ(root->lineBoxes().size(), 2);
  // Height should be roughly 2 lines * 16px = 32px (at least > 20px)
  EXPECT_GE(root->dimensions().content.height, 30);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return xiaopeng::test::runTests();
}
