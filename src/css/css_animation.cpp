#include <css/css_animation.hpp>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <regex>
#include <cctype>

namespace xiaopeng {
namespace css {

// TimingFunction implementation
double TimingFunction::calculate(double t) const {
  t = std::max(0.0, std::min(1.0, t));

  switch (type) {
  case TimingFunctionType::Linear:
    return t;

  case TimingFunctionType::Ease:
    return solveCubicBezier(0.25, 0.1, 0.25, 1.0, t);

  case TimingFunctionType::EaseIn:
    return solveCubicBezier(0.42, 0, 1.0, 1.0, t);

  case TimingFunctionType::EaseOut:
    return solveCubicBezier(0, 0, 0.58, 1.0, t);

  case TimingFunctionType::EaseInOut:
    return solveCubicBezier(0.42, 0, 0.58, 1.0, t);

  case TimingFunctionType::CubicBezier:
    return solveCubicBezier(x1, y1, x2, y2, t);

  case TimingFunctionType::StepSteps:
    if (stepCount <= 0) return t;
    if (jumpStart) {
      return std::floor(t * stepCount) / stepCount;
    } else {
      return std::floor(t * stepCount + 0.5) / stepCount;
    }

  default:
    return t;
  }
}

double TimingFunction::solveCubicBezier(double x1, double y1, double x2, double y2, double t) const {
  auto bezier = [x1, x2](double t) -> double {
    double t2 = t * t;
    double t3 = t2 * t;
    double mt = 1 - t;
    double mt2 = mt * mt;
    double mt3 = mt2 * mt;
    return 3 * mt2 * t * x1 + 3 * mt * t2 * x2 + t3;
  };

  auto bezierDerivative = [x1, x2](double t) -> double {
    double t2 = t * t;
    double mt = 1 - t;
    double mt2 = mt * mt;
    return 3 * mt2 * x1 + 6 * mt * t * (x2 - x1) + 3 * t2 * (1 - x2);
  };

  double guess = t;
  for (int i = 0; i < 8; ++i) {
    double currentX = bezier(guess);
    double dx = currentX - t;
    if (std::abs(dx) < 1e-6) break;

    double derivative = bezierDerivative(guess);
    if (std::abs(derivative) < 1e-6) break;

    guess -= dx / derivative;
    guess = std::max(0.0, std::min(1.0, guess));
  }

  double t2 = guess * guess;
  double t3 = t2 * guess;
  double mt = 1 - guess;
  double mt2 = mt * mt;
  double mt3 = mt2 * mt;

  return 3 * mt2 * guess * y1 + 3 * mt * t2 * y2 + t3;
}

// KeyframesRule implementation
const KeyframeBlock* KeyframesRule::findBlock(double progress) const {
  progress = std::max(0.0, std::min(1.0, progress)) * 100.0;

  for (const auto &block : blocks) {
    for (const auto &selector : block.selectors) {
      if (selector.isRange) {
        if (progress >= selector.from && progress <= selector.to) {
          return &block;
        }
      } else {
        if (std::abs(progress - selector.from) < 0.01) {
          return &block;
        }
      }
    }
  }

  return nullptr;
}

// TransitionState implementation
double TransitionState::getProgress(double currentTime) const {
  if (duration <= 0) return 1.0;
  double elapsed = currentTime - startTime;
  return std::max(0.0, std::min(1.0, elapsed / duration));
}

bool TransitionState::isRunning(double currentTime) const {
  if (finished) return false;
  return getProgress(currentTime) < 1.0;
}

// AnimationManager implementation
void AnimationManager::addKeyframes(const KeyframesRule &keyframes) {
  m_keyframes[keyframes.name] = keyframes;
}

const KeyframesRule* AnimationManager::getKeyframes(const std::string &name) const {
  auto it = m_keyframes.find(name);
  return it != m_keyframes.end() ? &it->second : nullptr;
}

bool AnimationManager::hasKeyframes(const std::string &name) const {
  return m_keyframes.find(name) != m_keyframes.end();
}

void AnimationManager::startAnimation(const std::string &elementId,
                                       const AnimationProperties &props) {
  AnimationState state;
  state.animationName = props.name;
  state.startTime = 0.0;
  state.currentTime = 0.0;
  state.progress = 0.0;
  state.currentIteration = 0;
  state.playState = props.playState;
  state.finished = false;
  state.started = true;

  m_animations[elementId].push_back(state);
}

void AnimationManager::stopAnimation(const std::string &elementId,
                                      const std::string &name) {
  auto it = m_animations.find(elementId);
  if (it != m_animations.end()) {
    auto &states = it->second;
    states.erase(std::remove_if(states.begin(), states.end(),
                                [&name](const AnimationState &s) {
                                  return s.animationName == name;
                                }),
                 states.end());
  }
}

void AnimationManager::pauseAnimation(const std::string &elementId,
                                       const std::string &name) {
  auto it = m_animations.find(elementId);
  if (it != m_animations.end()) {
    for (auto &state : it->second) {
      if (state.animationName == name) {
        state.playState = AnimationPlayState::Paused;
      }
    }
  }
}

void AnimationManager::resumeAnimation(const std::string &elementId,
                                        const std::string &name) {
  auto it = m_animations.find(elementId);
  if (it != m_animations.end()) {
    for (auto &state : it->second) {
      if (state.animationName == name) {
        state.playState = AnimationPlayState::Running;
      }
    }
  }
}

void AnimationManager::startTransition(const std::string &elementId,
                                        const TransitionProperties &props,
                                        const std::string &startValue,
                                        const std::string &endValue) {
  TransitionState state;
  state.propertyName = props.property;
  state.startTime = 0.0;
  state.duration = props.duration;
  state.startValue = startValue;
  state.endValue = endValue;
  state.timingFunction = props.timingFunction;
  state.finished = false;
  state.started = true;

  m_transitions[elementId].push_back(state);
}

void AnimationManager::stopTransition(const std::string &elementId,
                                       const std::string &property) {
  auto it = m_transitions.find(elementId);
  if (it != m_transitions.end()) {
    auto &states = it->second;
    states.erase(std::remove_if(states.begin(), states.end(),
                                [&property](const TransitionState &s) {
                                  return s.propertyName == property;
                                }),
                 states.end());
  }
}

void AnimationManager::update(double currentTime) {
  for (auto &[elementId, states] : m_animations) {
    for (auto &state : states) {
      if (state.isRunning()) {
        state.currentTime = currentTime;
        state.progress = state.finished ? 1.0 : std::fmod(currentTime, 1.0);
      }
    }
  }

  for (auto &[elementId, states] : m_transitions) {
    for (auto &state : states) {
      if (!state.finished) {
        double progress = state.getProgress(currentTime);
        if (progress >= 1.0) {
          state.finished = true;
        }
      }
    }
  }
}

const AnimationState* AnimationManager::getAnimationState(const std::string &elementId,
                                                           const std::string &name) const {
  auto it = m_animations.find(elementId);
  if (it != m_animations.end()) {
    for (const auto &state : it->second) {
      if (state.animationName == name) {
        return &state;
      }
    }
  }
  return nullptr;
}

const TransitionState* AnimationManager::getTransitionState(const std::string &elementId,
                                                             const std::string &property) const {
  auto it = m_transitions.find(elementId);
  if (it != m_transitions.end()) {
    for (const auto &state : it->second) {
      if (state.propertyName == property) {
        return &state;
      }
    }
  }
  return nullptr;
}

bool AnimationManager::hasActiveAnimations(const std::string &elementId) const {
  auto it = m_animations.find(elementId);
  if (it != m_animations.end()) {
    for (const auto &state : it->second) {
      if (state.isRunning()) return true;
    }
  }
  return false;
}

bool AnimationManager::hasActiveTransitions(const std::string &elementId) const {
  auto it = m_transitions.find(elementId);
  if (it != m_transitions.end()) {
    for (const auto &state : it->second) {
      if (!state.finished) return true;
    }
  }
  return false;
}

std::map<std::string, std::string> AnimationManager::getComputedStyles(
    const std::string &elementId, double currentTime) const {
  std::map<std::string, std::string> styles;

  auto transitionIt = m_transitions.find(elementId);
  if (transitionIt != m_transitions.end()) {
    for (const auto &state : transitionIt->second) {
      if (!state.finished) {
        double progress = state.timingFunction.calculate(state.getProgress(currentTime));
        styles[state.propertyName] = AnimationInterpolator::interpolate(
            state.propertyName, state.startValue, state.endValue, progress);
      }
    }
  }

  return styles;
}

void AnimationManager::clearElementAnimations(const std::string &elementId) {
  m_animations.erase(elementId);
  m_transitions.erase(elementId);
}

// AnimationParser implementation
std::optional<AnimationProperties> AnimationParser::parseAnimation(const std::string &value) {
  AnimationProperties props;

  std::istringstream iss(value);
  std::string token;

  while (iss >> token) {
    if (token == "none") {
      return props;
    }

    if (token.length() > 1 && 
        (token.substr(token.length()-2) == "ms" || 
         (token.back() == 's' && std::isdigit(token[token.length()-2])))) {
      std::string numStr = token;
      if (token.substr(token.length()-2) == "ms") {
        numStr = token.substr(0, token.length()-2);
      } else {
        numStr = token.substr(0, token.length()-1);
      }
      
      try {
        double duration = std::stod(numStr);
        if (token.substr(token.length()-2) == "ms") {
          duration /= 1000.0;
        }
        if (props.duration == 0) {
          props.duration = duration;
        } else if (props.delay == 0) {
          props.delay = duration;
        }
      } catch (...) {
      }
    } else if (token == "infinite") {
      props.iterationCount = -1;
    } else if (token == "normal") {
      props.direction = AnimationDirection::Normal;
    } else if (token == "reverse") {
      props.direction = AnimationDirection::Reverse;
    } else if (token == "alternate") {
      props.direction = AnimationDirection::Alternate;
    } else if (token == "alternate-reverse") {
      props.direction = AnimationDirection::AlternateReverse;
    } else if (token == "none") {
      props.fillMode = AnimationFillMode::None;
    } else if (token == "forwards") {
      props.fillMode = AnimationFillMode::Forwards;
    } else if (token == "backwards") {
      props.fillMode = AnimationFillMode::Backwards;
    } else if (token == "both") {
      props.fillMode = AnimationFillMode::Both;
    } else if (token == "running") {
      props.playState = AnimationPlayState::Running;
    } else if (token == "paused") {
      props.playState = AnimationPlayState::Paused;
    } else if (token == "linear") {
      props.timingFunction = TimingFunction::linear();
    } else if (token == "ease") {
      props.timingFunction = TimingFunction::ease();
    } else if (token == "ease-in") {
      props.timingFunction = TimingFunction::easeIn();
    } else if (token == "ease-out") {
      props.timingFunction = TimingFunction::easeOut();
    } else if (token == "ease-in-out") {
      props.timingFunction = TimingFunction::easeInOut();
    } else {
      bool isNumber = true;
      try {
        size_t pos;
        std::stod(token, &pos);
        if (pos != token.length()) isNumber = false;
      } catch (...) {
        isNumber = false;
      }
      
      if (isNumber) {
        try {
          int count = std::stoi(token);
          props.iterationCount = count;
        } catch (...) {
          if (props.name.empty()) {
            props.name = token;
          }
        }
      } else {
        if (props.name.empty()) {
          props.name = token;
        }
      }
    }
  }

  return props;
}

std::optional<TransitionProperties> AnimationParser::parseTransition(const std::string &value) {
  TransitionProperties props;

  std::istringstream iss(value);
  std::string token;

  while (iss >> token) {
    if (token == "all") {
      props.property = "all";
    } else if (token == "none") {
      props.property = "none";
    } else if (token.length() > 1 && 
               (token.substr(token.length()-2) == "ms" || 
                (token.back() == 's' && std::isdigit(token[token.length()-2])))) {
      std::string numStr = token;
      if (token.substr(token.length()-2) == "ms") {
        numStr = token.substr(0, token.length()-2);
      } else {
        numStr = token.substr(0, token.length()-1);
      }
      
      try {
        double duration = std::stod(numStr);
        if (token.substr(token.length()-2) == "ms") {
          duration /= 1000.0;
        }
        if (props.duration == 0) {
          props.duration = duration;
        } else if (props.delay == 0) {
          props.delay = duration;
        }
      } catch (...) {
      }
    } else if (token == "linear") {
      props.timingFunction = TimingFunction::linear();
    } else if (token == "ease") {
      props.timingFunction = TimingFunction::ease();
    } else if (token == "ease-in") {
      props.timingFunction = TimingFunction::easeIn();
    } else if (token == "ease-out") {
      props.timingFunction = TimingFunction::easeOut();
    } else if (token == "ease-in-out") {
      props.timingFunction = TimingFunction::easeInOut();
    } else {
      props.property = token;
    }
  }

  return props;
}

std::optional<TimingFunction> AnimationParser::parseTimingFunction(const std::string &value) {
  if (value == "linear") return TimingFunction::linear();
  if (value == "ease") return TimingFunction::ease();
  if (value == "ease-in") return TimingFunction::easeIn();
  if (value == "ease-out") return TimingFunction::easeOut();
  if (value == "ease-in-out") return TimingFunction::easeInOut();

  if (value.find("cubic-bezier") != std::string::npos) {
    std::regex pattern(R"(cubic-bezier\s*\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*\))");
    std::smatch match;
    if (std::regex_search(value, match, pattern)) {
      double x1 = std::stod(match[1].str());
      double y1 = std::stod(match[2].str());
      double x2 = std::stod(match[3].str());
      double y2 = std::stod(match[4].str());
      return TimingFunction::cubicBezier(x1, y1, x2, y2);
    }
  }

  if (value.find("steps") != std::string::npos) {
    std::regex pattern(R"(steps\s*\(\s*(\d+)\s*(?:,\s*(\w+))?\s*\))");
    std::smatch match;
    if (std::regex_search(value, match, pattern)) {
      int n = std::stoi(match[1].str());
      bool jumpStart = match[2].matched && match[2].str() == "jump-start";
      return TimingFunction::stepSteps(n, jumpStart);
    }
  }

  return std::nullopt;
}

std::optional<KeyframesRule> AnimationParser::parseKeyframes(const std::string &css) {
  KeyframesRule rule;

  std::regex namePattern(R"(@keyframes\s+(\w+)\s*\{)");
  std::smatch nameMatch;
  if (!std::regex_search(css, nameMatch, namePattern)) {
    return std::nullopt;
  }
  rule.name = nameMatch[1].str();

  std::regex blockPattern(R"((\d+%|from|to)\s*\{([^}]*)\})");
  std::sregex_iterator it(css.begin(), css.end(), blockPattern);
  std::sregex_iterator end;

  while (it != end) {
    KeyframeBlock block;
    std::string selector = (*it)[1].str();
    std::string properties = (*it)[2].str();

    if (selector == "from") {
      block.selectors.push_back(KeyframeSelector::fromPercent(0));
    } else if (selector == "to") {
      block.selectors.push_back(KeyframeSelector::fromPercent(100));
    } else {
      double percent = std::stod(selector);
      block.selectors.push_back(KeyframeSelector::fromPercent(percent));
    }

    std::regex propPattern(R"(([\w-]+)\s*:\s*([^;]+))");
    std::sregex_iterator propIt(properties.begin(), properties.end(), propPattern);
    while (propIt != end) {
      std::string propName = (*propIt)[1].str();
      std::string propValue = (*propIt)[2].str();
      block.properties[propName] = propValue;
      ++propIt;
    }

    rule.blocks.push_back(block);
    ++it;
  }

  return rule;
}

// AnimationInterpolator implementation
double AnimationInterpolator::interpolateNumber(double start, double end, double progress) {
  return start + (end - start) * progress;
}

std::string AnimationInterpolator::interpolateColor(const std::string &start,
                                                     const std::string &end,
                                                     double progress) {
  auto parseColor = [](const std::string &color) -> std::tuple<int, int, int, int> {
    if (color.substr(0, 4) == "rgba") {
      std::regex pattern(R"(rgba\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\))");
      std::smatch match;
      if (std::regex_search(color, match, pattern)) {
        return {std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]),
                static_cast<int>(std::stod(match[4]) * 255)};
      }
    } else if (color.substr(0, 3) == "rgb") {
      std::regex pattern(R"(rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\))");
      std::smatch match;
      if (std::regex_search(color, match, pattern)) {
        return {std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]), 255};
      }
    } else if (color.substr(0, 1) == "#") {
      std::string hex = color.substr(1);
      if (hex.length() == 6) {
        int r = std::stoi(hex.substr(0, 2), nullptr, 16);
        int g = std::stoi(hex.substr(2, 2), nullptr, 16);
        int b = std::stoi(hex.substr(4, 2), nullptr, 16);
        return {r, g, b, 255};
      }
    }
    return {0, 0, 0, 255};
  };

  auto [r1, g1, b1, a1] = parseColor(start);
  auto [r2, g2, b2, a2] = parseColor(end);

  int r = static_cast<int>(interpolateNumber(r1, r2, progress));
  int g = static_cast<int>(interpolateNumber(g1, g2, progress));
  int b = static_cast<int>(interpolateNumber(b1, b2, progress));
  int a = static_cast<int>(interpolateNumber(a1, a2, progress));

  return "rgba(" + std::to_string(r) + "," + std::to_string(g) + "," +
         std::to_string(b) + "," + std::to_string(a / 255.0) + ")";
}

std::string AnimationInterpolator::interpolateLength(const std::string &start,
                                                      const std::string &end,
                                                      double progress) {
  std::regex pattern(R"(([\d.]+)(px|em|rem|%))");
  std::smatch startMatch, endMatch;

  if (!std::regex_search(start, startMatch, pattern) || !std::regex_search(end, endMatch, pattern)) {
    return progress < 0.5 ? start : end;
  }

  double startValue = std::stod(startMatch[1].str());
  double endValue = std::stod(endMatch[1].str());
  std::string unit = endMatch[2].str();

  double result = interpolateNumber(startValue, endValue, progress);
  return std::to_string(result) + unit;
}

std::string AnimationInterpolator::interpolate(const std::string &property,
                                                const std::string &start,
                                                const std::string &end,
                                                double progress) {
  if (property.find("color") != std::string::npos || start[0] == '#') {
    return interpolateColor(start, end, progress);
  }

  if (start.find("px") != std::string::npos || start.find("em") != std::string::npos ||
      start.find("rem") != std::string::npos || start.find("%") != std::string::npos) {
    return interpolateLength(start, end, progress);
  }

  try {
    double startNum = std::stod(start);
    double endNum = std::stod(end);
    return std::to_string(interpolateNumber(startNum, endNum, progress));
  } catch (...) {
    return progress < 0.5 ? start : end;
  }
}

} // namespace css
} // namespace xiaopeng
