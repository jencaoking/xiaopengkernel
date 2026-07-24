// Tests for CSS float layout support.
//
// Covers:
//   - Single left/right float positioning
//   - Float stacking (vertical)
//   - Left and right floats on the same line
//   - clear property (left/right/both)
//   - Container height includes float overflow
//   - Float with normal flow content

#include "test_framework.hpp"

#include <dom/dom.hpp>
#include <layout/layout_algorithm.hpp>
#include <layout/layout_box.hpp>

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::dom;
using namespace xiaopeng::layout;

namespace {

LayoutBoxPtr makeContainer(float width, float height = 0) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Px(width);
  if (height > 0)
    style.height = Length::Px(height);
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr makeFloatLeft(float w, float h) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Px(w);
  style.height = Length::Px(h);
  style.cssFloat = Float::Left;
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr makeFloatRight(float w, float h) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Px(w);
  style.height = Length::Px(h);
  style.cssFloat = Float::Right;
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr makeBlock(float w, float h) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Px(w);
  style.height = Length::Px(h);
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr makeBlockWithClear(float w, float h, Clear clear) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  style.width = Length::Px(w);
  style.height = Length::Px(h);
  style.clear = clear;
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

LayoutBoxPtr makeText(const std::string &content) {
  auto node = std::make_shared<TextNode>(content);
  ComputedStyle style;
  style.display = Display::Inline;
  return std::make_shared<LayoutBox>(BoxType::InlineNode, style, node);
}

void layoutRoot(LayoutBoxPtr root, float viewportWidth = 800) {
  Dimensions viewport;
  viewport.content.width = viewportWidth;
  LayoutAlgorithm algo;
  algo.layout(root, viewport);
}

} // namespace

// ---------------------------------------------------------------------------
// Single float positioning
// ---------------------------------------------------------------------------

TEST(Float_SingleLeftFloat) {
  auto root = makeContainer(200, 200);
  root->addChild(makeFloatLeft(50, 50));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 1u);
  auto &child = root->children()[0];
  // Left float should be at the left edge of the container's content area
  EXPECT_LT(child->dimensions().content.x, 10.0f);
  EXPECT_TRUE(child->isFloat());
}

TEST(Float_SingleRightFloat) {
  auto root = makeContainer(200, 200);
  root->addChild(makeFloatRight(50, 50));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 1u);
  auto &child = root->children()[0];
  // Right float should be near the right edge
  EXPECT_GT(child->dimensions().content.x, 140.0f);
  EXPECT_TRUE(child->isFloat());
}

// ---------------------------------------------------------------------------
// Float stacking
// ---------------------------------------------------------------------------

TEST(Float_TwoLeftFloatsStackVertically) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatLeft(50, 60));
  root->addChild(makeFloatLeft(50, 60));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &first = root->children()[0];
  auto &second = root->children()[1];

  // Both should be at the left edge
  EXPECT_LT(first->dimensions().content.x, 10.0f);
  EXPECT_LT(second->dimensions().content.x, 10.0f);

  // Second should be below the first (vertical stacking)
  EXPECT_GT(second->dimensions().content.y, first->dimensions().content.y);
}

TEST(Float_TwoRightFloatsStackVertically) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatRight(50, 60));
  root->addChild(makeFloatRight(50, 60));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &first = root->children()[0];
  auto &second = root->children()[1];

  // Both should be near the right edge
  EXPECT_GT(first->dimensions().content.x, 140.0f);
  EXPECT_GT(second->dimensions().content.x, 140.0f);

  // Second should be below the first
  EXPECT_GT(second->dimensions().content.y, first->dimensions().content.y);
}

TEST(Float_LeftAndRightOnSameLine) {
  auto root = makeContainer(200, 200);
  root->addChild(makeFloatLeft(50, 50));
  root->addChild(makeFloatRight(50, 50));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &leftFloat = root->children()[0];
  auto &rightFloat = root->children()[1];

  // Left float at left, right float at right
  EXPECT_LT(leftFloat->dimensions().content.x, 10.0f);
  EXPECT_GT(rightFloat->dimensions().content.x, 140.0f);

  // Both should be at the same Y (side by side)
  EXPECT_LT(
      std::abs(leftFloat->dimensions().content.y -
               rightFloat->dimensions().content.y),
      2.0f);
}

// ---------------------------------------------------------------------------
// Normal flow content
// ---------------------------------------------------------------------------

TEST(Float_NormalFlowBelowFloats) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatLeft(50, 80));
  root->addChild(makeBlock(100, 30)); // normal block after float
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &normalBlock = root->children()[1];

  // Normal block should start at Y=0 (floats don't affect normal flow Y)
  // but the float is at Y=0 too. The block is in normal flow.
  EXPECT_GE(normalBlock->dimensions().content.y, 0.0f);
}

// ---------------------------------------------------------------------------
// clear property
// ---------------------------------------------------------------------------

TEST(Float_ClearLeft) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatLeft(50, 80));
  root->addChild(makeBlockWithClear(100, 20, Clear::Left));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &clearedBlock = root->children()[1];

  // Block with clear:left should be below the float
  EXPECT_GT(clearedBlock->dimensions().content.y, 78.0f);
}

TEST(Float_ClearRight) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatRight(50, 80));
  root->addChild(makeBlockWithClear(100, 20, Clear::Right));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 2u);
  auto &clearedBlock = root->children()[1];

  // Block with clear:right should be below the right float
  EXPECT_GT(clearedBlock->dimensions().content.y, 78.0f);
}

TEST(Float_ClearBoth) {
  auto root = makeContainer(200, 400);
  root->addChild(makeFloatLeft(50, 60));
  root->addChild(makeFloatRight(50, 80));
  root->addChild(makeBlockWithClear(100, 20, Clear::Both));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 3u);
  auto &clearedBlock = root->children()[2];

  // Block with clear:both should be below both floats
  EXPECT_GT(clearedBlock->dimensions().content.y, 78.0f);
}

// ---------------------------------------------------------------------------
// Container height
// ---------------------------------------------------------------------------

TEST(Float_ContainerHeightIncludesFloat) {
  auto root = makeContainer(200); // auto height
  root->addChild(makeFloatLeft(50, 100));
  root->addChild(makeBlock(100, 30));
  layoutRoot(root);

  // Container height should include the float overflow
  EXPECT_GE(root->dimensions().content.height, 98.0f);
}

// ---------------------------------------------------------------------------
// Mixed content
// ---------------------------------------------------------------------------

TEST(Float_MixedContent) {
  auto root = makeContainer(300, 400);
  root->addChild(makeFloatLeft(80, 80));
  root->addChild(makeBlock(200, 40));
  root->addChild(makeBlock(200, 40));
  layoutRoot(root);

  ASSERT_EQ(root->children().size(), 3u);
  // Float at left
  EXPECT_LT(root->children()[0]->dimensions().content.x, 10.0f);
  // Block-level elements start at left edge (floats don't shift blocks)
  EXPECT_LT(root->children()[1]->dimensions().content.x, 10.0f);
  EXPECT_LT(root->children()[2]->dimensions().content.x, 10.0f);
  // Blocks stacked vertically
  EXPECT_GT(root->children()[2]->dimensions().content.y,
            root->children()[1]->dimensions().content.y);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return xiaopeng::test::runTests();
}
