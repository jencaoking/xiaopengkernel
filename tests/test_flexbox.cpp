#include "test_framework.hpp"

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/flexbox_algorithm.hpp"
#include "../include/dom/html_types.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;
using namespace xiaopeng::dom;

// ── Helpers ──────────────────────────────────────────────────

static LayoutBoxPtr makeFlexContainer(FlexDirection dir = FlexDirection::Row,
                                      JustifyContent justify = JustifyContent::FlexStart) {
  ComputedStyle style;
  style.display = Display::Flex;
  style.flexDirection = dir;
  style.justifyContent = justify;
  style.width = Length::Px(800);
  style.height = Length::Px(600);
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

TEST(Flexbox_RowBasic) {
  auto container = makeFlexContainer(FlexDirection::Row);
  auto item1 = makeFlexItem(100, 50);
  auto item2 = makeFlexItem(150, 50);
  auto item3 = makeFlexItem(80, 50);

  container->addChild(item1);
  container->addChild(item2);
  container->addChild(item3);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  // Items should be laid out horizontally
  EXPECT_EQ(container->children().size(), 3u);
  EXPECT_GT(item1->dimensions().content.width, 0.0f);
  EXPECT_GT(item2->dimensions().content.width, 0.0f);
  EXPECT_GT(item3->dimensions().content.width, 0.0f);
}

TEST(Flexbox_RowWithFlexGrow) {
  auto container = makeFlexContainer(FlexDirection::Row);
  auto item1 = makeFlexItem(100, 50, 1.0f);
  auto item2 = makeFlexItem(150, 50, 2.0f);
  auto item3 = makeFlexItem(80, 50, 0.0f);

  container->addChild(item1);
  container->addChild(item2);
  container->addChild(item3);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  // item2 should be larger than item1 due to higher flexGrow
  float w1 = item1->dimensions().content.width;
  float w2 = item2->dimensions().content.width;
  EXPECT_GT(w2, w1);
}

TEST(Flexbox_ColumnBasic) {
  auto container = makeFlexContainer(FlexDirection::Column);
  auto item1 = makeFlexItem(100, 40);
  auto item2 = makeFlexItem(100, 40);

  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  // Items should be stacked vertically
  EXPECT_GT(item1->dimensions().content.y, 0.0f);
  EXPECT_GT(item2->dimensions().content.y, item1->dimensions().content.y);
}

TEST(Flexbox_ColumnWithFlexGrow) {
  auto container = makeFlexContainer(FlexDirection::Column);
  auto item1 = makeFlexItem(100, 40, 1.0f);
  auto item2 = makeFlexItem(100, 40, 2.0f);

  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 300;
  parentDim.content.height = 400;

  flexbox.layoutFlexContainer(container, parentDim);

  // item2 should be taller than item1 due to higher flexGrow
  float h1 = item1->dimensions().content.height;
  float h2 = item2->dimensions().content.height;
  EXPECT_GT(h2, h1);
}

int main() { return xiaopeng::test::runTests(); }
