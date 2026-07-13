#pragma once

#include <memory>
#include <vector>

#include "../css/computed_style.hpp"

namespace xiaopeng {
namespace layout {

class LayoutBox;
using LayoutBoxPtr = std::shared_ptr<LayoutBox>;

/// Represents a fragment of a LayoutBox placed on a specific line.
///
/// A fragment is the atomic unit placed inside a LineBox. For text nodes a
/// fragment covers a contiguous substring [startOffset, endOffset). For
/// inline-level boxes (inline-block, replaced elements) the fragment covers
/// the whole element.
///
/// Coordinates:
///   - `x` is relative to the LineBox's content left edge.
///   - `y` is relative to the LineBox's top edge (set during alignment).
///   - `baseline` is the distance from the fragment's top edge to its
///     baseline. For inline-blocks without an explicit baseline we use the
///     bottom margin edge (CSS 2.1 §10.8.1).
struct BoxFragment {
  LayoutBoxPtr box;
  float x = 0;        // Relative to LineBox content left
  float y = 0;        // Relative to LineBox top (assigned during alignment)
  float width = 0;
  float height = 0;
  float baseline = 0; // Distance from top of fragment to its baseline
  size_t startOffset = 0;
  size_t endOffset = 0;

  // Inline-level alignment requested by this fragment's `vertical-align`.
  css::VerticalAlign verticalAlign = css::VerticalAlign::Baseline;

  // The fragment's own ascent/descent measured from its baseline. For text
  // these come from the font metrics. For atomic inline-level boxes ascent
  // is `baseline` and descent is `height - baseline`.
  float ascent = 0;
  float descent = 0;

  /// True for fragments produced from text nodes (vs. atomic inline boxes).
  bool isText = false;
};

/// A line box is the horizontal strip that holds one row of inline content
/// inside an inline formatting context. Fragments are placed left-to-right
/// and aligned vertically according to their `vertical-align` value.
class LineBox {
public:
  LineBox() = default;

  /// Add a fragment to the line. Updates running ascent/descent and the
  /// line's overall height so that subsequent layout decisions (such as
  /// wrapping) can be made before `finalizeAlignment` is called.
  void addFragment(BoxFragment fragment) {
    fragments_.push_back(fragment);
    width_ += fragment.width;

    // For baseline-aligned fragments, the line's baseline is determined by
    // the maximum ascent above and descent below the baseline.
    if (fragment.verticalAlign == css::VerticalAlign::Baseline) {
      if (fragment.ascent > maxAscent_)
        maxAscent_ = fragment.ascent;
      if (fragment.descent > maxDescent_)
        maxDescent_ = fragment.descent;
    }

    // Track the geometric extents required by top/bottom aligned fragments
    // so we can size the line box to contain them after finalization.
    float fragTop = 0;    // relative to line top (tentative)
    float fragBottom = fragment.height;
    if (fragment.verticalAlign == css::VerticalAlign::Top) {
      topAlignMaxHeight_ = std::max(topAlignMaxHeight_, fragment.height);
    } else if (fragment.verticalAlign == css::VerticalAlign::Bottom) {
      bottomAlignMaxHeight_ = std::max(bottomAlignMaxHeight_, fragment.height);
    } else {
      // Baseline/middle/text-* variants: tentatively place by baseline so
      // we can compute the line's tentative height from ascent+descent.
      (void)fragTop;
      (void)fragBottom;
    }

    // Tentative height (refined by finalizeAlignment).
    height_ = std::max(maxAscent_ + maxDescent_,
                       topAlignMaxHeight_ + bottomAlignMaxHeight_);
  }

  /// Set the minimum "strut" height for the line, derived from the parent
  /// block's font-size and line-height. The line will be at least this tall.
  void setStrut(float ascent, float descent) {
    strutAscent_ = ascent;
    strutDescent_ = descent;
    if (ascent > maxAscent_)
      maxAscent_ = ascent;
    if (descent > maxDescent_)
      maxDescent_ = descent;
    height_ = std::max(maxAscent_ + maxDescent_,
                       topAlignMaxHeight_ + bottomAlignMaxHeight_);
  }

  /// Compute final vertical positions for every fragment based on its
  /// `vertical-align` mode. Must be called once after all fragments are
  /// added and before the line box is consumed by the painter.
  void finalizeAlignment() {
    // The line's actual height is the max of:
    //   - baseline-derived height (maxAscent + maxDescent)
    //   - top+bottom aligned extents
    //   - strut height (parent font's minimum line height)
    float baselineHeight = maxAscent_ + maxDescent_;
    float alignedHeight = topAlignMaxHeight_ + bottomAlignMaxHeight_;
    height_ = std::max({baselineHeight, alignedHeight, height_});

    // Place each fragment according to its vertical-align mode.
    for (auto &frag : fragments_) {
      switch (frag.verticalAlign) {
        case css::VerticalAlign::Top:
          // Top of fragment aligns with top of line box content area.
          frag.y = 0;
          break;
        case css::VerticalAlign::Bottom:
          // Bottom of fragment aligns with bottom of line box content area.
          frag.y = height_ - frag.height;
          break;
        case css::VerticalAlign::Middle:
          // Center of fragment aligns with baseline + half x-height.
          // Approximate x-height as 0.5em of the line strut ascent.
          frag.y = maxAscent_ - frag.height / 2.0f +
                   strutAscent_ * 0.25f;
          break;
        case css::VerticalAlign::TextTop:
          // Top of fragment aligns with top of strut's font ascent.
          frag.y = 0;
          break;
        case css::VerticalAlign::TextBottom:
          // Bottom of fragment aligns with bottom of strut's font descent.
          frag.y = height_ - strutDescent_ - frag.height;
          break;
        case css::VerticalAlign::Sub:
          // Lower by ~0.33em of strut font size.
          frag.y = maxAscent_ - frag.baseline + strutAscent_ * 0.33f;
          break;
        case css::VerticalAlign::Super:
          // Raise by ~0.33em of strut font size.
          frag.y = maxAscent_ - frag.baseline - strutAscent_ * 0.33f;
          break;
        case css::VerticalAlign::Baseline:
        default:
          // Align fragment's baseline with the line's baseline.
          frag.y = maxAscent_ - frag.baseline;
          break;
      }
    }
  }

  const std::vector<BoxFragment> &fragments() const { return fragments_; }
  std::vector<BoxFragment> &fragments() { return fragments_; }

  float width() const { return width_; }
  float height() const { return height_; }
  float y() const { return y_; }
  void setY(float y) { y_ = y; }

  float baseline() const { return maxAscent_; }
  float ascent() const { return maxAscent_; }
  float descent() const { return maxDescent_; }

  void setHeight(float h) { height_ = h; }
  void setWidth(float w) { width_ = w; }

  void shiftFragmentsX(float offset) {
    for (auto &frag : fragments_) {
      frag.x += offset;
    }
  }

  /// Reset the line box to its initial state (preserves allocated capacity).
  void clear() {
    fragments_.clear();
    width_ = 0;
    height_ = 0;
    y_ = 0;
    maxAscent_ = 0;
    maxDescent_ = 0;
    strutAscent_ = 0;
    strutDescent_ = 0;
    topAlignMaxHeight_ = 0;
    bottomAlignMaxHeight_ = 0;
  }

private:
  std::vector<BoxFragment> fragments_;
  float width_ = 0;
  float height_ = 0;
  float y_ = 0; // Relative to parent block content box
  float maxAscent_ = 0;
  float maxDescent_ = 0;
  float strutAscent_ = 0;
  float strutDescent_ = 0;
  float topAlignMaxHeight_ = 0;
  float bottomAlignMaxHeight_ = 0;
};

} // namespace layout
} // namespace xiaopeng
