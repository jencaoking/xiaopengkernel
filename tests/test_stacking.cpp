#include "test_framework.hpp"

#include "../include/css/css_types.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/stacking_context.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

// ── Helpers ──────────────────────────────────────────────────

static LayoutBoxPtr makeBoxWithStyle(const ComputedStyle &style) {
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);
}

// ── Tests ────────────────────────────────────────────────────

TEST(StackingContext_RootAlwaysCreates) {
  ComputedStyle style;
  style.display = Display::Block;
  auto box = makeBoxWithStyle(style);
  // Root (no parent) always creates stacking context
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_PositionedWithZIndex) {
  ComputedStyle style;
  style.display = Display::Block;
  style.position = Position::Relative;
  style.zIndex = 1;
  auto box = makeBoxWithStyle(style);
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_OpacityLessThanOne) {
  ComputedStyle style;
  style.display = Display::Block;
  style.opacity = 0.5f;
  auto box = makeBoxWithStyle(style);
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_TransformNone) {
  ComputedStyle style;
  style.display = Display::Block;
  style.transform = "none";
  auto box = makeBoxWithStyle(style);
  // "none" should NOT create stacking context
  EXPECT_FALSE(box->createsStackingContext());
}

TEST(StackingContext_TransformPresent) {
  ComputedStyle style;
  style.display = Display::Block;
  style.transform = "rotate(45deg)";
  auto box = makeBoxWithStyle(style);
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_FixedPosition) {
  ComputedStyle style;
  style.display = Display::Block;
  style.position = Position::Fixed;
  auto box = makeBoxWithStyle(style);
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_IsolationIsolate) {
  ComputedStyle style;
  style.display = Display::Block;
  style.isolation = Isolation::Isolate;
  auto box = makeBoxWithStyle(style);
  EXPECT_TRUE(box->createsStackingContext());
}

TEST(StackingContext_ZIndexSorting) {
  // Test that z-index values are correctly read
  ComputedStyle style1;
  style1.zIndex = 1;
  auto box1 = makeBoxWithStyle(style1);

  ComputedStyle style2;
  style2.zIndex = 2;
  auto box2 = makeBoxWithStyle(style2);

  ComputedStyle style3;
  style3.zIndex = -1;
  auto box3 = makeBoxWithStyle(style3);

  EXPECT_LT(box1->style().zIndex, box2->style().zIndex);
  EXPECT_GT(box1->style().zIndex, box3->style().zIndex);
}

int main() { return xiaopeng::test::runTests(); }
