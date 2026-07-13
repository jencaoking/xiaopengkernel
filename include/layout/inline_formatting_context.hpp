#pragma once

#include <memory>
#include <vector>

#include "../css/computed_style.hpp"
#include "layout_box.hpp"
#include "line_box.hpp"

namespace xiaopeng {
namespace layout {

class LayoutAlgorithm;

/// InlineFormattingContext (IFC) implements the CSS inline formatting
/// context for a block container.
///
/// Responsibilities:
///   - Collect inline-level children (text nodes, inline elements, atomic
///     inline-level boxes such as `inline-block` / replaced elements).
///   - Break the inline content into fragments and pack them into line
///     boxes, applying soft wrapping at the content edge.
///   - Honour `white-space`, `line-height`, `text-indent`, `text-align`
///     and `vertical-align` on the participating boxes.
///   - Reserve horizontal space for floats active in the surrounding BFC.
///
/// Non-goals (deliberately deferred):
///   - Bidirectional / RTL reordering.
///   - Glyph-level shaping (delegated to the renderer).
///   - Justify inter-word spacing distribution (we only align left/right/center).
///
/// The member function definitions live in `layout_algorithm.hpp` because
/// they require the full definition of `LayoutAlgorithm` (the IFC drives
/// atomic inline-level box layout by calling its private block-layout
/// helpers, made accessible via friendship).
class InlineFormattingContext {
public:
  /// Construct an IFC. `activeFloats` is the set of floats established by
  /// ancestor BFCs that intrude into the inline content area; they reduce
  /// the available width on the lines they overlap.
  explicit InlineFormattingContext(std::vector<LayoutBoxPtr> activeFloats = {})
      : activeFloats_(std::move(activeFloats)) {}

  /// Lay out the inline content of `box`. Produces line boxes on `box` and
  /// updates `box->dimensions().content.height` to the total line height.
  void layout(LayoutAlgorithm &algo, LayoutBoxPtr box);

private:
  // Recursively collect inline-level items from `box` into `out`.
  // Inline elements (span, em, strong, ...) are descended into transparently
  // so their text descendants participate in the same line box flow.
  void collectInlineItems(LayoutBoxPtr box,
                          std::vector<LayoutBoxPtr> &out) const;

  // Lay out a single text node, breaking it into word/space fragments and
  // respecting the node's `white-space` value.
  void layoutText(LayoutBoxPtr parent, LayoutBoxPtr nodeBox,
                  LineBox &currentLine, float &currentX, float maxWidth,
                  float startX, bool &lineHasContent);

  // Lay out an atomic inline-level box (inline-block / replaced element).
  void layoutInlineBlock(LayoutAlgorithm &algo, LayoutBoxPtr parent,
                         LayoutBoxPtr item, LineBox &currentLine,
                         float &currentX, float contentWidth,
                         float leftFloatWidth);

  // Apply text-align to all line boxes of `box`.
  void applyTextAlignment(LayoutBoxPtr box, float contentWidth,
                          float textIndent) const;

  // Compute the strut ascent/descent for a block container based on its
  // font-size and line-height properties.
  void computeStrut(LayoutBoxPtr box, float &outAscent,
                    float &outDescent) const;

  // Compute the effective line height (px) for a given computed style and
  // resolved font size.
  float effectiveLineHeight(const css::ComputedStyle &style,
                            int fontSize) const;

  // Compute the float-induced horizontal insets for the IFC's content area.
  void computeFloatInsets(float &leftFloatWidth,
                          float &rightFloatWidth) const;

  // Force-wrap the current line and start a fresh one.
  void breakLine(LayoutBoxPtr box, LineBox &currentLine, float &currentX,
                 float startX) const;

  std::vector<LayoutBoxPtr> activeFloats_;
};

} // namespace layout
} // namespace xiaopeng
