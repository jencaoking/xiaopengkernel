#pragma once

#include "html_types.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace xiaopeng {
namespace dom {

// Event phase constants (W3C standard)
enum class EventPhase : uint8_t {
  None = 0,
  Capturing = 1,
  AtTarget = 2,
  Bubbling = 3
};

// Event class representing a DOM Event
class Event {
public:
  Event(const std::string &type, bool bubbles = false,
        bool cancelable = false)
      : type_(type), bubbles_(bubbles), cancelable_(cancelable),
        phase_(EventPhase::None), stoppedPropagation_(false),
        preventedDefault_(false) {}

  const std::string &type() const { return type_; }
  bool bubbles() const { return bubbles_; }
  bool cancelable() const { return cancelable_; }

  EventPhase phase() const { return phase_; }
  void setPhase(EventPhase phase) { phase_ = phase; }

  NodePtr target() const { return target_; }
  void setTarget(NodePtr target) { target_ = target; }

  NodePtr currentTarget() const { return currentTarget_; }
  void setCurrentTarget(NodePtr currentTarget) {
    currentTarget_ = currentTarget;
  }

  void stopPropagation() { stoppedPropagation_ = true; }
  bool isPropagationStopped() const { return stoppedPropagation_; }

  void preventDefault() { preventedDefault_ = true; }
  bool isDefaultPrevented() const { return preventedDefault_; }

private:
  std::string type_;
  bool bubbles_;
  bool cancelable_;
  EventPhase phase_;
  NodePtr target_;
  NodePtr currentTarget_;
  bool stoppedPropagation_;
  bool preventedDefault_;
};

// EventSystem handles complete W3C event flow
class EventSystem {
public:
  // Dispatch an event with complete capture -> target -> bubble flow
  // Returns true if the event was prevented default
  static bool dispatchEvent(NodePtr target, const std::shared_ptr<Event> &event) {
    if (!target)
      return false;

    event->setTarget(target);

    // Build the propagation path (parent chain from root to target)
    std::vector<NodePtr> propagationPath;
    NodePtr current = target;
    while (current) {
      propagationPath.push_back(current);
      current = current->parentNode();
    }
    // Reverse to get from root -> target for capture
    std::reverse(propagationPath.begin(), propagationPath.end());

    // Capture phase: from root down to parent of target
    event->setPhase(EventPhase::Capturing);
    for (size_t i = 0; i < propagationPath.size() - 1; i++) {
      if (event->isPropagationStopped())
        break;
      invokeEventListeners(propagationPath[i], event, EventPhase::Capturing);
    }

    // Target phase
    if (!event->isPropagationStopped()) {
      event->setPhase(EventPhase::AtTarget);
      invokeEventListeners(target, event, EventPhase::AtTarget);
    }

    // Bubble phase (only if event bubbles): from target's parent up to root
    if (event->bubbles() && !event->isPropagationStopped()) {
      event->setPhase(EventPhase::Bubbling);
      for (size_t i = propagationPath.size() - 2; i < propagationPath.size();
           i--) {
        if (event->isPropagationStopped())
          break;
        invokeEventListeners(propagationPath[i], event, EventPhase::Bubbling);
      }
    }

    return event->isDefaultPrevented();
  }

private:
  // Invoke listeners for given phase (just a placeholder for now;
  // will integrate with ScriptEngine later)
  static void invokeEventListeners(NodePtr node,
                                   const std::shared_ptr<Event> &event,
                                   EventPhase phase) {
    event->setCurrentTarget(node);
    // The actual callback invocation will happen in the ScriptEngine
    // or EventBinding layer with QuickJS
  }
};

} // namespace dom
} // namespace xiaopeng
