#pragma once

// ═══════════════════════════════════════════════════════════════
//  Phase 2: Stacking Context Builder
//
//  Implements CSS 2.1 / CSS Positioned Layout Module stacking
//  context creation rules and painting order.
//
//  A stacking context is formed by an element that has:
//    1. Root element (html)
//    2. position: absolute/relative with z-index != auto
//    3. position: fixed/sticky
//    4. flex item with z-index != auto
//    5. grid item with z-index != auto
//    6. opacity < 1
//    7. transform != none
//    8. filter != none
//    9. perspective != none
//   10. isolation: isolate
//   11. will-change for z-index, opacity, transform, filter, perspective
//   12. contain: layout, paint, strict, or content
//   13. mix-blend-mode != normal
//
//  Painting order within a stacking context:
//    1. Background & borders of the element forming the context
//    2. Descendant stacking contexts with negative z-index
//    3. In-flow, non-inline-level descendants
//    4. Non-positioned floats
//    5. In-flow inline-level descendants
//    6. Positioned descendants with z-index: auto / z-index: 0
//    7. Descendant stacking contexts with positive z-index
// ═══════════════════════════════════════════════════════════════

#pragma once

#include "../css/computed_style.hpp"
#include "../dom/dom.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace xiaopeng {
namespace layout {

// Forward declaration
class StackingContext;
using StackingContextPtr = std::shared_ptr<StackingContext>;

// ── Stacking Context ─────────────────────────────────────────
class StackingContext : public std::enable_shared_from_this<StackingContext> {
public:
  StackingContext(dom::ElementPtr element, css::ComputedStyle style,
                  StackingContextPtr parent = nullptr)
      : element_(element), style_(std::move(style)), parent_(parent) {}

  // ── Static: Does this element create a new stacking context? ──
  static bool createsStackingContext(const css::ComputedStyle &style,
                                     dom::ElementPtr element) {
    // 1. Root element
    if (element && element->localName() == "html") return true;

    // 2. Position absolute/relative with z-index != auto
    //    (auto means "use parent context's z-index", not 0)
    if ((style.position == css::Position::Relative ||
         style.position == css::Position::Absolute) &&
        style.zIndex != 0) {
      // Note: z-index auto is represented as a special value.
      // In our ComputedStyle, zIndex defaults to 0.
      // A true "auto" would need a separate flag. For now,
      // any non-zero z-index on positioned elements creates a context.
      return true;
    }

    // 3. Position fixed or sticky always creates a context
    if (style.position == css::Position::Fixed ||
        style.position == css::Position::Sticky) {
      return true;
    }

    // 4. Opacity < 1
    if (style.opacity < 1.0f) return true;

    // 5. Transform != none
    if (!style.transform.empty() && style.transform != "none") return true;

    // 6. Filter != none
    if (!style.filter.empty() && style.filter != "none") return true;

    // 7. Perspective != none
    if (!style.perspective.empty() && style.perspective != "none") return true;

    // 8. Isolation: isolate
    if (style.isolation == css::Isolation::Isolate) return true;

    // 9. will-change for stacking context properties
    if (!style.willChange.empty()) {
      const std::string &wc = style.willChange;
      if (wc.find("z-index") != std::string::npos ||
          wc.find("opacity") != std::string::npos ||
          wc.find("transform") != std::string::npos ||
          wc.find("filter") != std::string::npos ||
          wc.find("perspective") != std::string::npos) {
        return true;
      }
    }

    // 10. mix-blend-mode != normal (stored in otherProperties)
    auto it = style.otherProperties.find("mix-blend-mode");
    if (it != style.otherProperties.end() && it->second != "normal") {
      return true;
    }

    // 11. contain: layout | paint | strict | content
    auto cit = style.otherProperties.find("contain");
    if (cit != style.otherProperties.end()) {
      const std::string &cv = cit->second;
      if (cv == "layout" || cv == "paint" || cv == "strict" || cv == "content") {
        return true;
      }
    }

    return false;
  }

  // ── Build the stacking context tree from a DOM subtree ──
  static StackingContextPtr buildTree(dom::ElementPtr root,
                                      const css::ComputedStyle &rootStyle) {
    auto ctx = std::make_shared<StackingContext>(root, rootStyle, nullptr);
    ctx->collectChildren(root);
    return ctx;
  }

  // ── Get flattened painting order ──
  // Returns elements in the order they should be painted
  std::vector<StackingContextPtr> paintingOrder() const {
    std::vector<StackingContextPtr> result;
    paintSelfAndDescendants(result);
    return result;
  }

  // ── Accessors ──
  dom::ElementPtr element() const { return element_; }
  const css::ComputedStyle &style() const { return style_; }
  int zIndex() const { return style_.zIndex; }
  StackingContextPtr parent() const { return parent_.lock(); }

  const std::vector<StackingContextPtr> &children() const { return children_; }

  // ── Sort children by z-index for painting ──
  void sortChildren() {
    std::stable_sort(children_.begin(), children_.end(),
                     [](const StackingContextPtr &a, const StackingContextPtr &b) {
                       return a->zIndex() < b->zIndex();
                     });
  }

private:
  dom::ElementPtr element_;
  css::ComputedStyle style_;
  std::weak_ptr<StackingContext> parent_;
  std::vector<StackingContextPtr> children_;

  // Recursively collect child stacking contexts
  void collectChildren(dom::ElementPtr element) {
    if (!element) return;

    for (const auto &child : element->childNodes()) {
      if (child->nodeType() != dom::NodeType::Element) continue;
      auto childElem = std::static_pointer_cast<dom::Element>(child);

      // Compute child style (simplified - in real engine, use StyleResolver)
      css::ComputedStyle childStyle;
      // TODO: integrate with StyleResolver for proper style computation
      // For now, read inline style attributes
      auto pos = childElem->getAttribute("style");
      if (pos.has_value()) {
        // Parse inline style for position, z-index, opacity, transform
        parseInlineStyle(pos.value(), childStyle);
      }

      if (createsStackingContext(childStyle, childElem)) {
        auto childCtx =
            std::make_shared<StackingContext>(childElem, childStyle, shared_from_this());
        childCtx->collectChildren(childElem);
        children_.push_back(childCtx);
      } else {
        // Not a stacking context — recurse into children
        collectChildren(childElem);
      }
    }
  }

  // Paint self and descendants in CSS painting order
  void paintSelfAndDescendants(std::vector<StackingContextPtr> &result) const {
    // Step 1: Background & borders (this element)
    result.push_back(const_cast<StackingContext *>(this)->shared_from_this());

    // Step 2: Negative z-index children (sorted)
    std::vector<StackingContextPtr> negZ, zeroZ, posZ;
    for (const auto &child : children_) {
      if (child->zIndex() < 0)
        negZ.push_back(child);
      else if (child->zIndex() == 0)
        zeroZ.push_back(child);
      else
        posZ.push_back(child);
    }

    std::sort(negZ.begin(), negZ.end(),
              [](auto &a, auto &b) { return a->zIndex() < b->zIndex(); });
    std::sort(posZ.begin(), posZ.end(),
              [](auto &a, auto &b) { return a->zIndex() < b->zIndex(); });

    // Negative z-index (painted before this element's content)
    for (auto &child : negZ) {
      child->paintSelfAndDescendants(result);
    }

    // Steps 3-5: In-flow content (handled by layout, not here)

    // Step 6: z-index: 0 / auto positioned elements
    for (auto &child : zeroZ) {
      child->paintSelfAndDescendants(result);
    }

    // Step 7: Positive z-index
    for (auto &child : posZ) {
      child->paintSelfAndDescendants(result);
    }
  }

  // Minimal inline style parser for position/z-index/opacity/transform
  static void parseInlineStyle(const std::string &styleStr,
                               css::ComputedStyle &style) {
    // Split by semicolons
    size_t pos = 0;
    while (pos < styleStr.size()) {
      size_t semi = styleStr.find(';', pos);
      if (semi == std::string::npos) semi = styleStr.size();
      std::string decl = styleStr.substr(pos, semi - pos);
      pos = semi + 1;

      // Trim whitespace
      size_t start = decl.find_first_not_of(" \t\n");
      if (start == std::string::npos) continue;
      decl = decl.substr(start);
      size_t end = decl.find_last_not_of(" \t\n");
      if (end != std::string::npos) decl = decl.substr(0, end + 1);

      // Split by colon
      size_t colon = decl.find(':');
      if (colon == std::string::npos) continue;
      std::string prop = decl.substr(0, colon);
      std::string val = decl.substr(colon + 1);

      // Trim
      start = prop.find_first_not_of(" \t");
      if (start != std::string::npos) prop = prop.substr(start);
      end = prop.find_last_not_of(" \t");
      if (end != std::string::npos) prop = prop.substr(0, end + 1);

      start = val.find_first_not_of(" \t");
      if (start != std::string::npos) val = val.substr(start);
      end = val.find_last_not_of(" \t");
      if (end != std::string::npos) val = val.substr(0, end + 1);

      if (prop == "position") {
        if (val == "relative") style.position = css::Position::Relative;
        else if (val == "absolute") style.position = css::Position::Absolute;
        else if (val == "fixed") style.position = css::Position::Fixed;
        else if (val == "sticky") style.position = css::Position::Sticky;
      } else if (prop == "z-index") {
        try { style.zIndex = std::stoi(val); } catch (...) {}
      } else if (prop == "opacity") {
        try { style.opacity = std::stof(val); } catch (...) {}
      } else if (prop == "transform") {
        style.transform = val;
      } else if (prop == "filter") {
        style.filter = val;
      } else if (prop == "isolation") {
        if (val == "isolate") style.isolation = css::Isolation::Isolate;
      } else if (prop == "will-change") {
        style.willChange = val;
      } else if (prop == "perspective") {
        style.perspective = val;
      }
    }
  }
};

// ── Stacking Context Utilities ───────────────────────────────

// Find the stacking context that an element belongs to
inline StackingContextPtr findContainingStackingContext(
    dom::ElementPtr element, StackingContextPtr root) {
  if (!root || !element) return nullptr;

  // Check if any child context contains this element
  for (const auto &child : root->children()) {
    if (child->element() == element) return child;
    auto found = findContainingStackingContext(element, child);
    if (found) return found;
  }

  // If not found in any child context, this root is the containing context
  return root;
}

// Get the effective z-index of an element within its stacking context
inline int getEffectiveZIndex(dom::ElementPtr element,
                              const css::ComputedStyle &style,
                              StackingContextPtr root) {
  // If element creates its own stacking context, use its z-index
  if (StackingContext::createsStackingContext(style, element)) {
    return style.zIndex;
  }
  // Otherwise, it participates in the parent context's ordering
  return 0;
}

// Check if element A should be painted before element B
inline bool shouldPaintBefore(dom::ElementPtr a, const css::ComputedStyle &styleA,
                              dom::ElementPtr b, const css::ComputedStyle &styleB,
                              StackingContextPtr root) {
  int zA = getEffectiveZIndex(a, styleA, root);
  int zB = getEffectiveZIndex(b, styleB, root);

  if (zA != zB) return zA < zB;

  // Same z-index: later in DOM order paints on top
  // (This is a simplification; real engines track source order)
  return false;
}

} // namespace layout
} // namespace xiaopeng
