#pragma once

#include "../css/computed_style.hpp"
#include "layout_box.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace xiaopeng {
namespace layout {

// Flex item structure for layout calculation
struct FlexItem {
  LayoutBoxPtr box;
  float flexGrow = 0.0f;
  float flexShrink = 1.0f;
  float flexBasis = 0.0f;
  float size = 0.0f;      // Main axis size
  float crossSize = 0.0f; // Cross axis size
  bool isAuto = false;
};

class FlexboxAlgorithm {
public:
  void layoutFlexContainer(LayoutBoxPtr container,
                           const Dimensions &parentDim) {
    if (!container || container->type() != BoxType::BlockNode) {
      return;
    }

    const auto &style = container->style();
    if (style.display != css::Display::Flex) {
      return;
    }

    // 1. Calculate container dimensions
    calculateFlexContainerDimensions(container, parentDim);

    // 2. Collect flex items
    std::vector<FlexItem> items = collectFlexItems(container);

    // 3. Determine main and cross axes
    bool isRow = (style.flexDirection == css::FlexDirection::Row ||
                  style.flexDirection == css::FlexDirection::RowReverse);
    bool isReverse = (style.flexDirection == css::FlexDirection::RowReverse ||
                      style.flexDirection == css::FlexDirection::ColumnReverse);

    // 4. Measure flex items
    measureFlexItems(items, container, isRow);

    // 5. Calculate flex lines
    std::vector<std::vector<FlexItem>> lines =
        calculateFlexLines(items, container, isRow);

    // 6. Layout each flex line
    for (auto &line : lines) {
      layoutFlexLine(line, container, isRow, isReverse);
    }

    // 7. Calculate container height if auto
    if (container->style().height.unit == css::Length::Unit::Auto) {
      calculateContainerHeight(container, lines, isRow);
    }
  }

private:
  void calculateFlexContainerDimensions(LayoutBoxPtr container,
                                        const Dimensions &parentDim) {
    const auto &style = container->style();

    // Width calculation
    float width = style.width.unit == css::Length::Unit::Auto
                      ? parentDim.content.width
                      : style.width.value;

    if (style.width.unit == css::Length::Unit::Percent) {
      width = parentDim.content.width * (style.width.value / 100.0f);
    }

    container->dimensions().content.width = std::max(0.0f, width);

    // Height calculation (auto initially, will be calculated later)
    if (style.height.unit == css::Length::Unit::Auto) {
      container->dimensions().content.height = 0; // Will be calculated later
    } else {
      container->dimensions().content.height = style.height.value;
    }

    // Copy padding/border/margin
    container->dimensions().padding.left = style.paddingLeft.value;
    container->dimensions().padding.right = style.paddingRight.value;
    container->dimensions().padding.top = style.paddingTop.value;
    container->dimensions().padding.bottom = style.paddingBottom.value;

    container->dimensions().border.left = style.borderLeftWidth.value;
    container->dimensions().border.right = style.borderRightWidth.value;
    container->dimensions().border.top = style.borderTopWidth.value;
    container->dimensions().border.bottom = style.borderBottomWidth.value;

    container->dimensions().margin.left = style.marginLeft.value;
    container->dimensions().margin.right = style.marginRight.value;
    container->dimensions().margin.top = style.marginTop.value;
    container->dimensions().margin.bottom = style.marginBottom.value;
  }

  std::vector<FlexItem> collectFlexItems(LayoutBoxPtr container) {
    std::vector<FlexItem> items;

    for (auto child : container->children()) {
      if (child->type() == BoxType::BlockNode) {
        const auto &childStyle = child->style();

        // Only process flex items (children of flex container)
        FlexItem item;
        item.box = child;

        // Get flex properties
        if (childStyle.flexGrow.unit == css::Length::Unit::Px) {
          item.flexGrow = childStyle.flexGrow.value;
        }
        if (childStyle.flexShrink.unit == css::Length::Unit::Px) {
          item.flexShrink = childStyle.flexShrink.value;
        }

        // Handle flex-basis
        if (childStyle.flexBasis.unit == css::Length::Unit::Auto) {
          item.isAuto = true;
          item.flexBasis = 0.0f;
        } else if (childStyle.flexBasis.unit == css::Length::Unit::Percent) {
          // Will be calculated based on container size
          item.flexBasis = 0.0f;
          item.isAuto = true;
        } else {
          item.flexBasis = childStyle.flexBasis.value;
          item.isAuto = false;
        }

        items.push_back(item);
      }
    }

    return items;
  }

  void measureFlexItems(std::vector<FlexItem> &items, LayoutBoxPtr container,
                        bool isRow) {
    float containerMainSize = isRow ? container->dimensions().content.width
                                    : container->dimensions().content.height;
    (void)containerMainSize;

    for (auto &item : items) {
      const auto &style = item.box->style();

      // Calculate main axis size
      if (item.isAuto) {
        // Auto basis: use content size or specified width/height
        if (isRow) {
          if (style.width.unit == css::Length::Unit::Auto) {
            // Use content width (simplified: use padding + border)
            item.size = style.paddingLeft.value + style.paddingRight.value +
                        style.borderLeftWidth.value +
                        style.borderRightWidth.value;
          } else {
            item.size = style.width.value;
          }
        } else {
          if (style.height.unit == css::Length::Unit::Auto) {
            // Use content height
            item.size = style.paddingTop.value + style.paddingBottom.value +
                        style.borderTopWidth.value +
                        style.borderBottomWidth.value;
          } else {
            item.size = style.height.value;
          }
        }
      } else {
        item.size = item.flexBasis;
      }

      // Calculate cross axis size
      if (isRow) {
        if (style.height.unit == css::Length::Unit::Auto) {
          item.crossSize = style.paddingTop.value + style.paddingBottom.value +
                           style.borderTopWidth.value +
                           style.borderBottomWidth.value;
        } else {
          item.crossSize = style.height.value;
        }
      } else {
        if (style.width.unit == css::Length::Unit::Auto) {
          item.crossSize = style.paddingLeft.value + style.paddingRight.value +
                           style.borderLeftWidth.value +
                           style.borderRightWidth.value;
        } else {
          item.crossSize = style.width.value;
        }
      }

      // Update the box dimensions with the calculated size
      if (isRow) {
        item.box->dimensions().content.width = item.size;
        item.box->dimensions().content.height = item.crossSize;
      } else {
        item.box->dimensions().content.width = item.crossSize;
        item.box->dimensions().content.height = item.size;
      }
    }
  }

  std::vector<std::vector<FlexItem>>
  calculateFlexLines(std::vector<FlexItem> &items, LayoutBoxPtr container,
                     bool isRow) {

    std::vector<std::vector<FlexItem>> lines;
    std::vector<FlexItem> currentLine;

    float containerMainSize = isRow ? container->dimensions().content.width
                                    : container->dimensions().content.height;

    float currentLineSize = 0.0f;
    const auto &style = container->style();

    for (auto &item : items) {
      // Check if we need to wrap
      if (style.flexWrap == css::FlexWrap::Wrap &&
          currentLineSize + item.size > containerMainSize &&
          !currentLine.empty()) {
        // Start new line
        lines.push_back(currentLine);
        currentLine.clear();
        currentLineSize = 0.0f;
      }

      currentLine.push_back(item);
      currentLineSize += item.size;
    }

    // Add last line
    if (!currentLine.empty()) {
      lines.push_back(currentLine);
    }

    return lines;
  }

  void layoutFlexLine(std::vector<FlexItem> &line, LayoutBoxPtr container,
                      bool isRow, bool isReverse) {

    float containerMainSize = isRow ? container->dimensions().content.width
                                    : container->dimensions().content.height;

    // Calculate total flex grow and shrink
    float totalFlexGrow = 0.0f;
    float totalFlexShrink = 0.0f;
    float totalSize = 0.0f;

    for (const auto &item : line) {
      totalFlexGrow += item.flexGrow;
      totalFlexShrink += item.flexShrink;
      totalSize += item.size;
    }

    // Calculate available space
    float availableSpace = containerMainSize - totalSize;

    // Distribute space based on flex factors
    if (availableSpace > 0 && totalFlexGrow > 0) {
      // Grow items
      for (auto &item : line) {
        if (item.flexGrow > 0) {
          float growAmount = (item.flexGrow / totalFlexGrow) * availableSpace;
          item.size += growAmount;
        }
      }
    } else if (availableSpace < 0 && totalFlexShrink > 0) {
      // Shrink items
      for (auto &item : line) {
        if (item.flexShrink > 0) {
          float shrinkAmount =
              (item.flexShrink / totalFlexShrink) * availableSpace;
          item.size += shrinkAmount; // availableSpace is negative
        }
      }
    }

    // Calculate positions
    float currentPos = 0.0f;
    if (isReverse) {
      currentPos = containerMainSize;
    }

    // Justify content
    const auto &containerStyle = container->style();
    float totalLineSize = 0.0f;
    for (const auto &item : line) {
      totalLineSize += item.size;
    }

    float justifyOffset = 0.0f;
    float justifySpacing = 0.0f;

    switch (containerStyle.justifyContent) {
    case css::JustifyContent::FlexStart:
      justifyOffset = 0.0f;
      break;
    case css::JustifyContent::FlexEnd:
      justifyOffset = containerMainSize - totalLineSize;
      break;
    case css::JustifyContent::Center:
      justifyOffset = (containerMainSize - totalLineSize) / 2.0f;
      break;
    case css::JustifyContent::SpaceBetween:
      if (line.size() > 1) {
        justifySpacing =
            (containerMainSize - totalLineSize) / (line.size() - 1);
      }
      break;
    case css::JustifyContent::SpaceAround:
      justifySpacing = (containerMainSize - totalLineSize) / line.size();
      justifyOffset = justifySpacing / 2.0f;
      break;
    case css::JustifyContent::SpaceEvenly:
      justifySpacing = (containerMainSize - totalLineSize) / (line.size() + 1);
      justifyOffset = justifySpacing;
      break;
    }

    // Apply justify offset to initial position
    if (!isReverse) {
      currentPos = justifyOffset;
    } else {
      currentPos = containerMainSize - justifyOffset;
    }

    // Position items
    for (auto &item : line) {
      if (isReverse) {
        currentPos -= item.size;
        if (isRow) {
          item.box->dimensions().content.x = currentPos;
        } else {
          item.box->dimensions().content.y = currentPos;
        }
      } else {
        if (isRow) {
          item.box->dimensions().content.x = currentPos;
        } else {
          item.box->dimensions().content.y = currentPos;
        }
        currentPos += item.size;
      }

      // Add justify spacing between items
      if (justifySpacing > 0) {
        currentPos += justifySpacing;
      }
    }

    // Update box dimensions with resolved sizes
    for (auto &item : line) {
      if (isRow) {
        item.box->dimensions().content.width = item.size;
        item.box->dimensions().content.height = item.crossSize;
      } else {
        item.box->dimensions().content.width = item.crossSize;
        item.box->dimensions().content.height = item.size;
      }
      // Copy padding/border/margin from style
      const auto &s = item.box->style();
      item.box->dimensions().padding.left = s.paddingLeft.value;
      item.box->dimensions().padding.right = s.paddingRight.value;
      item.box->dimensions().padding.top = s.paddingTop.value;
      item.box->dimensions().padding.bottom = s.paddingBottom.value;
      item.box->dimensions().border.left = s.borderLeftWidth.value;
      item.box->dimensions().border.right = s.borderRightWidth.value;
      item.box->dimensions().border.top = s.borderTopWidth.value;
      item.box->dimensions().border.bottom = s.borderBottomWidth.value;
      item.box->dimensions().margin.left = s.marginLeft.value;
      item.box->dimensions().margin.right = s.marginRight.value;
      item.box->dimensions().margin.top = s.marginTop.value;
      item.box->dimensions().margin.bottom = s.marginBottom.value;
    }

    // Align items on cross axis
    float maxCrossSize = 0.0f;
    for (const auto &item : line) {
      if (item.crossSize > maxCrossSize) {
        maxCrossSize = item.crossSize;
      }
    }

    float crossStart = 0.0f;
    for (auto &item : line) {
      switch (containerStyle.alignItems) {
      case css::AlignItems::FlexStart:
        crossStart = 0.0f;
        break;
      case css::AlignItems::FlexEnd:
        crossStart = maxCrossSize - item.crossSize;
        break;
      case css::AlignItems::Center:
        crossStart = (maxCrossSize - item.crossSize) / 2.0f;
        break;
      case css::AlignItems::Stretch:
        // Stretch items to fill cross axis
        item.crossSize = maxCrossSize;
        crossStart = 0.0f;
        break;
      case css::AlignItems::Baseline:
        // Simplified: treat as flex-start
        crossStart = 0.0f;
        break;
      }

      // Apply cross axis position
      if (isRow) {
        item.box->dimensions().content.y = crossStart;
      } else {
        item.box->dimensions().content.x = crossStart;
      }
    }
  }

  void calculateContainerHeight(LayoutBoxPtr container,
                                const std::vector<std::vector<FlexItem>> &lines,
                                bool isRow) {
    if (!isRow)
      return; // For column layout, height is already calculated

    float totalHeight = 0.0f;
    for (const auto &line : lines) {
      float maxCrossSize = 0.0f;
      for (const auto &item : line) {
        if (item.crossSize > maxCrossSize) {
          maxCrossSize = item.crossSize;
        }
      }
      totalHeight += maxCrossSize;
    }

    container->dimensions().content.height = totalHeight;
  }
};

} // namespace layout
} // namespace xiaopeng