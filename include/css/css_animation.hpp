#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <memory>
#include <optional>

namespace xiaopeng {
namespace css {

// Animation timing function types
enum class TimingFunctionType {
  Linear,
  Ease,
  EaseIn,
  EaseOut,
  EaseInOut,
  CubicBezier,
  StepSteps
};

// Timing function for animations
struct TimingFunction {
  TimingFunctionType type = TimingFunctionType::Ease;
  double x1 = 0.0;
  double y1 = 0.0;
  double x2 = 1.0;
  double y2 = 1.0;
  int stepCount = 0;
  bool jumpStart = false;

  static TimingFunction linear() {
    return {TimingFunctionType::Linear, 0, 0, 1, 1, 0, false};
  }

  static TimingFunction ease() {
    return {TimingFunctionType::Ease, 0.25, 0.1, 0.25, 1.0, 0, false};
  }

  static TimingFunction easeIn() {
    return {TimingFunctionType::EaseIn, 0.42, 0, 1.0, 1.0, 0, false};
  }

  static TimingFunction easeOut() {
    return {TimingFunctionType::EaseOut, 0, 0, 0.58, 1.0, 0, false};
  }

  static TimingFunction easeInOut() {
    return {TimingFunctionType::EaseInOut, 0.42, 0, 0.58, 1.0, 0, false};
  }

  static TimingFunction cubicBezier(double x1, double y1, double x2, double y2) {
    return {TimingFunctionType::CubicBezier, x1, y1, x2, y2, 0, false};
  }

  static TimingFunction stepSteps(int n, bool jumpStart) {
    return {TimingFunctionType::StepSteps, 0, 0, 1, 1, n, jumpStart};
  }

  double calculate(double t) const;

private:
  double solveCubicBezier(double x1, double y1, double x2, double y2, double t) const;
};

// Animation direction
enum class AnimationDirection {
  Normal,
  Reverse,
  Alternate,
  AlternateReverse
};

// Animation fill mode
enum class AnimationFillMode {
  None,
  Forwards,
  Backwards,
  Both
};

// Animation play state
enum class AnimationPlayState {
  Running,
  Paused
};

// Keyframe selector (percentage)
struct KeyframeSelector {
  double from = 0.0;
  double to = 100.0;
  bool isRange = false;

  static KeyframeSelector fromPercent(double p) {
    return {p, p, false};
  }

  static KeyframeSelector range(double f, double t) {
    return {f, t, true};
  }
};

// Keyframe block
struct KeyframeBlock {
  std::vector<KeyframeSelector> selectors;
  std::map<std::string, std::string> properties;

  std::string getProperty(const std::string &name) const {
    auto it = properties.find(name);
    return it != properties.end() ? it->second : "";
  }
};

// @keyframes rule
struct KeyframesRule {
  std::string name;
  std::vector<KeyframeBlock> blocks;

  const KeyframeBlock* findBlock(double progress) const;
};

// Animation properties
struct AnimationProperties {
  std::string name;
  double duration = 0.0;
  TimingFunction timingFunction = TimingFunction::ease();
  double delay = 0.0;
  int iterationCount = 1;
  AnimationDirection direction = AnimationDirection::Normal;
  AnimationFillMode fillMode = AnimationFillMode::None;
  AnimationPlayState playState = AnimationPlayState::Running;

  bool isInfinite() const { return iterationCount == -1; }
};

// Transition properties
struct TransitionProperties {
  std::string property;
  double duration = 0.0;
  TimingFunction timingFunction = TimingFunction::ease();
  double delay = 0.0;
};

// Animation state for a running animation
struct AnimationState {
  std::string animationName;
  double startTime = 0.0;
  double currentTime = 0.0;
  double progress = 0.0;
  int currentIteration = 0;
  AnimationPlayState playState = AnimationPlayState::Running;
  bool finished = false;
  bool started = false;

  bool isRunning() const { return playState == AnimationPlayState::Running && !finished; }
};

// Transition state
struct TransitionState {
  std::string propertyName;
  double startTime = 0.0;
  double duration = 0.0;
  std::string startValue;
  std::string endValue;
  TimingFunction timingFunction;
  bool finished = false;
  bool started = false;

  double getProgress(double currentTime) const;
  bool isRunning(double currentTime) const;
};

// Animation manager - handles all animations and transitions
class AnimationManager {
public:
  AnimationManager() = default;

  // Keyframes management
  void addKeyframes(const KeyframesRule &keyframes);
  const KeyframesRule* getKeyframes(const std::string &name) const;
  bool hasKeyframes(const std::string &name) const;

  // Animation control
  void startAnimation(const std::string &elementId, const AnimationProperties &props);
  void stopAnimation(const std::string &elementId, const std::string &name);
  void pauseAnimation(const std::string &elementId, const std::string &name);
  void resumeAnimation(const std::string &elementId, const std::string &name);

  // Transition control
  void startTransition(const std::string &elementId, const TransitionProperties &props,
                       const std::string &startValue, const std::string &endValue);
  void stopTransition(const std::string &elementId, const std::string &property);

  // Update animations
  void update(double currentTime);

  // Get animation state
  const AnimationState* getAnimationState(const std::string &elementId,
                                          const std::string &name) const;
  const TransitionState* getTransitionState(const std::string &elementId,
                                             const std::string &property) const;

  // Check if element has active animations
  bool hasActiveAnimations(const std::string &elementId) const;
  bool hasActiveTransitions(const std::string &elementId) const;

  // Get computed style at current time
  std::map<std::string, std::string> getComputedStyles(const std::string &elementId,
                                                        double currentTime) const;

  // Clear all animations for an element
  void clearElementAnimations(const std::string &elementId);

private:
  std::map<std::string, KeyframesRule> m_keyframes;
  std::map<std::string, std::vector<AnimationState>> m_animations;
  std::map<std::string, std::vector<TransitionState>> m_transitions;
};

// Animation parser - parses CSS animation syntax
class AnimationParser {
public:
  static std::optional<AnimationProperties> parseAnimation(const std::string &value);
  static std::optional<TransitionProperties> parseTransition(const std::string &value);
  static std::optional<TimingFunction> parseTimingFunction(const std::string &value);
  static std::optional<KeyframesRule> parseKeyframes(const std::string &css);
};

// Interpolation utilities
class AnimationInterpolator {
public:
  static std::string interpolate(const std::string &property,
                                  const std::string &start,
                                  const std::string &end,
                                  double progress);

  static double interpolateNumber(double start, double end, double progress);
  static std::string interpolateColor(const std::string &start, const std::string &end, double progress);
  static std::string interpolateLength(const std::string &start, const std::string &end, double progress);
};

} // namespace css
} // namespace xiaopeng
