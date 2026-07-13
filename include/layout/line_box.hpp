#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace xiaopeng {
namespace layout {

class LayoutBox;
using LayoutBoxPtr = std::shared_ptr<LayoutBox>;

// Represents a fragment of a LayoutBox on a specific line.
// For example, an inline <span> might be split across multiple lines.
struct BoxFragment {
  LayoutBoxPtr box;
  float x = 0; // Relative to LineBox
  float y = 0; // Relative to LineBox
  float width = 0;
  float height = 0;
  float baseline = 0;     // Baseline relative to fragment's top edge
  size_t startOffset = 0; // For text nodes: start index in text
  size_t endOffset = 0;   // For text nodes: end index in text
};

class LineBox {
public:
  LineBox() = default;
  explicit LineBox(float strutHeight) : strutHeight_(strutHeight) {
    height_ = strutHeight;
  }

  void addFragment(BoxFragment fragment) {
    fragments_.push_back(fragment);
    width_ += fragment.width;
    
    // Calculate max ascent and descent for baseline alignment
    float ascent = fragment.baseline;
    float descent = fragment.height - fragment.baseline;
    
    if (ascent > maxAscent_) {
      maxAscent_ = ascent;
    }
    if (descent > maxDescent_) {
      maxDescent_ = descent;
    }
    
    // The height of the line box is determined by the max ascent and max descent,
    // but never smaller than the strut (line-height) established by the parent block.
    height_ = std::max(maxAscent_ + maxDescent_, strutHeight_);
  }

  void finalizeAlignment() {
    // Shift fragments vertically so their baselines align with the line's maxAscent
    for (auto &fragment : fragments_) {
      fragment.y = maxAscent_ - fragment.baseline;
    }
  }

  const std::vector<BoxFragment> &fragments() const { return fragments_; }
  std::vector<BoxFragment> &fragments() { return fragments_; }

  float width() const { return width_; }
  float height() const { return height_; }
  float y() const { return y_; }
  void setY(float y) { y_ = y; }
  
  float baseline() const { return maxAscent_; }

  void setHeight(float h) { height_ = h; }
  void setWidth(float w) { width_ = w; }
  void setStrutHeight(float h) { strutHeight_ = h; }

  void shiftFragmentsX(float offset) {
    for (auto &fragment : fragments_) {
      fragment.x += offset;
    }
  }

private:
  std::vector<BoxFragment> fragments_;
  float width_ = 0;
  float height_ = 0;
  float y_ = 0; // Relative to parent block content box
  float maxAscent_ = 0;
  float maxDescent_ = 0;
  float strutHeight_ = 0;
};

} // namespace layout
} // namespace xiaopeng
