#pragma once

#include "../css/computed_style.hpp"
#include "../dom/dom.hpp"
#include <memory>
#include <optional>
#include <vector>

// Forward declare LineBox to avoid circular dependency
// Actually we need the full definition for std::vector<LineBox> member.
// So we will include it at the end or use incomplete type trick?
// No, std::vector<T> allows T to be incomplete in some std lib versions but
// generally risky. Let's implement LineBox in a separate way or header
// structure.

// To break cycle:
// line_box.hpp will NOT include layout_box.hpp. It will strictly forward
// declare.
#include "line_box.hpp"

namespace xiaopeng {
namespace layout {

struct Rect {
  float x = 0;
  float y = 0;
  float width = 0;
  float height = 0;
};

struct EdgeSizes {
  float top = 0;
  float right = 0;
  float bottom = 0;
  float left = 0;
};

// Dimensions of a box model
struct Dimensions {
  Rect content;
  EdgeSizes padding;
  EdgeSizes border;
  EdgeSizes margin;
};

enum class BoxType { BlockNode, InlineNode, AnonymousBlock, AnonymousInline };

class LayoutBox;
using LayoutBoxPtr = std::shared_ptr<LayoutBox>;

class LayoutBox : public std::enable_shared_from_this<LayoutBox> {
public:
  LayoutBox(BoxType type, css::ComputedStyle style, dom::NodePtr node)
      : type_(type), style_(std::move(style)), node_(node) {}

  virtual ~LayoutBox() = default;

  BoxType type() const { return type_; }
  const css::ComputedStyle &style() const { return style_; }
  dom::NodePtr node() const { return node_; } // Can be null for anonymous boxes

  Dimensions &dimensions() { return dimensions_; }
  const Dimensions &dimensions() const { return dimensions_; }

  const std::vector<LayoutBoxPtr> &children() const { return children_; }

  void addChild(LayoutBoxPtr child) { children_.push_back(child); }

  // Line box management for Block Containers with Inline Formatting Context
  void addLineBox(LineBox line) { lineBoxes_.push_back(line); }
  const std::vector<LineBox> &lineBoxes() const { return lineBoxes_; }
  std::vector<LineBox> &lineBoxes() { return lineBoxes_; } // Mutable access

  // Convenience for layout tree construction
  LayoutBoxPtr getLastChild() const {
    if (children_.empty())
      return nullptr;
    return children_.back();
  }

  // Virtual method for layout calculation could be added here or kept in
  // algorithm Let's keep logic separate for now.

private:
  BoxType type_;
  css::ComputedStyle style_;
  dom::NodePtr node_;
  Dimensions dimensions_;
  std::vector<LayoutBoxPtr> children_;
  std::vector<LineBox> lineBoxes_; // For Block Formatting Context
};

} // namespace layout
} // namespace xiaopeng
