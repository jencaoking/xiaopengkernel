#include <iostream>
#include <memory>
#include <vector>
#include <string>

// Include necessary headers
#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/flexbox_algorithm.hpp"
#include "../include/dom/html_types.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;
using namespace xiaopeng::dom;

void testFlexWrap() {
  std::cout << "\n=== Testing Flex Wrap ===" << std::endl;
  
  ComputedStyle containerStyle;
  containerStyle.display = Display::Flex;
  containerStyle.width = Length::Px(300);
  containerStyle.height = Length::Px(200);
  containerStyle.flexDirection = FlexDirection::Row;
  containerStyle.flexWrap = FlexWrap::Wrap;
  containerStyle.justifyContent = JustifyContent::FlexStart;

  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  // Create items that should wrap
  for (int i = 0; i < 5; i++) {
    ComputedStyle itemStyle;
    itemStyle.width = Length::Px(100);
    itemStyle.height = Length::Px(40);
    itemStyle.flexGrow = Length::Px(0);

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container->addChild(item);
  }

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "Container dimensions: " 
            << container->dimensions().content.width << "x" 
            << container->dimensions().content.height << std::endl;

  for (size_t i = 0; i < container->children().size(); i++) {
    auto child = container->children()[i];
    std::cout << "Item " << i << " position: (" 
              << child->dimensions().content.x << ", " 
              << child->dimensions().content.y << ")" 
              << " size: " << child->dimensions().content.width << "x" 
              << child->dimensions().content.height << std::endl;
  }
}

void testJustifyContent() {
  std::cout << "\n=== Testing Justify Content ===" << std::endl;
  
  // Test SpaceBetween
  ComputedStyle containerStyle;
  containerStyle.display = Display::Flex;
  containerStyle.width = Length::Px(500);
  containerStyle.height = Length::Px(100);
  containerStyle.flexDirection = FlexDirection::Row;
  containerStyle.justifyContent = JustifyContent::SpaceBetween;

  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  for (int i = 0; i < 3; i++) {
    ComputedStyle itemStyle;
    itemStyle.width = Length::Px(80);
    itemStyle.height = Length::Px(40);
    itemStyle.flexGrow = Length::Px(0);

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container->addChild(item);
  }

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "SpaceBetween test:" << std::endl;
  for (size_t i = 0; i < container->children().size(); i++) {
    auto child = container->children()[i];
    std::cout << "Item " << i << " x: " << child->dimensions().content.x << std::endl;
  }

  // Test Center
  containerStyle.justifyContent = JustifyContent::Center;
  container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  for (int i = 0; i < 3; i++) {
    ComputedStyle itemStyle;
    itemStyle.width = Length::Px(80);
    itemStyle.height = Length::Px(40);
    itemStyle.flexGrow = Length::Px(0);

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container->addChild(item);
  }

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "Center test:" << std::endl;
  for (size_t i = 0; i < container->children().size(); i++) {
    auto child = container->children()[i];
    std::cout << "Item " << i << " x: " << child->dimensions().content.x << std::endl;
  }
}

void testAlignItems() {
  std::cout << "\n=== Testing Align Items ===" << std::endl;
  
  ComputedStyle containerStyle;
  containerStyle.display = Display::Flex;
  containerStyle.width = Length::Px(500);
  containerStyle.height = Length::Px(200);
  containerStyle.flexDirection = FlexDirection::Row;
  containerStyle.alignItems = AlignItems::Center;

  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  for (int i = 0; i < 3; i++) {
    ComputedStyle itemStyle;
    itemStyle.width = Length::Px(80);
    itemStyle.height = Length::Px(40 + i * 20); // Different heights
    itemStyle.flexGrow = Length::Px(0);

    LayoutBoxPtr item = std::make_shared<LayoutBox>(
      BoxType::BlockNode, itemStyle, nullptr);
    container->addChild(item);
  }

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "AlignItems Center test:" << std::endl;
  for (size_t i = 0; i < container->children().size(); i++) {
    auto child = container->children()[i];
    std::cout << "Item " << i << " y: " << child->dimensions().content.y 
              << " height: " << child->dimensions().content.height << std::endl;
  }
}

void testFlexGrow() {
  std::cout << "\n=== Testing Flex Grow ===" << std::endl;
  
  ComputedStyle containerStyle;
  containerStyle.display = Display::Flex;
  containerStyle.width = Length::Px(500);
  containerStyle.height = Length::Px(100);
  containerStyle.flexDirection = FlexDirection::Row;
  containerStyle.justifyContent = JustifyContent::FlexStart;

  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  // Create items with different flex-grow values
  ComputedStyle itemStyle1;
  itemStyle1.width = Length::Px(100);
  itemStyle1.height = Length::Px(40);
  itemStyle1.flexGrow = Length::Px(1);

  ComputedStyle itemStyle2;
  itemStyle2.width = Length::Px(100);
  itemStyle2.height = Length::Px(40);
  itemStyle2.flexGrow = Length::Px(2);

  ComputedStyle itemStyle3;
  itemStyle3.width = Length::Px(100);
  itemStyle3.height = Length::Px(40);
  itemStyle3.flexGrow = Length::Px(0);

  LayoutBoxPtr item1 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, itemStyle1, nullptr);
  LayoutBoxPtr item2 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, itemStyle2, nullptr);
  LayoutBoxPtr item3 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, itemStyle3, nullptr);

  container->addChild(item1);
  container->addChild(item2);
  container->addChild(item3);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "FlexGrow test:" << std::endl;
  std::cout << "Total width: " << container->dimensions().content.width << std::endl;
  std::cout << "Item 1 width: " << item1->dimensions().content.width 
            << " (flex-grow: 1)" << std::endl;
  std::cout << "Item 2 width: " << item2->dimensions().content.width 
            << " (flex-grow: 2)" << std::endl;
  std::cout << "Item 3 width: " << item3->dimensions().content.width 
            << " (flex-grow: 0)" << std::endl;
}

int main() {
  std::cout << "Advanced Flexbox Tests" << std::endl;
  
  testFlexWrap();
  testJustifyContent();
  testAlignItems();
  testFlexGrow();
  
  std::cout << "\nAll advanced tests completed!" << std::endl;
  return 0;
}