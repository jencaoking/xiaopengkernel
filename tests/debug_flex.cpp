#include <iostream>
#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/flexbox_algorithm.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

int main() {
  ComputedStyle style;
  style.display = Display::Flex;
  style.flexDirection = FlexDirection::Column;
  style.justifyContent = JustifyContent::FlexStart;
  style.width = Length::Px(800);
  style.height = Length::Px(600);
  auto container = std::make_shared<LayoutBox>(BoxType::BlockNode, style, nullptr);

  ComputedStyle s1;
  s1.width = Length::Px(100);
  s1.height = Length::Px(40);
  auto item1 = std::make_shared<LayoutBox>(BoxType::BlockNode, s1, nullptr);

  ComputedStyle s2;
  s2.width = Length::Px(100);
  s2.height = Length::Px(40);
  auto item2 = std::make_shared<LayoutBox>(BoxType::BlockNode, s2, nullptr);

  container->addChild(item1);
  container->addChild(item2);

  FlexboxAlgorithm flexbox;
  Dimensions parentDim;
  parentDim.content.width = 800;
  parentDim.content.height = 600;

  std::cout << "BEFORE layout:\n";
  std::cout << "  container.content = {x=" << container->dimensions().content.x
            << ", y=" << container->dimensions().content.y
            << ", w=" << container->dimensions().content.width
            << ", h=" << container->dimensions().content.height << "}\n";
  std::cout << "  container.padding.top = " << container->dimensions().padding.top << "\n";
  std::cout << "  parentDim.content.y = " << parentDim.content.y << "\n";

  flexbox.layoutFlexContainer(container, parentDim);

  std::cout << "AFTER layout:\n";
  std::cout << "  container.content = {x=" << container->dimensions().content.x
            << ", y=" << container->dimensions().content.y
            << ", w=" << container->dimensions().content.width
            << ", h=" << container->dimensions().content.height << "}\n";
  std::cout << "  container.padding.top = " << container->dimensions().padding.top << "\n";
  std::cout << "  item1.content = {x=" << item1->dimensions().content.x
            << ", y=" << item1->dimensions().content.y
            << ", w=" << item1->dimensions().content.width
            << ", h=" << item1->dimensions().content.height << "}\n";
  std::cout << "  item2.content = {x=" << item2->dimensions().content.x
            << ", y=" << item2->dimensions().content.y
            << ", w=" << item2->dimensions().content.width
            << ", h=" << item2->dimensions().content.height << "}\n";
  return 0;
}
