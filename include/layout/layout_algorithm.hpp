#pragma once

#include "layout_box.hpp"
#include "renderer/text_metrics.hpp"
#include "flexbox_algorithm.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace xiaopeng {
namespace layout {

class LayoutAlgorithm {
public:
  void layout(LayoutBoxPtr root, const Dimensions &container) {
    if (!root)
      return;

    // Initial containing block
    root->dimensions().content = container.content;
    root->dimensions().content.x = 0;
    root->dimensions().content.y = 0;

    layoutBox(root, container);
  }

private:
  void layoutBox(LayoutBoxPtr box, const Dimensions &parentDim) {
    // 1. Calculate Width
    calculateBlockWidth(box, parentDim);

    // 2. Calculate Position (Margins, Boarder, Padding)
    // For root, this is 0. For others, it depends on parent type.
    // In this simplified engine, we assume block flow handles positioning.

    // 3. Determine Layout Mode (BFC, IFC, or Flexbox)
    if (box->type() == BoxType::BlockNode ||
        box->type() == BoxType::AnonymousBlock) {
      
      // Check if this is a flex container
      if (box->style().display == css::Display::Flex) {
        // Use Flexbox layout algorithm
        FlexboxAlgorithm flexbox;
        flexbox.layoutFlexContainer(box, parentDim);
      } else if (isInlineFormattingContext(box)) {
        layoutInline(box);
      } else {
        layoutBlock(box);
      }
    } else if (box->type() == BoxType::InlineNode ||
               box->type() == BoxType::AnonymousInline) {
      // Inline nodes are typically handled by their parent's IFC.
      // But if we are here, it might be an inline-block (not supported yet) or
      // just recursion. For now, simple text/inline is handled by parent
      // `layoutInline`.
    }

    // 4. Calculate Height (for non-flex containers)
    if (box->style().display != css::Display::Flex) {
      calculateBlockHeight(box);
    }
  }

  bool isInlineFormattingContext(LayoutBoxPtr box) {
    bool hasInline = false;
    for (auto child : box->children()) {
      if (child->type() == BoxType::BlockNode ||
          child->type() == BoxType::AnonymousBlock) {
        return false; // Contains block, so it's a BFC (mixed content handled by
                      // anon blocks or ignored)
      }
      if (child->type() == BoxType::InlineNode ||
          child->type() == BoxType::AnonymousInline ||
          (child->node() && child->node()->nodeType() == dom::NodeType::Text)) {
        hasInline = true;
      }
    }
    return hasInline;
  }

  void layoutBlock(LayoutBoxPtr box) {
    // Standard Block Layout (Vertical Stacking)
    float currentY = 0;

    for (auto child : box->children()) {
      // Recursively layout child
      layoutBox(child, box->dimensions());

      // Position child's CONTENT box relative to parent's BORDER box
      // In CSS, the offset is: parent_padding + child_margin + child_border +
      // child_padding
      child->dimensions().content.x =
          box->dimensions().padding.left + child->dimensions().margin.left +
          child->dimensions().border.left + child->dimensions().padding.left;

      child->dimensions().content.y = box->dimensions().padding.top + currentY +
                                      child->dimensions().margin.top +
                                      child->dimensions().border.top +
                                      child->dimensions().padding.top;

      // Advance Y by child's full outer height
      currentY +=
          child->dimensions().margin.top + child->dimensions().border.top +
          child->dimensions().padding.top + child->dimensions().content.height +
          child->dimensions().padding.bottom +
          child->dimensions().border.bottom + child->dimensions().margin.bottom;
    }
  }

  void layoutInline(LayoutBoxPtr box) {
    // Inline Formatting Context (Simple Line Breaking)
    box->lineBoxes().clear();

    LineBox currentLine;
    // LineBox y is relative to parent's BORDER box.
    // Base it on this box's content area start.
    currentLine.setY(box->dimensions().padding.top +
                     box->dimensions().border.top);

    float contentWidth = box->dimensions().content.width;
    float currentX = 0;
    float lineHeight = 20.0f; // Default line height approximation reference

    // Flatten inline children into a sequence of items to layout?
    // Or iterate and handle.
    // Simplifying assumption: We iterate direct children.
    // If child is Text -> wrap words.
    // If child is Element -> treat as atomic box for now (simplified).

    for (auto child : box->children()) {
      if (child->node() && child->node()->nodeType() == dom::NodeType::Text) {
        layoutText(box, child, currentLine, currentX, contentWidth, lineHeight);
      } else {
        // layoutInlineElement(box, child, currentLine, currentX, contentWidth);
        // For MVP, treat non-text inlines as 0-size or simple blocks?
        // Let's recursively measure them?
        // Inline elements (span) don't have width/height in same way.
        // We need to recurse into them.
        // But for "Simple Line Breaking", let's assume flat text for now plus
        // simple atomic rendering.
      }
    }

    // Add last line if not empty
    if (!currentLine.fragments().empty()) {
      box->addLineBox(currentLine);
    }
  }

  void layoutText(LayoutBoxPtr parent, LayoutBoxPtr nodeBox,
                  LineBox &currentLine, float &currentX, float maxWidth,
                  float &lineHeight) {
    if (!nodeBox || !nodeBox->node())
      return;
    const std::string &text = nodeBox->node()->textContent();

    // Detect H1 for scaling (must match PaintingAlgorithm)
    float scale = 1.0f;
    if (nodeBox->node() && nodeBox->node()->parentNode() &&
        nodeBox->node()->parentNode()->nodeType() == dom::NodeType::Element) {
      auto parentElem = std::dynamic_pointer_cast<dom::Element>(
          nodeBox->node()->parentNode());
      if (parentElem && parentElem->localName() == "h1") {
        scale = 2.0f;
      }
    }

    // Font metrics
    std::string fontFamily = nodeBox->style().fontFamily;
    int fontSize = static_cast<int>(nodeBox->style().fontSize.value);
    if (fontSize <= 0)
      fontSize = 16;
    fontSize = static_cast<int>(fontSize * scale);

    auto &metrics = renderer::TextMetrics::instance();
    float spaceWidth = metrics.spaceWidth(fontFamily, fontSize);
    lineHeight = metrics.lineHeight(fontFamily, fontSize);
    if (lineHeight <= 0)
      lineHeight = 20.0f * scale;
    currentLine.setHeight(lineHeight);

    size_t pos = 0;
    while (pos < text.length()) {
      // 1. Skip whitespace
      while (pos < text.length() &&
             std::isspace(static_cast<unsigned char>(text[pos]))) {
        pos++;
      }
      if (pos >= text.length())
        break;

      // 2. Find word end
      size_t startOffset = pos;
      while (pos < text.length() &&
             !std::isspace(static_cast<unsigned char>(text[pos]))) {
        pos++;
      }
      size_t endOffset = pos;

      // Extract word for measurement (could be optimized)
      // Length = endOffset - startOffset
      size_t wordLen = endOffset - startOffset;
      float wordWidth = metrics
                            .measureWord(text.substr(startOffset, wordLen),
                                         fontFamily, fontSize)
                            .width;

      // Check if word fits
      if (currentX + wordWidth > maxWidth && currentX > 0) {
        // New Line
        parent->addLineBox(currentLine);
        float prevY = currentLine.y();
        float prevH = currentLine.height();

        currentLine = LineBox();         // Reset
        currentLine.setY(prevY + prevH); // Next line Y
        currentX = 0;
      }

      // Add Fragment with CORRECT offsets
      BoxFragment fragment;
      fragment.box = nodeBox;
      fragment.x = currentX;
      fragment.y = 0; // Relative to LineBox top
      fragment.width = wordWidth;
      fragment.height = static_cast<float>(fontSize); // Font size
      fragment.startOffset = startOffset;
      fragment.endOffset = endOffset;

      currentLine.addFragment(fragment);
      currentLine.setHeight(std::max(currentLine.height(), fragment.height));

      currentX += wordWidth + spaceWidth;
    }
  }

  void calculateBlockWidth(LayoutBoxPtr box, const Dimensions &parentDim) {
    const auto &style = box->style();

    // auto width = parent content width
    float width = style.width.unit == css::Length::Unit::Auto
                      ? parentDim.content.width
                      : style.width.value;

    // Percentage support (basic)
    if (style.width.unit == css::Length::Unit::Percent) {
      width = parentDim.content.width * (style.width.value / 100.0f);
    }

    // Defensive: clamp width to zero to prevent negative dimensions
    box->dimensions().content.width = std::max(0.0f, width);

    // Padding/Margin/Border (simplified copy)
    box->dimensions().padding.left = style.paddingLeft.value;
    box->dimensions().padding.right = style.paddingRight.value;
    box->dimensions().padding.top = style.paddingTop.value;
    box->dimensions().padding.bottom = style.paddingBottom.value;

    // Border width
    box->dimensions().border.left = style.borderLeftWidth.value;
    box->dimensions().border.right = style.borderRightWidth.value;
    box->dimensions().border.top = style.borderTopWidth.value;
    box->dimensions().border.bottom = style.borderBottomWidth.value;

    // Margin handling with auto support
    float marginLeft = style.marginLeft.value;
    float marginRight = style.marginRight.value;

    if (style.display == css::Display::Block &&
        style.width.unit != css::Length::Unit::Auto) {
      // Calculate available space
      float totalWidth = parentDim.content.width;
      float usedWidth = marginLeft + box->dimensions().border.left +
                        box->dimensions().padding.left +
                        box->dimensions().content.width +
                        box->dimensions().padding.right +
                        box->dimensions().border.right + marginRight;

      if (usedWidth < totalWidth) {
        if (style.marginLeft.unit == css::Length::Unit::Auto &&
            style.marginRight.unit == css::Length::Unit::Auto) {
          float remaining = totalWidth - usedWidth;
          marginLeft = remaining / 2.0f;
          marginRight = remaining / 2.0f;
        } else if (style.marginLeft.unit == css::Length::Unit::Auto) {
          marginLeft = totalWidth - usedWidth;
        } else if (style.marginRight.unit == css::Length::Unit::Auto) {
          marginRight = totalWidth - usedWidth;
        }
      }
    }

    box->dimensions().margin.left = marginLeft;
    box->dimensions().margin.right = marginRight;
    box->dimensions().margin.top = style.marginTop.value;
    box->dimensions().margin.bottom = style.marginBottom.value;
  }

  void calculateBlockHeight(LayoutBoxPtr box) {
    const auto &style = box->style();

    if (style.height.unit == css::Length::Unit::Auto) {
      float height = 0;
      if (isInlineFormattingContext(box)) {
        // Height = sum of line boxes
        for (const auto &line : box->lineBoxes()) {
          height += line.height();
        }
        box->dimensions().content.height = height;
      } else {
        // Height = bottom of last child
        for (auto child : box->children()) {
          // child->dimensions().content.y is relative to parent's border box.
          // childBottom is also relative to parent's border box.
          float childBottom = child->dimensions().content.y +
                              child->dimensions().content.height +
                              child->dimensions().padding.bottom +
                              child->dimensions().border.bottom;

          if (childBottom > height)
            height = childBottom;
        }

        // Parent's content height = max(childBottom) - padding.top - border.top
        float contentHeight = height - box->dimensions().padding.top -
                              box->dimensions().border.top;
        if (contentHeight < 0)
          contentHeight = 0;
        box->dimensions().content.height = contentHeight;
      }
    } else {
      box->dimensions().content.height = style.height.value;
    }
  }
};

} // namespace layout
} // namespace xiaopeng
