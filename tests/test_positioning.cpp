#include "test_framework.hpp"

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/layout_algorithm.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

// ── Tests ────────────────────────────────────────────────────

TEST(Positioning_Absolute_TopLeft) {
  ComputedStyle containerStyle;
  containerStyle.display = Display::Block;
  containerStyle.position = Position::Relative;
  containerStyle.width = Length::Px(600);
  containerStyle.height = Length::Px(400);
  auto container = std::make_shared<LayoutBox>(BoxType::BlockNode, containerStyle, nullptr);

  ComputedStyle absStyle;
  absStyle.display = Display::Block;
  absStyle.position = Position::Absolute;
  absStyle.top = Length::Px(20);
  absStyle.left = Length::Px(20);
  absStyle.width = Length::Px(100);
  absStyle.height = Length::Px(80);
  auto abs1 = std::make_shared<LayoutBox>(BoxType::BlockNode, absStyle, nullptr);
  container->addChild(abs1);

  LayoutAlgorithm algorithm;
  Dimensions parentDim;
  parentDim.content = {0, 0, 800, 600};
  algorithm.layout(container, parentDim);

  EXPECT_GE(abs1->dimensions().content.x, 20.0f);
  EXPECT_GE(abs1->dimensions().content.y, 20.0f);
  EXPECT_EQ(abs1->dimensions().content.width, 100.0f);
  EXPECT_EQ(abs1->dimensions().content.height, 80.0f);
}

TEST(Positioning_Absolute_BottomRight) {
  ComputedStyle containerStyle;
  containerStyle.display = Display::Block;
  containerStyle.position = Position::Relative;
  containerStyle.width = Length::Px(600);
  containerStyle.height = Length::Px(400);
  auto container = std::make_shared<LayoutBox>(BoxType::BlockNode, containerStyle, nullptr);

  ComputedStyle absStyle;
  absStyle.display = Display::Block;
  absStyle.position = Position::Absolute;
  absStyle.bottom = Length::Px(30);
  absStyle.right = Length::Px(30);
  absStyle.width = Length::Px(120);
  absStyle.height = Length::Px(60);
  auto abs2 = std::make_shared<LayoutBox>(BoxType::BlockNode, absStyle, nullptr);
  container->addChild(abs2);

  LayoutAlgorithm algorithm;
  Dimensions parentDim;
  parentDim.content = {0, 0, 800, 600};
  algorithm.layout(container, parentDim);

  EXPECT_EQ(abs2->dimensions().content.width, 120.0f);
  EXPECT_EQ(abs2->dimensions().content.height, 60.0f);
}

TEST(Positioning_Fixed) {
  ComputedStyle containerStyle;
  containerStyle.display = Display::Block;
  containerStyle.position = Position::Relative;
  containerStyle.width = Length::Px(400);
  containerStyle.height = Length::Px(300);
  auto container = std::make_shared<LayoutBox>(BoxType::BlockNode, containerStyle, nullptr);

  ComputedStyle fixedStyle;
  fixedStyle.display = Display::Block;
  fixedStyle.position = Position::Fixed;
  fixedStyle.top = Length::Px(50);
  fixedStyle.left = Length::Px(50);
  fixedStyle.width = Length::Px(200);
  fixedStyle.height = Length::Px(100);
  fixedStyle.zIndex = 10;
  auto fixed = std::make_shared<LayoutBox>(BoxType::BlockNode, fixedStyle, nullptr);
  container->addChild(fixed);

  LayoutAlgorithm algorithm;
  Dimensions parentDim;
  parentDim.content = {0, 0, 800, 600};
  algorithm.layout(container, parentDim);

  EXPECT_EQ(fixed->dimensions().content.width, 200.0f);
  EXPECT_EQ(fixed->dimensions().content.height, 100.0f);
}

TEST(Positioning_MixedLayout) {
  ComputedStyle rootStyle;
  rootStyle.display = Display::Block;
  rootStyle.width = Length::Px(800);
  rootStyle.height = Length::Px(600);
  auto root = std::make_shared<LayoutBox>(BoxType::BlockNode, rootStyle, nullptr);

  // Normal flow element
  ComputedStyle normalStyle;
  normalStyle.display = Display::Block;
  normalStyle.width = Length::Px(100);
  normalStyle.height = Length::Px(50);
  auto normal1 = std::make_shared<LayoutBox>(BoxType::BlockNode, normalStyle, nullptr);
  root->addChild(normal1);

  // Relative container with absolute child
  ComputedStyle relStyle;
  relStyle.display = Display::Block;
  relStyle.position = Position::Relative;
  relStyle.width = Length::Px(300);
  relStyle.height = Length::Px(200);
  auto rel = std::make_shared<LayoutBox>(BoxType::BlockNode, relStyle, nullptr);
  root->addChild(rel);

  ComputedStyle absChildStyle;
  absChildStyle.display = Display::Block;
  absChildStyle.position = Position::Absolute;
  absChildStyle.top = Length::Px(10);
  absChildStyle.width = Length::Px(80);
  absChildStyle.height = Length::Px(40);
  auto absChild = std::make_shared<LayoutBox>(BoxType::BlockNode, absChildStyle, nullptr);
  rel->addChild(absChild);

  LayoutAlgorithm algorithm;
  Dimensions parentDim;
  parentDim.content = {0, 0, 1000, 800};
  algorithm.layout(root, parentDim);

  EXPECT_EQ(root->children().size(), 2u);
  EXPECT_EQ(rel->children().size(), 1u);
}

int main() { return xiaopeng::test::runTests(); }
