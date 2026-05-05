#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "css/css_parser.hpp"
#include "css/css_types.hpp"
#include "css/computed_style.hpp"
#include "css/css_animation.hpp"

using namespace xiaopeng::css;

void test_keyframes_parsing() {
  std::cout << "=== Testing @keyframes parsing ===" << std::endl;

  std::string css = R"(
    @keyframes slideIn {
      from {
        transform: translateX(-100%);
        opacity: 0;
      }
      50% {
        transform: translateX(0%);
        opacity: 0.5;
      }
      to {
        transform: translateX(0%);
        opacity: 1;
      }
    }

    @keyframes pulse {
      0%, 100% {
        transform: scale(1);
      }
      50% {
        transform: scale(1.05);
      }
    }

    div {
      width: 100px;
      height: 100px;
      background-color: red;
    }
  )";

  CssParser parser(css);
  ExtendedStyleSheet sheet = parser.parseExtended();

  assert(sheet.keyframesRules.size() >= 2 && "Should have parsed 2 keyframes rules");

  bool foundSlideIn = false;
  bool foundPulse = false;
  for (const auto& kf : sheet.keyframesRules) {
    if (kf.name == "slideIn") {
      foundSlideIn = true;
      std::cout << "  Found @keyframes slideIn" << std::endl;
      std::cout << "  - Block count: " << kf.blocks.size() << std::endl;
    }
    if (kf.name == "pulse") {
      foundPulse = true;
      std::cout << "  Found @keyframes pulse" << std::endl;
    }
  }

  assert(foundSlideIn && "Should have found slideIn keyframes");
  assert(foundPulse && "Should have found pulse keyframes");

  std::cout << "@keyframes parsing PASSED!" << std::endl;
}

void test_computed_style_animation_properties() {
  std::cout << std::endl << "=== Testing Computed Style Animation Properties ===" << std::endl;

  ComputedStyle style;

  // Test that all animation properties exist with defaults
  assert(style.animationName.empty() && "animationName should default to empty");
  assert(style.animationDuration == 0.0f && "animationDuration should default to 0");
  assert(style.animationDelay == 0.0f && "animationDelay should default to 0");
  assert(style.animationIterationCount == 1 && "animationIterationCount should default to 1");
  assert(style.animationDirection == "normal" && "animationDirection should default to 'normal'");
  assert(style.animationFillMode == "none" && "animationFillMode should default to 'none'");
  assert(style.animationPlayState == "running" && "animationPlayState should default to 'running'");

  // Test setting and accessing
  style.animationName = "testAnimation";
  style.animationDuration = 2.5f;
  style.animationDelay = 0.5f;
  style.animationIterationCount = 3;
  style.animationDirection = "alternate";
  style.animationFillMode = "both";
  style.animationPlayState = "paused";

  assert(style.animationName == "testAnimation" && "animationName should be set");
  assert(style.animationDuration == 2.5f && "animationDuration should be set");
  assert(style.animationDelay == 0.5f && "animationDelay should be set");
  assert(style.animationIterationCount == 3 && "animationIterationCount should be set");
  assert(style.animationDirection == "alternate" && "animationDirection should be set");
  assert(style.animationFillMode == "both" && "animationFillMode should be set");
  assert(style.animationPlayState == "paused" && "animationPlayState should be set");

  std::cout << "Computed Style Animation Properties PASSED!" << std::endl;
}

void test_computed_style_transition_properties() {
  std::cout << std::endl << "=== Testing Computed Style Transition Properties ===" << std::endl;

  ComputedStyle style;

  // Check defaults
  assert(style.transitionProperty.size() > 0 && "transitionProperty should have default");
  assert(style.transitionProperty[0] == "all" && "transitionProperty should default to 'all'");
  assert(style.transitionDuration.size() > 0 && "transitionDuration should have default");
  assert(style.transitionDelay.size() > 0 && "transitionDelay should have default");
  assert(style.transitionTimingFunction.size() > 0 && "transitionTimingFunction should have default");

  // Set some values
  style.transitionProperty = {"opacity", "transform"};
  style.transitionDuration = {0.3f, 0.5f};
  style.transitionDelay = {0.0f, 0.1f};
  style.transitionTimingFunction = {"ease-out", "cubic-bezier(0.4, 0, 0.2, 1)"};

  assert(style.transitionProperty.size() == 2 && "Should have 2 transition properties");
  assert(style.transitionDuration.size() == 2 && "Should have 2 transition durations");
  assert(style.transitionDelay.size() == 2 && "Should have 2 transition delays");
  assert(style.transitionTimingFunction.size() == 2 && "Should have 2 transition timing functions");

  std::cout << "Computed Style Transition Properties PASSED!" << std::endl;
}

void test_computed_style_border_radius() {
  std::cout << std::endl << "=== Testing Computed Style Border Radius Properties ===" << std::endl;

  ComputedStyle style;
  style.borderTopLeftRadius = Length::Px(5);
  style.borderTopRightRadius = Length::Px(10);
  style.borderBottomLeftRadius = Length::Px(15);
  style.borderBottomRightRadius = Length::Px(20);

  assert(style.borderTopLeftRadius.value == 5 && "borderTopLeftRadius should be set");
  assert(style.borderTopRightRadius.value == 10 && "borderTopRightRadius should be set");
  assert(style.borderBottomLeftRadius.value == 15 && "borderBottomLeftRadius should be set");
  assert(style.borderBottomRightRadius.value == 20 && "borderBottomRightRadius should be set");

  std::cout << "Computed Style Border Radius Properties PASSED!" << std::endl;
}

void test_keyframe_parser_integration() {
  std::cout << std::endl << "=== Testing Keyframe Parser Integration ===" << std::endl;

  std::string css = R"(
    /* Normal rule */
    .box {
      animation-name: fadeIn;
      animation-duration: 1s;
    }

    /* Keyframes rule */
    @keyframes fadeIn {
      from {
        opacity: 0;
      }
      to {
        opacity: 1;
      }
    }
  )";

  CssParser parser(css);
  ExtendedStyleSheet sheet = parser.parseExtended();

  assert(sheet.rules.size() >= 1 && "Should have normal rules");
  assert(sheet.keyframesRules.size() >= 1 && "Should have keyframes rules");

  std::cout << "Keyframe Parser Integration PASSED!" << std::endl;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  XiaopengKernel CSS Complete Test Suite" << std::endl;
  std::cout << "========================================" << std::endl;

  try {
    test_keyframes_parsing();
    test_computed_style_animation_properties();
    test_computed_style_transition_properties();
    test_computed_style_border_radius();
    test_keyframe_parser_integration();

    std::cout << std::endl << "========================================" << std::endl;
    std::cout << "  All CSS Complete Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test FAILED: " << e.what() << std::endl;
    return 1;
  }
}
