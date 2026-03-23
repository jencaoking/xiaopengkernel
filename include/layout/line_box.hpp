#pragma once

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
  float baseline = 0;     // Initialize to 0 to fix BUG-026
  size_t startOffset = 0; // For text nodes: start index in text
  size_t endOffset = 0;   // For text nodes: end index in text
};

class LineBox {
public:
  LineBox() = default;

  void addFragment(BoxFragment fragment) {
    fragments_.push_back(fragment);
    width_ += fragment.width;
    if (fragment.height > height_)
      height_ = fragment.height;
  }

  const std::vector<BoxFragment> &fragments() const { return fragments_; }

  float width() const { return width_; }
  float height() const { return height_; }
  float y() const { return y_; }
  void setY(float y) { y_ = y; }

  void setHeight(float h) { height_ = h; }
  void setWidth(float w) { width_ = w; }

private:
  std::vector<BoxFragment> fragments_;
  float width_ = 0;
  float height_ = 0;
  float y_ = 0; // Relative to parent block content box
};

} // namespace layout
} // namespace xiaopeng
