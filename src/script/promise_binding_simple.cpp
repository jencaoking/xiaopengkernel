#include <script/promise_binding_simple.hpp>

namespace xiaopeng {
namespace script {

namespace {
// Global event loop instance for script bindings
std::shared_ptr<engine::EventLoop> g_event_loop;
} // namespace

void PromiseBinding::setEventLoop(std::shared_ptr<engine::EventLoop> loop) {
  g_event_loop = loop;
}

std::shared_ptr<engine::EventLoop> PromiseBinding::getEventLoop() {
  return g_event_loop;
}

void PromiseBinding::enqueueMicroTask(std::function<void()> task) {
  if (g_event_loop) {
    g_event_loop->queueMicroTask(task);
  } else {
    // Fallback: execute immediately if no event loop
    task();
  }
}

} // namespace script
} // namespace xiaopeng
