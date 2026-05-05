#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace xiaopeng {
namespace engine {

class EventLoop {
public:
  using Task = std::function<void()>;

  enum class TaskQueue { Macro, Micro };

  EventLoop() = default;

  // Queue a macro task (setTimeout, event listeners, etc.)
  void queueMacroTask(Task task);

  // Queue a micro task (Promise.then, queueMicrotask, etc.)
  void queueMicroTask(Task task);

  // Run a single iteration of the event loop
  void tick();

  // Flush all pending microtasks
  void flushMicroTasks();

  // Process pending macro tasks up to limit (0 = no limit)
  void processMacroTasks(size_t maxCount = 0);

  // Get pending task counts
  size_t macroTaskCount() const;
  size_t microTaskCount() const;

  // Clear all pending tasks
  void clear();

  // Check if idle (no pending tasks)
  bool isIdle() const;

private:
  std::queue<Task> m_macroTasks;
  std::queue<Task> m_microTasks;
  mutable std::mutex m_macroMutex;
  mutable std::mutex m_microMutex;
};

} // namespace engine
} // namespace xiaopeng
