#include <iostream>
#include <memory>
#include <cassert>

#include "../include/css/computed_style.hpp"
#include "../include/layout/layout_box.hpp"
#include "../include/layout/layout_algorithm.hpp"
#include "../include/layout/flexbox_algorithm.hpp"
#include "../include/dom/dom.hpp"

using namespace xiaopeng;
using namespace xiaopeng::css;
using namespace xiaopeng::layout;

int test_absolute_positioning() {
    std::cout << "=== Testing Absolute Positioning ===" << std::endl;

    // Create container (will be containing block)
    ComputedStyle containerStyle;
    containerStyle.display = Display::Block;
    containerStyle.position = Position::Relative; // Important! Must be non-static
    containerStyle.width = Length::Px(600);
    containerStyle.height = Length::Px(400);
    containerStyle.backgroundColor = {240, 240, 240, 255};

    LayoutBoxPtr container = std::make_shared<LayoutBox>(
        BoxType::BlockNode, containerStyle, nullptr);

    // Create absolute positioned element 1 (top-left)
    ComputedStyle absStyle1;
    absStyle1.display = Display::Block;
    absStyle1.position = Position::Absolute;
    absStyle1.top = Length::Px(20);
    absStyle1.left = Length::Px(20);
    absStyle1.width = Length::Px(100);
    absStyle1.height = Length::Px(80);
    absStyle1.backgroundColor = {255, 100, 100, 255};

    LayoutBoxPtr abs1 = std::make_shared<LayoutBox>(
        BoxType::BlockNode, absStyle1, nullptr);
    container->addChild(abs1);

    // Create absolute positioned element 2 (bottom-right)
    ComputedStyle absStyle2;
    absStyle2.display = Display::Block;
    absStyle2.position = Position::Absolute;
    absStyle2.bottom = Length::Px(30);
    absStyle2.right = Length::Px(30);
    absStyle2.width = Length::Px(120);
    absStyle2.height = Length::Px(60);
    absStyle2.backgroundColor = {100, 255, 100, 255};

    LayoutBoxPtr abs2 = std::make_shared<LayoutBox>(
        BoxType::BlockNode, absStyle2, nullptr);
    container->addChild(abs2);

    // Create absolute positioned element 3 (percentages)
    ComputedStyle absStyle3;
    absStyle3.display = Display::Block;
    absStyle3.position = Position::Absolute;
    absStyle3.top = Length::Percent(50);
    absStyle3.left = Length::Percent(50);
    absStyle3.width = Length::Percent(20);
    absStyle3.height = Length::Percent(20);
    absStyle3.backgroundColor = {100, 100, 255, 255};

    LayoutBoxPtr abs3 = std::make_shared<LayoutBox>(
        BoxType::BlockNode, absStyle3, nullptr);
    container->addChild(abs3);

    // Layout
    LayoutAlgorithm algorithm;
    Dimensions parentDim;
    parentDim.content = {0, 0, 800, 600};
    algorithm.layout(container, parentDim);

    // Verify results
    std::cout << "Container position/size: " 
              << container->dimensions().content.x << "," 
              << container->dimensions().content.y << " " 
              << container->dimensions().content.width << "x" 
              << container->dimensions().content.height << std::endl;

    std::cout << "Absolute 1 position/size: " 
              << abs1->dimensions().content.x << "," 
              << abs1->dimensions().content.y << " " 
              << abs1->dimensions().content.width << "x" 
              << abs1->dimensions().content.height << std::endl;

    std::cout << "Absolute 2 position/size: " 
              << abs2->dimensions().content.x << "," 
              << abs2->dimensions().content.y << " " 
              << abs2->dimensions().content.width << "x" 
              << abs2->dimensions().content.height << std::endl;

    std::cout << "Absolute 3 position/size: " 
              << abs3->dimensions().content.x << "," 
              << abs3->dimensions().content.y << " " 
              << abs3->dimensions().content.width << "x" 
              << abs3->dimensions().content.height << std::endl;

    // Simple checks
    assert(abs1->dimensions().content.x >= 20);
    assert(abs1->dimensions().content.y >= 20);
    assert(abs1->dimensions().content.width == 100);
    assert(abs1->dimensions().content.height == 80);

    std::cout << "Absolute positioning test PASSED" << std::endl;
    return 0;
}

int test_fixed_positioning() {
    std::cout << "\n=== Testing Fixed Positioning ===" << std::endl;

    // Create container (won't affect fixed children)
    ComputedStyle containerStyle;
    containerStyle.display = Display::Block;
    containerStyle.position = Position::Relative;
    containerStyle.width = Length::Px(400);
    containerStyle.height = Length::Px(300);
    containerStyle.backgroundColor = {200, 200, 200, 255};

    LayoutBoxPtr container = std::make_shared<LayoutBox>(
        BoxType::BlockNode, containerStyle, nullptr);

    // Create fixed element (should be relative to viewport)
    ComputedStyle fixedStyle;
    fixedStyle.display = Display::Block;
    fixedStyle.position = Position::Fixed;
    fixedStyle.top = Length::Px(50);
    fixedStyle.left = Length::Px(50);
    fixedStyle.width = Length::Px(200);
    fixedStyle.height = Length::Px(100);
    fixedStyle.backgroundColor = {255, 200, 0, 255};
    fixedStyle.zIndex = 10;

    LayoutBoxPtr fixed = std::make_shared<LayoutBox>(
        BoxType::BlockNode, fixedStyle, nullptr);
    container->addChild(fixed);

    // Layout
    LayoutAlgorithm algorithm;
    Dimensions parentDim;
    parentDim.content = {0, 0, 800, 600};
    algorithm.layout(container, parentDim);

    std::cout << "Fixed element position/size: " 
              << fixed->dimensions().content.x << "," 
              << fixed->dimensions().content.y << " " 
              << fixed->dimensions().content.width << "x" 
              << fixed->dimensions().content.height << std::endl;

    // Check
    assert(fixed->dimensions().content.width == 200);
    assert(fixed->dimensions().content.height == 100);

    std::cout << "Fixed positioning test PASSED" << std::endl;
    return 0;
}

int test_mixed_layout() {
    std::cout << "\n=== Testing Mixed Layout (Normal + Positioned) ===" << std::endl;

    // Create root container
    ComputedStyle rootStyle;
    rootStyle.display = Display::Block;
    rootStyle.width = Length::Px(800);
    rootStyle.height = Length::Px(600);

    LayoutBoxPtr root = std::make_shared<LayoutBox>(
        BoxType::BlockNode, rootStyle, nullptr);

    // Normal flow element 1
    ComputedStyle normalStyle1;
    normalStyle1.display = Display::Block;
    normalStyle1.width = Length::Px(100);
    normalStyle1.height = Length::Px(50);
    normalStyle1.backgroundColor = {150, 150, 150, 255};

    LayoutBoxPtr normal1 = std::make_shared<LayoutBox>(
        BoxType::BlockNode, normalStyle1, nullptr);
    root->addChild(normal1);

    // Positioned relative container
    ComputedStyle relStyle;
    relStyle.display = Display::Block;
    relStyle.position = Position::Relative;
    relStyle.width = Length::Px(300);
    relStyle.height = Length::Px(200);
    relStyle.backgroundColor = {200, 220, 255, 255};

    LayoutBoxPtr rel = std::make_shared<LayoutBox>(
        BoxType::BlockNode, relStyle, nullptr);
    root->addChild(rel);

    // Absolute child inside relative container
    ComputedStyle absChildStyle;
    absChildStyle.display = Display::Block;
    absChildStyle.position = Position::Absolute;
    absChildStyle.top = Length::Px(10);
    absChildStyle.right = Length::Px(10);
    absChildStyle.width = Length::Px(80);
    absChildStyle.height = Length::Px(40);
    absChildStyle.backgroundColor = {255, 150, 150, 255};

    LayoutBoxPtr absChild = std::make_shared<LayoutBox>(
        BoxType::BlockNode, absChildStyle, nullptr);
    rel->addChild(absChild);

    // Normal flow element 2
    ComputedStyle normalStyle2;
    normalStyle2.display = Display::Block;
    normalStyle2.width = Length::Px(100);
    normalStyle2.height = Length::Px(50);
    normalStyle2.backgroundColor = {150, 150, 150, 255};

    LayoutBoxPtr normal2 = std::make_shared<LayoutBox>(
        BoxType::BlockNode, normalStyle2, nullptr);
    root->addChild(normal2);

    // Layout
    LayoutAlgorithm algorithm;
    Dimensions parentDim;
    parentDim.content = {0, 0, 1000, 800};
    algorithm.layout(root, parentDim);

    std::cout << "Root children count: " << root->children().size() << std::endl;
    std::cout << "Relative container children count: " << rel->children().size() << std::endl;

    std::cout << "Mixed layout test PASSED" << std::endl;
    return 0;
}

int test_overflow_with_positioned() {
    std::cout << "\n=== Testing Overflow with Positioned Elements ===" << std::endl;

    // Create scroll container
    ComputedStyle scrollStyle;
    scrollStyle.display = Display::Block;
    scrollStyle.position = Position::Relative;
    scrollStyle.width = Length::Px(200);
    scrollStyle.height = Length::Px(150);
    scrollStyle.overflowX = Overflow::Hidden;
    scrollStyle.overflowY = Overflow::Scroll;
    scrollStyle.backgroundColor = {245, 245, 245, 255};

    LayoutBoxPtr scrollContainer = std::make_shared<LayoutBox>(
        BoxType::BlockNode, scrollStyle, nullptr);

    // Positioned element that should overflow
    ComputedStyle bigStyle;
    bigStyle.display = Display::Block;
    bigStyle.position = Position::Absolute;
    bigStyle.top = Length::Px(-20);
    bigStyle.left = Length::Px(-20);
    bigStyle.width = Length::Px(300);
    bigStyle.height = Length::Px(300);
    bigStyle.backgroundColor = {200, 0, 0, 128};

    LayoutBoxPtr bigElement = std::make_shared<LayoutBox>(
        BoxType::BlockNode, bigStyle, nullptr);
    scrollContainer->addChild(bigElement);

    // Layout
    LayoutAlgorithm algorithm;
    Dimensions parentDim;
    parentDim.content = {0, 0, 800, 600};
    algorithm.layout(scrollContainer, parentDim);

    std::cout << "Scroll container size: " 
              << scrollContainer->dimensions().content.width << "x" 
              << scrollContainer->dimensions().content.height << std::endl;
    std::cout << "Big positioned element size: " 
              << bigElement->dimensions().content.width << "x" 
              << bigElement->dimensions().content.height << std::endl;

    std::cout << "Overflow with positioned test PASSED" << std::endl;
    return 0;
}

int main() {
    std::cout << "XiaopengKernel Positioning Tests\n" << std::endl;
    
    int result = 0;
    
    result += test_absolute_positioning();
    result += test_fixed_positioning();
    result += test_mixed_layout();
    result += test_overflow_with_positioned();
    
    std::cout << "\n=== All Positioning Tests Passed ===" << std::endl;
    
    return result;
}
