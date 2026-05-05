#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

namespace xiaopeng {
namespace engine {

// Page lifecycle state
enum class PageLifecycleState {
  Loading,
  Interactive,
  Complete,
  Unloading
};

// Page lifecycle event types
enum class PageLifecycleEvent {
  DOMContentLoaded,
  Load,
  BeforeUnload,
  Unload,
  VisibilityChange
};

// Page lifecycle manager
class PageLifecycleManager {
public:
  using EventCallback = std::function<void()>;

  PageLifecycleManager() = default;

  // Get/set current state
  PageLifecycleState getState() const { return m_state; }
  void setState(PageLifecycleState state);

  // Fire lifecycle events
  void fireDOMContentLoaded();
  void fireLoad();
  void fireBeforeUnload();
  void fireUnload();

  // Register event listeners
  void addEventListener(PageLifecycleEvent event, EventCallback callback);
  void removeEventListener(PageLifecycleEvent event, EventCallback callback);

  // Reset lifecycle state
  void reset();

private:
  // Fire all registered listeners for an event
  void fireEvent(PageLifecycleEvent event);

  PageLifecycleState m_state = PageLifecycleState::Loading;
  std::unordered_map<PageLifecycleEvent, std::vector<EventCallback>> m_listeners;
};

} // namespace engine
} // namespace xiaopeng
