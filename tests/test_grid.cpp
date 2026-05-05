#include <iostream>
#include <memory>
#include <vector>
#include <string>

// Include necessary headers
#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/grid_algorithm.hpp"
#include "../include/dom/html_types.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;
using namespace xiaopeng::dom;

int main() {
  std::cout << "Testing Grid Layout Implementation..." << std::endl;

  // Test 1: Simple 2x2 grid
  std::cout << "\n=== Test 1: Simple 2x2 Grid ===" << std::endl;
  
  ComputedStyle containerStyle;
  containerStyle.display = Display::Grid;
  containerStyle.width = Length::Px(400);
  containerStyle.height = Length::Px(300);
  containerStyle.gridColumnGap = Length::Px(10);
  containerStyle.gridRowGap = Length::Px(10);
  
  // Create grid template columns and rows
  containerStyle.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));

  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  // Create 4 grid items
  for (int i = 0; i < 4; ++i) {
    ComputedStyle itemStyle;
    itemStyle.gridColumnStart = (i % 2) + 1;
    itemStyle.gridColumnEnd = (i % 2) + 2;
    itemStyle.gridRowStart = (i / 2) + 1;
    itemStyle.gridRowEnd = (i / 2) + 2;

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container->addChild(item);
  }

  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  GridAlgorithm grid;
  grid.layoutGridContainer(container, parentDim);

  std::cout << "Container dimensions: " 
            << container->dimensions().content.width << "x" 
            << container->dimensions().content.height << std::endl;

  for (size_t i = 0; i < container->children().size(); ++i) {
    const auto& child = container->children()[i];
    std::cout << "Item " << i << " position: (" 
              << child->dimensions().content.x << ", " 
              << child->dimensions().content.y << ")" 
              << " size: " << child->dimensions().content.width << "x" 
              << child->dimensions().content.height << std::endl;
  }

  // Test 2: Mixed units (px, fr)
  std::cout << "\n=== Test 2: Mixed Units (px, fr) ===" << std::endl;
  
  ComputedStyle containerStyle2;
  containerStyle2.display = Display::Grid;
  containerStyle2.width = Length::Px(400);
  containerStyle2.height = Length::Px(300);
  containerStyle2.gridColumnGap = Length::Px(10);
  containerStyle2.gridRowGap = Length::Px(10);
  
  containerStyle2.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Px, 100.0f));
  containerStyle2.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 2.0f));
  containerStyle2.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle2.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Px, 80.0f));
  containerStyle2.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Px, 80.0f));

  LayoutBoxPtr container2 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle2, nullptr);

  // Create 6 grid items
  for (int i = 0; i < 6; ++i) {
    ComputedStyle itemStyle;
    itemStyle.gridColumnStart = (i % 3) + 1;
    itemStyle.gridColumnEnd = (i % 3) + 2;
    itemStyle.gridRowStart = (i / 3) + 1;
    itemStyle.gridRowEnd = (i / 3) + 2;

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container2->addChild(item);
  }

  grid.layoutGridContainer(container2, parentDim);

  std::cout << "Container dimensions: " 
            << container2->dimensions().content.width << "x" 
            << container2->dimensions().content.height << std::endl;

  for (size_t i = 0; i < container2->children().size(); ++i) {
    const auto& child = container2->children()[i];
    std::cout << "Item " << i << " position: (" 
              << child->dimensions().content.x << ", " 
              << child->dimensions().content.y << ")" 
              << " size: " << child->dimensions().content.width << "x" 
              << child->dimensions().content.height << std::endl;
  }

  // Test 3: Spanning items
  std::cout << "\n=== Test 3: Spanning Items ===" << std::endl;
  
  ComputedStyle containerStyle3;
  containerStyle3.display = Display::Grid;
  containerStyle3.width = Length::Px(400);
  containerStyle3.height = Length::Px(300);
  containerStyle3.gridColumnGap = Length::Px(10);
  containerStyle3.gridRowGap = Length::Px(10);
  
  containerStyle3.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle3.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle3.gridTemplateColumns.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle3.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));
  containerStyle3.gridTemplateRows.push_back(TrackSize(TrackSize::Type::Fr, 1.0f));

  LayoutBoxPtr container3 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle3, nullptr);

  // Item 0: spans 2 columns
  ComputedStyle itemStyle0;
  itemStyle0.gridColumnStart = 1;
  itemStyle0.gridColumnEnd = 3;
  itemStyle0.gridRowStart = 1;
  itemStyle0.gridRowEnd = 2;
  LayoutBoxPtr item0 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, itemStyle0, nullptr);
  container3->addChild(item0);

  // Item 1: spans 2 rows
  ComputedStyle itemStyle1;
  itemStyle1.gridColumnStart = 3;
  itemStyle1.gridColumnEnd = 4;
  itemStyle1.gridRowStart = 1;
  itemStyle1.gridRowEnd = 3;
  LayoutBoxPtr item1 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, itemStyle1, nullptr);
  container3->addChild(item1);

  // Items 2-3: normal items
  for (int i = 0; i < 2; ++i) {
    ComputedStyle itemStyle;
    itemStyle.gridColumnStart = i + 1;
    itemStyle.gridColumnEnd = i + 2;
    itemStyle.gridRowStart = 2;
    itemStyle.gridRowEnd = 3;

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container3->addChild(item);
  }

  grid.layoutGridContainer(container3, parentDim);

  std::cout << "Container dimensions: " 
            << container3->dimensions().content.width << "x" 
            << container3->dimensions().content.height << std::endl;

  for (size_t i = 0; i < container3->children().size(); ++i) {
    const auto& child = container3->children()[i];
    std::cout << "Item " << i << " position: (" 
              << child->dimensions().content.x << ", " 
              << child->dimensions().content.y << ")" 
              << " size: " << child->dimensions().content.width << "x" 
              << child->dimensions().content.height << std::endl;
  }

  std::cout << "\nGrid test completed successfully!" << std::endl;
  return 0;
}
