#pragma once

#include "layout/layout_box.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace xiaopeng {
namespace renderer {

class PaintLayer {
public:
  PaintLayer(layout::LayoutBoxPtr box) : box_(box) {}

  void addNormalFlowChild(layout::LayoutBoxPtr child) {
    normalFlowChildren_.push_back(child);
  }

  void addStackingContextChild(std::shared_ptr<PaintLayer> childLayer) {
    int zIndex = childLayer->box()->style().zIndex;
    if (zIndex < 0) {
      negZOrderList_.push_back(childLayer);
    } else {
      posZOrderList_.push_back(childLayer);
    }
  }

  void sortPosZOrderList() {
    std::sort(posZOrderList_.begin(), posZOrderList_.end(), 
              [](const std::shared_ptr<PaintLayer>& a, const std::shared_ptr<PaintLayer>& b) {
                return a->box()->style().zIndex < b->box()->style().zIndex;
              });
  }

  void sortNegZOrderList() {
    std::sort(negZOrderList_.begin(), negZOrderList_.end(), 
              [](const std::shared_ptr<PaintLayer>& a, const std::shared_ptr<PaintLayer>& b) {
                return a->box()->style().zIndex < b->box()->style().zIndex;
              });
  }

  layout::LayoutBoxPtr box() const { return box_; }
  const std::vector<std::shared_ptr<PaintLayer>>& negZOrderList() const { return negZOrderList_; }
  const std::vector<layout::LayoutBoxPtr>& normalFlowChildren() const { return normalFlowChildren_; }
  const std::vector<std::shared_ptr<PaintLayer>>& posZOrderList() const { return posZOrderList_; }

  static bool createsStackingContext(layout::LayoutBoxPtr box) {
    // Root element always creates a stacking context
    if (!box->parent().lock()) {
      return true;
    }
    
    const auto& style = box->style();
    
    // Positioned elements with a z-index value other than "auto"
    if (style.position != css::Position::Static && style.zIndex != 0) { // MVP: assuming 0 is auto
      return true;
    }
    
    // Elements with opacity less than 1
    if (style.opacity < 1.0f) {
      return true;
    }
    
    // Elements with transform
    if (!style.transform.empty() && style.transform != "none") {
      return true;
    }
    
    return false;
  }

private:
  layout::LayoutBoxPtr box_;
  std::vector<std::shared_ptr<PaintLayer>> negZOrderList_;
  std::vector<layout::LayoutBoxPtr> normalFlowChildren_;
  std::vector<std::shared_ptr<PaintLayer>> posZOrderList_;
};

using PaintLayerPtr = std::shared_ptr<PaintLayer>;

} // namespace renderer
} // namespace xiaopeng
