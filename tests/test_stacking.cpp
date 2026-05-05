#include <iostream>
#include <memory>
#include <vector>
#include <string>

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/layout_algorithm.hpp"
#include "../include/dom/html_types.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;
using namespace xiaopeng::dom;

int main() {
  std::cout << "Testing Stacking Context Implementation..." << std::endl;

  // Test 1: Basic stacking context detection
  std::cout << "\n=== Test 1: Stacking Context Detection ===" << std::endl;
  
  // Root element creates stacking context
  ComputedStyle rootStyle;
  rootStyle.display = Display::Block;
  rootStyle.width = Length::Px(800);
  rootStyle.height = Length::Px(600);
  
  auto root = std::make_shared<LayoutBox>(
    BoxType::BlockNode, rootStyle, nullptr);
  
  // Test position: absolute with z-index
  ComputedStyle absStyle;
  absStyle.display = Display::Block;
  absStyle.position = Position::Absolute;
  absStyle.zIndex = 10;
  absStyle.width = Length::Px(100);
  absStyle.height = Length::Px(100);
  
  auto absBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, absStyle, nullptr);
  root->addChild(absBox);
  
  std::cout << "Root creates stacking context: " << std::boolalpha << root->createsStackingContext() << std::endl;
  std::cout << "Absolute with z-index creates stacking context: " << std::boolalpha << absBox->createsStackingContext() << std::endl;

  // Test 2: opacity creates stacking context
  ComputedStyle opacityStyle;
  opacityStyle.display = Display::Block;
  opacityStyle.opacity = 0.5f;
  opacityStyle.width = Length::Px(100);
  opacityStyle.height = Length::Px(100);
  
  auto opacityBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, opacityStyle, nullptr);
  root->addChild(opacityBox);
  
  std::cout << "Opacity < 1 creates stacking context: " << std::boolalpha << opacityBox->createsStackingContext() << std::endl;

  // Test 3: transform creates stacking context
  ComputedStyle transformStyle;
  transformStyle.display = Display::Block;
  transformStyle.transform = "translateX(10px)";
  transformStyle.width = Length::Px(100);
  transformStyle.height = Length::Px(100);
  
  auto transformBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, transformStyle, nullptr);
  root->addChild(transformBox);
  
  std::cout << "Transform creates stacking context: " << std::boolalpha << transformBox->createsStackingContext() << std::endl;

  // Test 4: Fixed position creates stacking context
  ComputedStyle fixedStyle;
  fixedStyle.display = Display::Block;
  fixedStyle.position = Position::Fixed;
  fixedStyle.width = Length::Px(100);
  fixedStyle.height = Length::Px(100);
  
  auto fixedBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, fixedStyle, nullptr);
  root->addChild(fixedBox);
  
  std::cout << "Fixed position creates stacking context: " << std::boolalpha << fixedBox->createsStackingContext() << std::endl;

  // Test 5: Isolation creates stacking context
  ComputedStyle isolationStyle;
  isolationStyle.display = Display::Block;
  isolationStyle.isolation = Isolation::Isolate;
  isolationStyle.width = Length::Px(100);
  isolationStyle.height = Length::Px(100);
  
  auto isolationBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, isolationStyle, nullptr);
  root->addChild(isolationBox);
  
  std::cout << "Isolation: isolate creates stacking context: " << std::boolalpha << isolationBox->createsStackingContext() << std::endl;

  // Test 6: Stacking level calculation
  std::cout << "\n=== Test 2: Stacking Level Calculation ===" << std::endl;
  
  ComputedStyle level1Style;
  level1Style.display = Display::Block;
  level1Style.position = Position::Relative;
  level1Style.zIndex = 5;
  level1Style.width = Length::Px(50);
  level1Style.height = Length::Px(50);
  
  auto level1Box = std::make_shared<LayoutBox>(
    BoxType::BlockNode, level1Style, nullptr);
  root->addChild(level1Box);
  
  std::cout << "Relative with z-index=5, stacking level: " << level1Box->stackingLevel() << std::endl;

  // Test 7: Find nearest stacking context
  std::cout << "\n=== Test 3: Nearest Stacking Context ===" << std::endl;
  
  auto nestedBox = std::make_shared<LayoutBox>(
    BoxType::BlockNode, level1Style, nullptr);
  level1Box->addChild(nestedBox);
  
  auto nearestSC = nestedBox->nearestStackingContext();
  std::cout << "Nested box nearest stacking context exists: " << std::boolalpha << (nearestSC != nullptr) << std::endl;

  // Test 8: Pseudo-class matching
  std::cout << "\n=== Test 4: Pseudo-class Support ===" << std::endl;
  
  auto elem = std::make_shared<Element>("div");
  elem->setAttribute("class", "test");
  
  std::cout << "Element isFirstChild: " << std::boolalpha << elem->isFirstChild() << std::endl;
  std::cout << "Element isEmpty: " << std::boolalpha << elem->isEmpty() << std::endl;
  std::cout << "Element hasClass('test'): " << std::boolalpha << elem->hasClass("test") << std::endl;
  
  elem->setStateFlag(ElementState::Hovered, true);
  std::cout << "Element isHovered: " << std::boolalpha << elem->isHovered() << std::endl;

  // Test 9: nth-child index calculation
  std::cout << "\n=== Test 5: Nth-child Index ===" << std::endl;
  
  auto parent = std::make_shared<Element>("div");
  auto child1 = std::make_shared<Element>("span");
  auto child2 = std::make_shared<Element>("span");
  auto child3 = std::make_shared<Element>("span");
  
  parent->appendChild(child1);
  parent->appendChild(child2);
  parent->appendChild(child3);
  
  std::cout << "Child 1 index: " << child1->getElementSiblingIndex() << std::endl;
  std::cout << "Child 2 index: " << child2->getElementSiblingIndex() << std::endl;
  std::cout << "Child 3 index: " << child3->getElementSiblingIndex() << std::endl;

  std::cout << "\nAll stacking context tests completed successfully!" << std::endl;
  return 0;
}
