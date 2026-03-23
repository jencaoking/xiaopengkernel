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

int main() {
  std::cout << "Testing Flexbox Implementation..." << std::endl;

  // Test 1: Create a flex container
  ComputedStyle containerStyle;
  containerStyle.display = Display::Flex;
  containerStyle.width = Length::Px(500);
  containerStyle.height = Length::Px(300);
  containerStyle.flexDirection = FlexDirection::Row;
  containerStyle.justifyContent = JustifyContent::SpaceBetween;

  // Create a flex container box
  LayoutBoxPtr container = std::make_shared<LayoutBox>(
    BoxType::BlockNode, containerStyle, nullptr);

  // Test 2: Create flex items
  ComputedStyle itemStyle1;
  itemStyle1.width = Length::Px(100);
  itemStyle1.height = Length::Px(50);
  itemStyle1.flexGrow = Length::Px(1);

  ComputedStyle itemStyle2;
  itemStyle2.width = Length::Px(150);
  itemStyle2.height = Length::Px(50);
  itemStyle2.flexGrow = Length::Px(2);

  ComputedStyle itemStyle3;
  itemStyle3.width = Length::Px(80);
  itemStyle3.height = Length::Px(50);
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

  // Test 3: Run flexbox layout
  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  flexbox.layoutFlexContainer(container, parentDim);

  // Test 4: Verify results
  std::cout << "Container dimensions: " 
            << container->dimensions().content.width << "x" 
            << container->dimensions().content.height << std::endl;

  std::cout << "Item 1 position: (" 
            << item1->dimensions().content.x << ", " 
            << item1->dimensions().content.y << ")" 
            << " size: " << item1->dimensions().content.width << "x" 
            << item1->dimensions().content.height << std::endl;

  std::cout << "Item 2 position: (" 
            << item2->dimensions().content.x << ", " 
            << item2->dimensions().content.y << ")" 
            << " size: " << item2->dimensions().content.width << "x" 
            << item2->dimensions().content.height << std::endl;

  std::cout << "Item 3 position: (" 
            << item3->dimensions().content.x << ", " 
            << item3->dimensions().content.y << ")" 
            << " size: " << item3->dimensions().content.width << "x" 
            << item3->dimensions().content.height << std::endl;

  // Test 5: Test column layout
  std::cout << "\nTesting Column Layout..." << std::endl;
  
  ComputedStyle columnContainerStyle;
  columnContainerStyle.display = Display::Flex;
  columnContainerStyle.width = Length::Px(300);
  columnContainerStyle.height = Length::Px(400);
  columnContainerStyle.flexDirection = FlexDirection::Column;
  columnContainerStyle.justifyContent = JustifyContent::Center;

  LayoutBoxPtr columnContainer = std::make_shared<LayoutBox>(
    BoxType::BlockNode, columnContainerStyle, nullptr);

  ComputedStyle columnItemStyle;
  columnItemStyle.width = Length::Px(100);
  columnItemStyle.height = Length::Px(40);
  columnItemStyle.flexGrow = Length::Px(1);

  LayoutBoxPtr columnItem1 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, columnItemStyle, nullptr);
  LayoutBoxPtr columnItem2 = std::make_shared<LayoutBox>(
    BoxType::BlockNode, columnItemStyle, nullptr);

  columnContainer->addChild(columnItem1);
  columnContainer->addChild(columnItem2);

  flexbox.layoutFlexContainer(columnContainer, parentDim);

  std::cout << "Column container dimensions: " 
            << columnContainer->dimensions().content.width << "x" 
            << columnContainer->dimensions().content.height << std::endl;

  std::cout << "Column item 1 position: (" 
            << columnItem1->dimensions().content.x << ", " 
            << columnItem1->dimensions().content.y << ")" 
            << " size: " << columnItem1->dimensions().content.width << "x" 
            << columnItem1->dimensions().content.height << std::endl;

  std::cout << "Column item 2 position: (" 
            << columnItem2->dimensions().content.x << ", " 
            << columnItem2->dimensions().content.y << ")" 
            << " size: " << columnItem2->dimensions().content.width << "x" 
            << columnItem2->dimensions().content.height << std::endl;

  std::cout << "\nFlexbox test completed successfully!" << std::endl;
  return 0;
}