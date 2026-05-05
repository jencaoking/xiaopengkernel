#pragma once

#include <engine/event_loop.hpp>
#include <string>
#include <memory>
#include <functional>

namespace xiaopeng {
namespace script {

// Forward declarations
struct PromiseBindingInternal;

class PromiseBinding {
public:
  // Get/set the global event loop
  static void setEventLoop(std::shared_ptr<engine::EventLoop> loop);
  static std::shared_ptr<engine::EventLoop> getEventLoop();

  // Enqueue a microtask
  static void enqueueMicroTask(std::function<void()> task);
};

} // namespace script
} // namespace xiaopeng
