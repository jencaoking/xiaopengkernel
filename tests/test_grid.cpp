#include "test_framework.hpp"

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/grid_algorithm.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

// ── Helpers ──────────────────────────────────────────────────

static LayoutBoxPtr makeGridContainer() {
  ComputedStyle style;
  style.display = Display::Grid;
  style.width = Length::Px(600);
  style.height = Length::Px(400);
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);
}

static LayoutBoxPtr makeGridItem(float width, float height) {
  ComputedStyle style;
  style.width = Length::Px(width);
  style.height = Length::Px(height);
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);
}

// ── Tests ────────────────────────────────────────────────────

TEST(Grid_BasicContainer) {
  auto container = makeGridContainer();
  EXPECT_EQ(container->style().display, Display::Grid);
  EXPECT_GT(container->style().width.value, 0.0f);
}

TEST(Grid_AddItems) {
  auto container = makeGridContainer();
  auto item1 = makeGridItem(100, 50);
  auto item2 = makeGridItem(100, 50);

  container->addChild(item1);
  container->addChild(item2);

  EXPECT_EQ(container->children().size(), 2u);
}

TEST(Grid_LayoutAlgorithm) {
  auto container = makeGridContainer();

  // Add 6 items for a 3x2 grid
  for (int i = 0; i < 6; ++i) {
    container->addChild(makeGridItem(100, 50));
  }

  GridAlgorithm grid;
  Dimensions parentDim;
  parentDim.content.width = 600;
  parentDim.content.height = 400;

  grid.layoutGridContainer(container, parentDim);

  // All items should have non-zero dimensions
  for (const auto &child : container->children()) {
    EXPECT_GT(child->dimensions().content.width, 0.0f);
    EXPECT_GT(child->dimensions().content.height, 0.0f);
  }
}

int main() { return xiaopeng::test::runTests(); }
