// Comprehensive tests for the Inline Formatting Context (IFC) implementation.
//
// Covers:
//   - Basic text wrapping and line-box generation
//   - Inline-block (atomic inline-level box) layout and baseline placement
//   - vertical-align: baseline / top / middle / bottom
//   - white-space: normal / pre / nowrap / pre-wrap / pre-line
//   - line-height: number / length / percent / normal
//   - text-indent on the first formatted line
//   - text-align: left / center / right

#include "test_framework.hpp"

#include <dom/dom.hpp>
#include <layout/layout_algorithm.hpp>
#include <layout/layout_box.hpp>
#include <layout/line_box.hpp>

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::dom;
using namespace xiaopeng::layout;

namespace {

// Helper: build a block container with explicit width.
LayoutBoxPtr makeBlock(float width = 0) {
  auto node = std::make_shared<Element>("div");
  ComputedStyle style;
  style.display = Display::Block;
  if (width > 0) {
    style.width = Length::Px(width);
  }
  return std::make_shared<LayoutBox>(BoxType::BlockNode, style, node);
}

// Helper: build a text inline box carrying the given string.
LayoutBoxPtr makeText(const std::string &content,
                      const ComputedStyle &style = ComputedStyle{}) {
  auto node = std::make_shared<TextNode>(content);
  auto s = style;
  s.display = Display::Inline;
  return std::make_shared<LayoutBox>(BoxType::InlineNode, s, node);
}

// Helper: build an inline-block box with given w/h.
LayoutBoxPtr makeInlineBlock(float w, float h) {
  auto node = std::make_shared<Element>("span");
  ComputedStyle style;
  style.display = Display::InlineBlock;
  style.width = Length::Px(w);
  style.height = Length::Px(h);
  return std::make_shared<LayoutBox>(BoxType::InlineBlockNode, style, node);
}

// Run layout with a 1000px-wide viewport (container width comes from style).
void layoutRoot(LayoutBoxPtr root, float viewportWidth = 1000) {
  Dimensions viewport;
  viewport.content.width = viewportWidth;
  LayoutAlgorithm algo;
  algo.layout(root, viewport);
}

} // namespace

// ---------------------------------------------------------------------------
// Basic text wrapping
// ---------------------------------------------------------------------------

TEST(IFC_SingleLineFits) {
  // width=100, "Hello World" ≈ 5*8 + 8 + 5*8 = 88px → 1 line.
  auto root = makeBlock(100);
  root->addChild(makeText("Hello World"));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 1u);
}

TEST(IFC_ForcedWrapTwoLines) {
  // width=60, "Hello World" exceeds 60px after the space → 2 lines.
  auto root = makeBlock(60);
  root->addChild(makeText("Hello World"));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 2u);
  // Each line is at least the strut height (~20px) → total ≥ 30.
  EXPECT_GE(root->dimensions().content.height, 30.0f);
}

TEST(IFC_EmptyTextProducesNoLineBoxes) {
  auto root = makeBlock(100);
  root->addChild(makeText(""));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 0u);
}

TEST(IFC_LeadingSpacesAreCollapsed) {
  // Leading whitespace in `normal` white-space must not produce a leading
  // blank line; the first word goes on the first line.
  auto root = makeBlock(200);
  root->addChild(makeText("   hello"));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 1u);
  if (!root->lineBoxes().empty()) {
    EXPECT_EQ(root->lineBoxes()[0].fragments().size(), 1u);
  }
}

TEST(IFC_MultipleSpacesCollapseToOne) {
  auto root = makeBlock(200);
  root->addChild(makeText("a     b"));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 1u);
  // Two word fragments on the line.
  if (!root->lineBoxes().empty()) {
    EXPECT_EQ(root->lineBoxes()[0].fragments().size(), 2u);
  }
}

// ---------------------------------------------------------------------------
// Inline-block (atomic inline-level box) participation
// ---------------------------------------------------------------------------

TEST(IFC_InlineBlockParticipatesInLine) {
  // Text + inline-block on the same line.
  auto root = makeBlock(300);
  root->addChild(makeText("Hi "));
  root->addChild(makeInlineBlock(40, 40));
  layoutRoot(root);

  EXPECT_EQ(root->lineBoxes().size(), 1u);
  if (!root->lineBoxes().empty()) {
    const auto &frags = root->lineBoxes()[0].fragments();
    // 1 text fragment ("Hi") + 1 inline-block fragment.
    EXPECT_EQ(frags.size(), 2u);
  }
}

TEST(IFC_InlineBlockWrapsWhenDoesNotFit) {
  // 50px text + 100px inline-block won't fit in 100px container.
  auto root = makeBlock(100);
  root->addChild(makeText("Hello")); // 5*8 = 40px
  root->addChild(makeInlineBlock(100, 30));
  layoutRoot(root);

  // "Hello" on line 1, inline-block wraps to line 2.
  EXPECT_EQ(root->lineBoxes().size(), 2u);
}

TEST(IFC_InlineBlockBaselineIsBottomEdge) {
  // For `inline-block` with `overflow: visible`, the baseline is the
  // bottom margin edge (CSS 2.1 §10.8.1). With vertical-align: baseline
  // the fragment's `y` should place its bottom on the line baseline.
  auto root = makeBlock(200);
  root->addChild(makeInlineBlock(40, 40));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &frags = root->lineBoxes()[0].fragments();
  ASSERT_EQ(frags.size(), 1u);
  const auto &frag = frags[0];
  // baseline (distance from top) should equal fragment height.
  EXPECT_EQ(frag.baseline, frag.height);
}

// ---------------------------------------------------------------------------
// vertical-align
// ---------------------------------------------------------------------------

TEST(IFC_VerticalAlignTop) {
  // A 40px tall inline-block aligned to the top should sit at y=0
  // regardless of the strut's baseline.
  auto ib = makeInlineBlock(40, 40);
  ib->style(); // touch
  // Mutate style via re-creation since ComputedStyle is read-only via
  // the const accessor. We rebuild the box with the desired style.
  ComputedStyle style;
  style.display = Display::InlineBlock;
  style.width = Length::Px(40);
  style.height = Length::Px(40);
  style.verticalAlign = VerticalAlign::Top;
  auto topAligned = std::make_shared<LayoutBox>(
      BoxType::InlineBlockNode, style, std::make_shared<Element>("span"));

  auto root = makeBlock(200);
  root->addChild(topAligned);
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &frags = root->lineBoxes()[0].fragments();
  ASSERT_EQ(frags.size(), 1u);
  // top-aligned fragment should be at y=0.
  EXPECT_EQ(frags[0].y, 0.0f);
}

TEST(IFC_VerticalAlignBottom) {
  ComputedStyle style;
  style.display = Display::InlineBlock;
  style.width = Length::Px(40);
  style.height = Length::Px(40);
  style.verticalAlign = VerticalAlign::Bottom;
  auto bottomAligned = std::make_shared<LayoutBox>(
      BoxType::InlineBlockNode, style, std::make_shared<Element>("span"));

  auto root = makeBlock(200);
  root->addChild(bottomAligned);
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &line = root->lineBoxes()[0];
  ASSERT_EQ(line.fragments().size(), 1u);
  const auto &frag = line.fragments()[0];
  // bottom-aligned: y = lineHeight - fragHeight
  EXPECT_GE(frag.y + frag.height, line.height() - 0.001f);
}

// ---------------------------------------------------------------------------
// white-space
// ---------------------------------------------------------------------------

TEST(IFC_WhiteSpace_NoWrapDoesNotWrap) {
  // `white-space: nowrap` should keep everything on one line even if it
  // overflows the container.
  ComputedStyle style;
  style.whiteSpace = WhiteSpace::NoWrap;
  auto root = makeBlock(20);
  root->addChild(makeText("abcdefghijklmnopqrstuvwxyz", style));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 1u);
}

TEST(IFC_WhiteSpace_PreservesNewline) {
  // `white-space: pre-line` preserves newlines as forced breaks.
  ComputedStyle style;
  style.whiteSpace = WhiteSpace::PreLine;
  auto root = makeBlock(500);
  root->addChild(makeText("line one\nline two", style));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 2u);
}

TEST(IFC_WhiteSpace_PrePreservesSpaces) {
  // `white-space: pre` preserves runs of spaces.
  ComputedStyle style;
  style.whiteSpace = WhiteSpace::Pre;
  auto root = makeBlock(500);
  root->addChild(makeText("a    b", style));
  layoutRoot(root);
  EXPECT_EQ(root->lineBoxes().size(), 1u);
  // With `pre`, the four spaces contribute their full width; we just
  // verify the line was produced.
  ASSERT_FALSE(root->lineBoxes().empty());
}

TEST(IFC_WhiteSpace_PreWrapWrapsLongLines) {
  // `white-space: pre-wrap` preserves spaces AND wraps long lines.
  // Use multiple words so wrapping can occur at space boundaries (a single
  // unbreakable word longer than the line overflows rather than being split,
  // since word-breaking is not implemented).
  ComputedStyle style;
  style.whiteSpace = WhiteSpace::PreWrap;
  auto root = makeBlock(100);
  root->addChild(makeText("aaaa bbbb cccc dddd eeee ffff", style));
  layoutRoot(root);
  // Should wrap into multiple lines (not 1).
  EXPECT_GT(root->lineBoxes().size(), 1u);
}

// ---------------------------------------------------------------------------
// line-height
// ---------------------------------------------------------------------------

TEST(IFC_LineHeightNumberIncreasesLineHeight) {
  // line-height: 3 with font-size 16 → 48px line. Default is ~19.2px.
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(500);
  blockStyle.fontSize = Length::Px(16);
  blockStyle.lineHeight = LineHeight::Number(3.0f);
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hi"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  // 16 * 3 = 48px line height.
  EXPECT_GE(root->lineBoxes()[0].height(), 48.0f);
}

TEST(IFC_LineHeightLengthOverridesFontMetrics) {
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(500);
  blockStyle.fontSize = Length::Px(16);
  blockStyle.lineHeight = LineHeight::Length(40.0f);
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hi"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  // With explicit 40px line-height, the strut pushes line height to ~40.
  EXPECT_GE(root->lineBoxes()[0].height(), 40.0f);
}

TEST(IFC_LineHeightPercentScalesFontSize) {
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(500);
  blockStyle.fontSize = Length::Px(16);
  blockStyle.lineHeight = LineHeight::Percent(200.0f); // 2x font-size = 32px
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hi"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  EXPECT_GE(root->lineBoxes()[0].height(), 32.0f);
}

// ---------------------------------------------------------------------------
// text-indent
// ---------------------------------------------------------------------------

TEST(IFC_TextIndentShiftsFirstLineOnly) {
  // text-indent: 50px on a 200px container. The first word "Hello" (40px)
  // starts at x=50; the second word "World" (40px) would land at 50+8+40=98,
  // which still fits in 200, so we expect 1 line with the first fragment
  // shifted by ~50px.
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(200);
  blockStyle.textIndent = Length::Px(50);
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hello World"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &frags = root->lineBoxes()[0].fragments();
  ASSERT_FALSE(frags.empty());
  // First fragment's x should be ≥ 50 (the indent) minus a small tolerance.
  EXPECT_GE(frags[0].x, 49.0f);
}

TEST(IFC_TextIndentDoesNotAffectWrappedLines) {
  // text-indent: 80px on a 100px container. "Hello" (40px) starts at x=80;
  // "World" (40px) would need 80+8+40=128 > 100, so it wraps. The second
  // line's first fragment should NOT carry the indent.
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(100);
  blockStyle.textIndent = Length::Px(80);
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hello World"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 2u);
  // First line: first fragment x ≥ 80.
  EXPECT_GE(root->lineBoxes()[0].fragments()[0].x, 79.0f);
  // Second line: first fragment x should be ~0 (no indent).
  EXPECT_LT(root->lineBoxes()[1].fragments()[0].x, 1.0f);
}

// ---------------------------------------------------------------------------
// text-align
// ---------------------------------------------------------------------------

TEST(IFC_TextAlignRightShiftsContent) {
  // 200px container, "Hi" (~16px). Right-aligned → first fragment x ≈ 184.
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(200);
  blockStyle.textAlign = TextAlign::Right;
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hi"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &frags = root->lineBoxes()[0].fragments();
  ASSERT_EQ(frags.size(), 1u);
  // Right edge of fragment should be near the right edge of content area.
  EXPECT_GE(frags[0].x + frags[0].width, 199.0f);
}

TEST(IFC_TextAlignCentersContent) {
  // 200px container, "Hi" (~16px). Centered → x ≈ 92.
  ComputedStyle blockStyle;
  blockStyle.display = Display::Block;
  blockStyle.width = Length::Px(200);
  blockStyle.textAlign = TextAlign::Center;
  auto root = std::make_shared<LayoutBox>(
      BoxType::BlockNode, blockStyle, std::make_shared<Element>("div"));
  root->addChild(makeText("Hi"));
  layoutRoot(root);

  ASSERT_EQ(root->lineBoxes().size(), 1u);
  const auto &frags = root->lineBoxes()[0].fragments();
  ASSERT_EQ(frags.size(), 1u);
  // Centered: (200 - width) / 2 ≈ 92. Allow some tolerance.
  EXPECT_GE(frags[0].x, 80.0f);
  EXPECT_LE(frags[0].x, 110.0f);
}

// ---------------------------------------------------------------------------
// Mixed inline content (text + inline-block + text)
// ---------------------------------------------------------------------------

TEST(IFC_MixedTextAndInlineBlockOnSameLine) {
  // Text + inline-block + text should all sit on the same line if they fit.
  auto root = makeBlock(500);
  root->addChild(makeText("before "));
  root->addChild(makeInlineBlock(40, 40));
  root->addChild(makeText(" after"));
  layoutRoot(root);

  EXPECT_EQ(root->lineBoxes().size(), 1u);
  if (!root->lineBoxes().empty()) {
    const auto &frags = root->lineBoxes()[0].fragments();
    // 1 (before) + 1 (ib) + 1 (after) = 3 fragments.
    EXPECT_EQ(frags.size(), 3u);
    // Fragments should be in left-to-right order.
    EXPECT_LT(frags[0].x, frags[1].x);
    EXPECT_LT(frags[1].x, frags[2].x);
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return xiaopeng::test::runTests();
}
