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
    if (node_ && node_->nodeType() == dom::NodeType::Element) {
      auto elem = std::static_pointer_cast<dom::Element>(node_);
      if (elem->localName() == "html") {
        return true;
      }
    }

    // A box with no parent is the root of the layout tree and always
    // establishes a stacking context (analogous to the document root).
    if (parent_.expired()) {
      return true;
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
    if (!s.filter.empty() && s.filter != "none") {
      return true;
    }

    // perspective != none
    if (!s.perspective.empty() && s.perspective != "none") {
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
};

} // namespace layout
} // namespace xiaopeng
