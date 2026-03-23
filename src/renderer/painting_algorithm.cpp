#include "renderer/painting_algorithm.hpp"
#include "dom/dom.hpp"
#include "loader/loader.hpp"
#include "renderer/image.hpp"
#include <iostream>
#include <memory>

namespace xiaopeng {
namespace renderer {

void PaintingAlgorithm::paint(layout::LayoutBoxPtr root, Canvas &canvas) {
  if (!root)
    return;

  // Paint background
  canvas.clear(Color::White());

  // Recursive paint — root's border box starts at (0,0)
  paintBox(root, canvas, 0, 0);
}

void PaintingAlgorithm::paintBox(layout::LayoutBoxPtr box, Canvas &canvas,
                                 int parentBorderBoxX, int parentBorderBoxY) {
  const auto &dims = box->dimensions();
  const auto &style = box->style();

  // Helper: convert css::Color to renderer::Color
  auto toColor = [](const css::Color &c) -> Color {
    return {c.r, c.g, c.b, c.a};
  };

  // Content box absolute position (relative to canvas)
  int contentAbsX = parentBorderBoxX + static_cast<int>(dims.content.x);
  int contentAbsY = parentBorderBoxY + static_cast<int>(dims.content.y);

  // Border box absolute position (derived from content box)
  int borderBoxX = contentAbsX - static_cast<int>(dims.padding.left) -
                   static_cast<int>(dims.border.left);
  int borderBoxY = contentAbsY - static_cast<int>(dims.padding.top) -
                   static_cast<int>(dims.border.top);

  int borderBoxW = static_cast<int>(dims.border.left + dims.padding.left +
                                    dims.content.width + dims.padding.right +
                                    dims.border.right);
  int borderBoxH = static_cast<int>(dims.border.top + dims.padding.top +
                                    dims.content.height + dims.padding.bottom +
                                    dims.border.bottom);

  // Viewport bounds clipping: skip boxes entirely outside the canvas
  if (borderBoxX + borderBoxW <= 0 || borderBoxY + borderBoxH <= 0 ||
      borderBoxX >= canvas.width() || borderBoxY >= canvas.height()) {
    return;
  }

  // --- 1. Draw Background (full border box area) ---
  if (style.backgroundColor.a > 0) {
    int bgX = borderBoxX + static_cast<int>(dims.border.left);
    int bgY = borderBoxY + static_cast<int>(dims.border.top);
    int bgW = static_cast<int>(dims.padding.left + dims.content.width +
                               dims.padding.right);
    int bgH = static_cast<int>(dims.padding.top + dims.content.height +
                               dims.padding.bottom);
    canvas.fillRect(bgX, bgY, bgW, bgH, toColor(style.backgroundColor));
  }

  // --- 2. Draw Borders ---

  // Top Border
  if (dims.border.top > 0 && style.borderTopColor.a > 0) {
    canvas.fillRect(borderBoxX, borderBoxY, borderBoxW,
                    static_cast<int>(dims.border.top),
                    toColor(style.borderTopColor));
  }

  // Bottom Border
  if (dims.border.bottom > 0 && style.borderBottomColor.a > 0) {
    int by = borderBoxY + borderBoxH - static_cast<int>(dims.border.bottom);
    canvas.fillRect(borderBoxX, by, borderBoxW,
                    static_cast<int>(dims.border.bottom),
                    toColor(style.borderBottomColor));
  }

  // Left Border
  if (dims.border.left > 0 && style.borderLeftColor.a > 0) {
    canvas.fillRect(borderBoxX, borderBoxY, static_cast<int>(dims.border.left),
                    borderBoxH, toColor(style.borderLeftColor));
  }

  // Right Border
  if (dims.border.right > 0 && style.borderRightColor.a > 0) {
    int rx = borderBoxX + borderBoxW - static_cast<int>(dims.border.right);
    canvas.fillRect(rx, borderBoxY, static_cast<int>(dims.border.right),
                    borderBoxH, toColor(style.borderRightColor));
  }

  // --- 3. Paint Block Children ---
  // Pass THIS box's borderBox origin as the parent origin for children.
  for (const auto &child : box->children()) {
    paintBox(child, canvas, borderBoxX, borderBoxY);
  }

  // --- 3.5. Paint Replaced Elements (Images) ---
  if (box->node() && box->node()->nodeType() == dom::NodeType::Element) {
    auto elem = std::dynamic_pointer_cast<dom::Element>(box->node());
    if (elem && elem->localName() == "img") {
      auto attr = elem->getAttribute("src");
      if (attr.has_value() && !attr.value().empty()) {
        std::string src = attr.value();

        // Cache lookup — avoids per-frame network download + decode
        std::shared_ptr<Image> image;
        auto cacheIt = imageCache_.find(src);
        if (cacheIt != imageCache_.end()) {
          image = cacheIt->second;
        } else {
          image = loader::GetLoader().loadImage(src);
          if (image) {
            imageCache_[src] = image;
            std::cout << "[Painter] Cached image: " << src << " ("
                      << image->width() << "x" << image->height() << ")"
                      << std::endl;
          }
        }

        if (image) {
          int imgX = borderBoxX + static_cast<int>(dims.border.left) +
                     static_cast<int>(dims.padding.left);
          int imgY = borderBoxY + static_cast<int>(dims.border.top) +
                     static_cast<int>(dims.padding.top);
          int imgW = static_cast<int>(dims.content.width);
          int imgH = static_cast<int>(dims.content.height);

          canvas.drawImage(*image, imgX, imgY, imgW, imgH);
        }
      }
    }
  }

  // --- 4. Paint Text Fragments (inline content via LineBoxes) ---
  for (const auto &line : box->lineBoxes()) {
    // line.y() is relative to parent's border box
    int lineAbsY = borderBoxY + static_cast<int>(line.y());

    for (const auto &frag : line.fragments()) {
      if (!frag.box)
        continue;

      // frag.x is relative to content area start
      int fragAbsX = borderBoxX + static_cast<int>(dims.border.left) +
                     static_cast<int>(dims.padding.left) +
                     static_cast<int>(frag.x);
      int fragAbsY = lineAbsY + static_cast<int>(frag.y);

      if (frag.box->node() &&
          frag.box->node()->nodeType() == dom::NodeType::Text) {
        std::string text = frag.box->node()->textContent();
        if (frag.startOffset < text.length()) {
          size_t count = frag.endOffset - frag.startOffset;
          // Clamp count to remaining length
          if (frag.startOffset + count > text.length()) {
            count = text.length() - frag.startOffset;
          }
          text = text.substr(frag.startOffset, count);
        } else {
          text = "";
        }

        // Skip empty text fragments
        if (text.empty())
          continue;

        const auto &c = frag.box->style().color;

        // Determine scale for fallback or if font size is relative
        int scale = 1;
        if (frag.box->node()->parentNode() &&
            frag.box->node()->parentNode()->nodeType() ==
                dom::NodeType::Element) {
          auto parent = std::dynamic_pointer_cast<dom::Element>(
              frag.box->node()->parentNode());
          if (parent && parent->localName() == "h1") {
            scale = 2;
          }
        }

        if (frag.box->style().fontSize.unit == css::Length::Unit::Px) {
          canvas.setFont(frag.box->style().fontFamily,
                         static_cast<int>(frag.box->style().fontSize.value));
        } else {
          // Fallback default
          canvas.setFont(frag.box->style().fontFamily, 16 * scale);
        }

        canvas.drawTextVector(fragAbsX, fragAbsY, text, toColor(c));
      }
    }
  }
}

Color PaintingAlgorithm::getColorForBox(layout::LayoutBoxPtr box) {
  if (!box->node())
    return Color::Transparent();
  if (box->node()->nodeType() != dom::NodeType::Element)
    return Color::Transparent();

  auto elem = std::dynamic_pointer_cast<dom::Element>(box->node());
  if (!elem)
    return Color::Transparent();
  if (elem->localName() == "div")
    return {200, 200, 200, 255};
  if (elem->localName() == "p")
    return {200, 200, 255, 255};
  if (elem->localName() == "h1")
    return {255, 200, 200, 255};

  return Color::Transparent();
}

} // namespace renderer
} // namespace xiaopeng
