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

enum class BoxType { BlockNode, InlineNode, InlineBlockNode, AnonymousBlock, AnonymousInline };

/// Tracks a single floated box's position and dimensions within a BFC.
struct FloatInfo {
  LayoutBoxPtr box;
  float x = 0;        // Left edge of margin box (relative to BFC content edge)
  float y = 0;        // Top edge of margin box
  float width = 0;    // Outer width (margin+border+padding+content)
  float height = 0;   // Outer height
  bool isLeft = true; // true for float:left, false for float:right
};

class LayoutBox;
using LayoutBoxPtr = std::shared_ptr<LayoutBox>;

class LayoutBox : public std::enable_shared_from_this<LayoutBox> {
public:
  LayoutBox(BoxType type, css::ComputedStyle style, dom::NodePtr node)
      : type_(type), style_(std::move(style)), node_(node) {}

  virtual ~LayoutBox() = default;

  BoxType type() const { return type_; }
  const css::ComputedStyle &style() const { return style_; }
  dom::NodePtr node() const { return node_; }

  Dimensions &dimensions() { return dimensions_; }
  const Dimensions &dimensions() const { return dimensions_; }

  const std::vector<LayoutBoxPtr> &children() const { return children_; }

  void addChild(LayoutBoxPtr child) { 
    child->parent_ = weak_from_this();
    children_.push_back(child); 
  }

  std::weak_ptr<LayoutBox> parent() const { return parent_; }

  // Line box management for Block Containers with Inline Formatting Context
  void addLineBox(LineBox line) { lineBoxes_.push_back(line); }
  const std::vector<LineBox> &lineBoxes() const { return lineBoxes_; }
  std::vector<LineBox> &lineBoxes() { return lineBoxes_; }

  // Float support
  bool isFloat() const { return isFloat_; }
  void setFloat(bool v) { isFloat_ = v; }

  // Float tracking for this BFC (set by LayoutAlgorithm after layout)
  const std::vector<FloatInfo> &leftFloats() const { return leftFloats_; }
  const std::vector<FloatInfo> &rightFloats() const { return rightFloats_; }
  void setActiveFloats(const std::vector<FloatInfo> &l,
                       const std::vector<FloatInfo> &r) {
    leftFloats_ = l;
    rightFloats_ = r;
  }

  // Convenience for layout tree construction
  LayoutBoxPtr getLastChild() const {
    if (children_.empty())
      return nullptr;
    return children_.back();
  }

  // Stacking context support
  bool createsStackingContext() const {
    const auto &s = style_;
    
    // Root element creates stacking context
    if (!parent_.lock()) {
      return true;
    }
    
    if (node_ && node_->nodeType() == dom::NodeType::Element) {
      auto elem = std::static_pointer_cast<dom::Element>(node_);
      if (elem->localName() == "html") {
        return true;
      }
    }

    // position: fixed or sticky
    if (s.position == css::Position::Fixed || s.position == css::Position::Sticky) {
      return true;
    }

    // position: absolute or relative with z-index != auto
    if ((s.position == css::Position::Absolute || s.position == css::Position::Relative) &&
        s.zIndex != 0) {
      return true;
    }

    // opacity < 1
    if (s.opacity < 1.0f && s.opacity >= 0.0f) {
      return true;
    }

    // transform != none
    if (!s.transform.empty() && s.transform != "none") {
      return true;
    }

    // filter != none
    if (!s.filter.empty()) {
      return true;
    }

    // perspective != none
    if (!s.perspective.empty()) {
      return true;
    }

    // isolation: isolate
    if (s.isolation == css::Isolation::Isolate) {
      return true;
    }

    // will-change with stacking context properties
    if (!s.willChange.empty()) {
      std::string willChange = s.willChange;
      if (willChange.find("transform") != std::string::npos ||
          willChange.find("opacity") != std::string::npos ||
          willChange.find("filter") != std::string::npos ||
          willChange.find("perspective") != std::string::npos) {
        return true;
      }
    }

    return false;
  }

  // Get stacking level within parent stacking context
  int stackingLevel() const {
    const auto &s = style_;
    
    if (createsStackingContext()) {
      return s.zIndex;
    }
    
    return 0;
  }

  // Find the nearest ancestor stacking context
  LayoutBoxPtr nearestStackingContext() const {
    auto parent = parent_.lock();
    while (parent) {
      if (parent->createsStackingContext()) {
        return parent;
      }
      parent = parent->parent_.lock();
    }
    return nullptr;
  }

private:
  BoxType type_;
  css::ComputedStyle style_;
  dom::NodePtr node_;
  Dimensions dimensions_;
  std::vector<LayoutBoxPtr> children_;
  std::vector<LineBox> lineBoxes_;
  std::weak_ptr<LayoutBox> parent_;

  // Float support
  bool isFloat_ = false;
  std::vector<FloatInfo> leftFloats_;
  std::vector<FloatInfo> rightFloats_;
};

} // namespace layout
} // namespace xiaopeng
