#include "test_framework.hpp"

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/flexbox_algorithm.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

// ── Helpers ──────────────────────────────────────────────────

static LayoutBoxPtr makeFlexContainer(FlexDirection dir = FlexDirection::Row,
                                      JustifyContent justify = JustifyContent::FlexStart,
                                      AlignItems align = AlignItems::Stretch) {
  ComputedStyle style;
  style.display = Display::Flex;
  style.flexDirection = dir;
  style.justifyContent = justify;
  style.alignItems = align;
  style.width = Length::Px(500);
  style.height = Length::Px(300);
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);
}

static LayoutBoxPtr makeFlexItem(float width, float height, float flexGrow = 0.0f) {
  ComputedStyle style;
  style.width = Length::Px(width);
  style.height = Length::Px(height);
  style.flexGrow = flexGrow;
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);
}

// ── Tests ────────────────────────────────────────────────────

TEST(FlexboxAdvanced_JustifyCenter) {
  auto container = makeFlexContainer(FlexDirection::Row, JustifyContent::Center);
  auto item1 = makeFlexItem(100, 50);
  container->addChild(item1);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 500;
  parentDim.content.height = 300;
  flexbox.layoutFlexContainer(container, parentDim);

  // Item should be centered horizontally
  EXPECT_GT(item1->dimensions().content.x, 0.0f);
}

TEST(FlexboxAdvanced_JustifySpaceBetween) {
  auto container = makeFlexContainer(FlexDirection::Row, JustifyContent::SpaceBetween);
  auto item1 = makeFlexItem(100, 50);
  auto item2 = makeFlexItem(100, 50);
  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 500;
  parentDim.content.height = 300;
  flexbox.layoutFlexContainer(container, parentDim);

  // Items should be spread apart
  EXPECT_GT(item2->dimensions().content.x, item1->dimensions().content.x + 100.0f);
}

TEST(FlexboxAdvanced_FlexShrink) {
  auto container = makeFlexContainer(FlexDirection::Row);
  auto item1 = makeFlexItem(300, 50, 1.0f);
  auto item2 = makeFlexItem(300, 50, 1.0f);
  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 400;  // Not enough for both items
  parentDim.content.height = 300;
  flexbox.layoutFlexContainer(container, parentDim);

  // Both items should be smaller than their original width
  EXPECT_LT(item1->dimensions().content.width, 300.0f);
  EXPECT_LT(item2->dimensions().content.width, 300.0f);
}

TEST(FlexboxAdvanced_Wrap) {
  ComputedStyle style;
  style.display = Display::Flex;
  style.flexWrap = FlexWrap::Wrap;
  style.width = Length::Px(200);
  style.height = Length::Px(300);
  auto container = std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);

  auto item1 = makeFlexItem(150, 50);
  auto item2 = makeFlexItem(150, 50);
  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 200;
  parentDim.content.height = 300;
  flexbox.layoutFlexContainer(container, parentDim);

  // Items should be wrapped to different lines
  EXPECT_GT(item2->dimensions().content.y, item1->dimensions().content.y);
}

int main() { return xiaopeng::test::runTests(); }
