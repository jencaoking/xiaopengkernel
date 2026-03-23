#pragma once

#include "layout/layout_box.hpp"
#include "renderer/canvas.hpp"
#include "renderer/image.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace xiaopeng {
namespace renderer {

class PaintingAlgorithm {
public:
  void paint(layout::LayoutBoxPtr root, Canvas &canvas);

  /// Clear the decoded-image cache (call on page reload / navigation).
  void clearImageCache() { imageCache_.clear(); }

private:
  void paintBox(layout::LayoutBoxPtr box, Canvas &canvas, int globalX,
                int globalY);

  // Heuristic to determine color based on element type/class for visualization
  Color getColorForBox(layout::LayoutBoxPtr box);

  // Image cache: URL → decoded Image (avoids per-frame re-download/decode)
  std::unordered_map<std::string, std::shared_ptr<Image>> imageCache_;
};

} // namespace renderer
} // namespace xiaopeng
