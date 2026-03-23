#pragma once

#include <dom/html_types.hpp>
#include <iostream>
#include <layout/layout_box.hpp>

namespace xiaopeng {
namespace layout {

// Hit testing: find the deepest DOM element at coordinates (x, y).
// Walks the layout tree in reverse child order (front-to-back) so that
// visually-topmost elements are matched first.
class HitTest {
public:
  // Returns the DOM Node at position (x, y), or nullptr if none found.
  static dom::NodePtr test(const LayoutBoxPtr &root, float x, float y) {
    if (!root)
      return nullptr;

    // Compute the border box (content + padding + border) absolute position
    // assuming dimensions().content.x is relative to the parent's padding edge.
    // Actually, in the current LayoutAlgorithm, child.x is absolute?
    // Let's assume absolute coordinates simplify this, but since layout puts
    // child.content.x relative to parent border box... wait.
    // In LayoutAlgorithm we did:
    // child->content.x = box->padding.left + child->margin.left + ...
    // This implies child->content.x was relative to parent.
    // So we need to compute absolute coordinates recursively.

    // For HitTest, we should pass the absolute origin of the current box
    // as an accumulated offset so that (bx, by) inside check is against (x, y)
    // in screen space.

    // Better yet: we just pass absolute x, y from standard layout. But since
    // LayoutAlgorithm does NOT compute absolute coordinates cleanly (it
    // computes relative), we need `x - bx` passing down OR we calculate
    // absolute bound here.

    // To fix BUG-015 properly based on `HitTest` logic, we assume layout
    // compute gave relative positions. We update `HitTest::test(root, x, y,
    // absX=0, absY=0)` Since `test` signature is fixed, we adjust `bx, by`
    // based on relative logic. Wait, the PaintingAlgorithm computes absolute
    // coordinates on the fly. We should copy PaintingAlgorithm's absolute
    // position calculation.

    return testRecursive(root, x, y, 0, 0);
  }

private:
  static dom::NodePtr testRecursive(const LayoutBoxPtr &root, float x, float y,
                                    float parentAbsX, float parentAbsY) {
    if (!root)
      return nullptr;

    const auto &dims = root->dimensions();

    // Content box absolute position (relative to canvas)
    float contentAbsX = parentAbsX + dims.content.x;
    float contentAbsY = parentAbsY + dims.content.y;

    // Border box absolute position
    float borderBoxX = contentAbsX - dims.padding.left - dims.border.left;
    float borderBoxY = contentAbsY - dims.padding.top - dims.border.top;

    float borderBoxW = dims.border.left + dims.padding.left +
                       dims.content.width + dims.padding.right +
                       dims.border.right;
    float borderBoxH = dims.border.top + dims.padding.top +
                       dims.content.height + dims.padding.bottom +
                       dims.border.bottom;

    // Point in box check (skip zero-size boxes to prevent false hits)
    bool inBox = (borderBoxW > 0 && borderBoxH > 0 && x >= borderBoxX &&
                  x < borderBoxX + borderBoxW && y >= borderBoxY &&
                  y < borderBoxY + borderBoxH);

    // If point is not in this box (and we assume children are contained, though
    // overflow is possible), for simplicity we could bail out, but overflow
    // means we should check children anyway in a real engine. Let's check
    // children if inBox is remotely possible or just always check (since
    // reverse order).

    // Check children first (deepest match wins)
    const auto &children = root->children();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      // Pass this box's border box origin as the parent origin for children
      // (child content.x/y are relative to parent's border box)
      auto hit = testRecursive(*it, x, y, borderBoxX, borderBoxY);
      if (hit)
        return hit;
    }

    if (inBox) {
      auto node = root->node();
      if (node && node->nodeType() == dom::NodeType::Element) {
        return node;
      }
    }

    return nullptr;
  }
};

} // namespace layout
} // namespace xiaopeng
