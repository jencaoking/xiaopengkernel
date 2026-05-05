#include <engine/event_loop.hpp>

namespace xiaopeng {
namespace engine {

void EventLoop::queueMacroTask(Task task) {
  std::lock_guard<std::mutex> lock(m_macroMutex);
  m_macroTasks.push(std::move(task));
}

void EventLoop::queueMicroTask(Task task) {
  std::lock_guard<std::mutex> lock(m_microMutex);
  m_microTasks.push(std::move(task));
}

void EventLoop::flushMicroTasks() {
  while (true) {
    Task task;
    {
      std::lock_guard<std::mutex> lock(m_microMutex);
      if (m_microTasks.empty()) {
        break;
      }
      task = std::move(m_microTasks.front());
      m_microTasks.pop();
    }
    task();
  }
}

void EventLoop::processMacroTasks(size_t maxCount) {
  size_t count = 0;
  while (true) {
    if (maxCount > 0 && count >= maxCount) {
      break;
    }

    Task task;
    {
      std::lock_guard<std::mutex> lock(m_macroMutex);
      if (m_macroTasks.empty()) {
        break;
      }
      task = std::move(m_macroTasks.front());
      m_macroTasks.pop();
    }

    task();
    count++;

    // After each macro task, flush microtasks
    flushMicroTasks();
  }
}

void EventLoop::tick() {
  // Process one macro task
  processMacroTasks(1);

  // All pending microtasks should have been flushed by processMacroTasks
  // But ensure we flush any that might have been added since last time
  flushMicroTasks();
}

size_t EventLoop::macroTaskCount() const {
  std::lock_guard<std::mutex> lock(m_macroMutex);
  return m_macroTasks.size();
}

size_t EventLoop::microTaskCount() const {
  std::lock_guard<std::mutex> lock(m_microMutex);
  return m_microTasks.size();
}

void EventLoop::clear() {
  {
    std::lock_guard<std::mutex> lock(m_macroMutex);
    std::queue<Task> empty;
    std::swap(m_macroTasks, empty);
  }
  {
    std::lock_guard<std::mutex> lock(m_microMutex);
    std::queue<Task> empty;
    std::swap(m_microTasks, empty);
  }
}

bool EventLoop::isIdle() const {
  return macroTaskCount() == 0 && microTaskCount() == 0;
}

} // namespace engine
} // namespace xiaopeng
