#include <css/css_animation.hpp>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace xiaopeng;
using namespace xiaopeng::css;

void testTimingFunctions() {
  std::cout << "=== Testing Timing Functions ===\n";

  auto linear = TimingFunction::linear();
  assert(std::abs(linear.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(linear.calculate(0.5) - 0.5) < 0.001);
  assert(std::abs(linear.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Linear timing: ✓\n";

  auto ease = TimingFunction::ease();
  assert(std::abs(ease.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(ease.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Ease timing: ✓\n";

  auto easeIn = TimingFunction::easeIn();
  assert(std::abs(easeIn.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(easeIn.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Ease-in timing: ✓\n";

  auto easeOut = TimingFunction::easeOut();
  assert(std::abs(easeOut.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(easeOut.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Ease-out timing: ✓\n";

  auto easeInOut = TimingFunction::easeInOut();
  assert(std::abs(easeInOut.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(easeInOut.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Ease-in-out timing: ✓\n";

  auto cubic = TimingFunction::cubicBezier(0.42, 0, 0.58, 1.0);
  assert(std::abs(cubic.calculate(0.0) - 0.0) < 0.001);
  assert(std::abs(cubic.calculate(1.0) - 1.0) < 0.001);
  std::cout << "Cubic-bezier timing: ✓\n";

  auto steps = TimingFunction::stepSteps(4, false);
  assert(std::abs(steps.calculate(0.0) - 0.0) < 0.001);
  std::cout << "Steps timing: ✓\n";

  std::cout << "Timing Functions test PASSED!\n";
}

void testAnimationProperties() {
  std::cout << "\n=== Testing Animation Properties ===\n";

  AnimationProperties props;
  props.name = "fade";
  props.duration = 1.0;
  props.delay = 0.5;
  props.iterationCount = 3;
  props.direction = AnimationDirection::Alternate;
  props.fillMode = AnimationFillMode::Forwards;

  assert(props.name == "fade");
  std::cout << "Animation name: ✓\n";

  assert(props.duration == 1.0);
  std::cout << "Animation duration: ✓\n";

  assert(props.delay == 0.5);
  std::cout << "Animation delay: ✓\n";

  assert(props.iterationCount == 3);
  assert(!props.isInfinite());
  std::cout << "Animation iteration count: ✓\n";

  props.iterationCount = -1;
  assert(props.isInfinite());
  std::cout << "Infinite animation: ✓\n";

  std::cout << "Animation Properties test PASSED!\n";
}

void testTransitionProperties() {
  std::cout << "\n=== Testing Transition Properties ===\n";

  TransitionProperties props;
  props.property = "opacity";
  props.duration = 0.3;
  props.timingFunction = TimingFunction::easeOut();
  props.delay = 0.1;

  assert(props.property == "opacity");
  std::cout << "Transition property: ✓\n";

  assert(props.duration == 0.3);
  std::cout << "Transition duration: ✓\n";

  std::cout << "Transition Properties test PASSED!\n";
}

void testKeyframesRule() {
  std::cout << "\n=== Testing Keyframes Rule ===\n";

  KeyframesRule rule;
  rule.name = "slide";

  KeyframeBlock fromBlock;
  fromBlock.selectors.push_back(KeyframeSelector::fromPercent(0));
  fromBlock.properties["transform"] = "translateX(0)";
  fromBlock.properties["opacity"] = "1";
  rule.blocks.push_back(fromBlock);

  KeyframeBlock toBlock;
  toBlock.selectors.push_back(KeyframeSelector::fromPercent(100));
  toBlock.properties["transform"] = "translateX(100px)";
  toBlock.properties["opacity"] = "0";
  rule.blocks.push_back(toBlock);

  assert(rule.name == "slide");
  std::cout << "Keyframes name: ✓\n";

  assert(rule.blocks.size() == 2);
  std::cout << "Keyframes blocks: ✓\n";

  const KeyframeBlock* block0 = rule.findBlock(0.0);
  assert(block0 != nullptr);
  assert(block0->getProperty("transform") == "translateX(0)");
  std::cout << "Find block at 0%: ✓\n";

  const KeyframeBlock* block100 = rule.findBlock(1.0);
  assert(block100 != nullptr);
  assert(block100->getProperty("transform") == "translateX(100px)");
  std::cout << "Find block at 100%: ✓\n";

  std::cout << "Keyframes Rule test PASSED!\n";
}

void testAnimationManager() {
  std::cout << "\n=== Testing Animation Manager ===\n";

  AnimationManager manager;

  KeyframesRule keyframes;
  keyframes.name = "fade";
  KeyframeBlock fromBlock;
  fromBlock.selectors.push_back(KeyframeSelector::fromPercent(0));
  fromBlock.properties["opacity"] = "1";
  keyframes.blocks.push_back(fromBlock);
  KeyframeBlock toBlock;
  toBlock.selectors.push_back(KeyframeSelector::fromPercent(100));
  toBlock.properties["opacity"] = "0";
  keyframes.blocks.push_back(toBlock);

  manager.addKeyframes(keyframes);
  assert(manager.hasKeyframes("fade"));
  std::cout << "Add keyframes: ✓\n";

  const KeyframesRule* found = manager.getKeyframes("fade");
  assert(found != nullptr);
  assert(found->name == "fade");
  std::cout << "Get keyframes: ✓\n";

  AnimationProperties animProps;
  animProps.name = "fade";
  animProps.duration = 1.0;

  manager.startAnimation("element1", animProps);
  assert(manager.hasActiveAnimations("element1"));
  std::cout << "Start animation: ✓\n";

  const AnimationState* state = manager.getAnimationState("element1", "fade");
  assert(state != nullptr);
  assert(state->animationName == "fade");
  std::cout << "Get animation state: ✓\n";

  manager.pauseAnimation("element1", "fade");
  state = manager.getAnimationState("element1", "fade");
  assert(state->playState == AnimationPlayState::Paused);
  std::cout << "Pause animation: ✓\n";

  manager.resumeAnimation("element1", "fade");
  state = manager.getAnimationState("element1", "fade");
  assert(state->playState == AnimationPlayState::Running);
  std::cout << "Resume animation: ✓\n";

  manager.stopAnimation("element1", "fade");
  assert(!manager.hasActiveAnimations("element1"));
  std::cout << "Stop animation: ✓\n";

  std::cout << "Animation Manager test PASSED!\n";
}

void testTransitionState() {
  std::cout << "\n=== Testing Transition State ===\n";

  TransitionState state;
  state.propertyName = "opacity";
  state.startTime = 0.0;
  state.duration = 1.0;
  state.startValue = "1";
  state.endValue = "0";
  state.timingFunction = TimingFunction::ease();

  assert(state.isRunning(0.5));
  std::cout << "Transition running: ✓\n";

  double progress = state.getProgress(0.5);
  assert(progress >= 0.0 && progress <= 1.0);
  std::cout << "Transition progress: ✓\n";

  assert(!state.isRunning(2.0));
  std::cout << "Transition finished: ✓\n";

  std::cout << "Transition State test PASSED!\n";
}

void testAnimationInterpolator() {
  std::cout << "\n=== Testing Animation Interpolator ===\n";

  double num = AnimationInterpolator::interpolateNumber(0, 100, 0.5);
  assert(std::abs(num - 50.0) < 0.001);
  std::cout << "Number interpolation: ✓\n";

  std::string color = AnimationInterpolator::interpolateColor("#ff0000", "#0000ff", 0.5);
  assert(!color.empty());
  std::cout << "Color interpolation: ✓\n";

  std::string length = AnimationInterpolator::interpolateLength("0px", "100px", 0.5);
  assert(!length.empty());
  std::cout << "Length interpolation: ✓\n";

  std::string opacity = AnimationInterpolator::interpolate("opacity", "1", "0", 0.5);
  assert(!opacity.empty());
  std::cout << "Property interpolation: ✓\n";

  std::cout << "Animation Interpolator test PASSED!\n";
}

void testAnimationParser() {
  std::cout << "\n=== Testing Animation Parser ===\n";

  auto simpleProps = AnimationParser::parseAnimation("fade 1s");
  assert(simpleProps.has_value());
  assert(simpleProps->name == "fade");
  assert(simpleProps->duration == 1.0);
  std::cout << "Simple animation parse: ✓\n";

  auto forwardsProps = AnimationParser::parseAnimation("test 2s forwards");
  assert(forwardsProps.has_value());
  assert(forwardsProps->fillMode == AnimationFillMode::Forwards);
  std::cout << "FillMode forwards: ✓\n";

  auto animProps = AnimationParser::parseAnimation("fade 1s ease-in-out 0.5s infinite alternate forwards");
  assert(animProps.has_value());
  assert(animProps->name == "fade");
  std::cout << "Animation name: ✓\n";
  
  assert(animProps->duration == 1.0);
  std::cout << "Animation duration: ✓\n";
  
  assert(animProps->delay == 0.5);
  std::cout << "Animation delay: ✓\n";
  
  assert(animProps->isInfinite());
  std::cout << "Animation infinite: ✓\n";
  
  assert(animProps->direction == AnimationDirection::Alternate);
  std::cout << "Animation direction: ✓\n";
  
  assert(animProps->fillMode == AnimationFillMode::Forwards);
  std::cout << "Parse animation: ✓\n";

  auto transProps = AnimationParser::parseTransition("opacity 0.3s ease-out 0.1s");
  assert(transProps.has_value());
  assert(transProps->property == "opacity");
  assert(transProps->duration == 0.3);
  assert(transProps->delay == 0.1);
  std::cout << "Parse transition: ✓\n";

  auto timing = AnimationParser::parseTimingFunction("cubic-bezier(0.42, 0, 0.58, 1)");
  assert(timing.has_value());
  assert(timing->type == TimingFunctionType::CubicBezier);
  std::cout << "Parse timing function: ✓\n";

  std::cout << "Animation Parser test PASSED!\n";
}

int main() {
  std::cout << "Running CSS Animation tests\n";
  std::cout << "================================================\n";

  try {
    testTimingFunctions();
    testAnimationProperties();
    testTransitionProperties();
    testKeyframesRule();
    testAnimationManager();
    testTransitionState();
    testAnimationInterpolator();
    testAnimationParser();

    std::cout << "================================================\n";
    std::cout << "All CSS Animation tests PASSED!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test FAILED: " << e.what() << "\n";
    return 1;
  }
}
