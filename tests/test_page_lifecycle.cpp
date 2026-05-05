#include <engine/page_lifecycle.hpp>
#include <engine/event_loop.hpp>
#include <script/promise_binding_simple.hpp>
#include <iostream>
#include <cassert>

using namespace xiaopeng;
using namespace xiaopeng::engine;
using namespace xiaopeng::script;

void testPageLifecycleBasics() {
  std::cout << "=== Testing Page Lifecycle Basics ===\n";
  
  PageLifecycleManager lifecycle;

  assert(lifecycle.getState() == PageLifecycleState::Loading);
  std::cout << "Initial state: Loading ✓\n";

  int domContentLoadedCount = 0;
  int loadCount = 0;
  int beforeUnloadCount = 0;
  int unloadCount = 0;

  // Register listeners
  lifecycle.addEventListener(PageLifecycleEvent::DOMContentLoaded,
    [&]() { domContentLoadedCount++; });

  lifecycle.addEventListener(PageLifecycleEvent::Load,
    [&]() { loadCount++; });

  lifecycle.addEventListener(PageLifecycleEvent::BeforeUnload,
    [&]() { beforeUnloadCount++; });

  lifecycle.addEventListener(PageLifecycleEvent::Unload,
    [&]() { unloadCount++; });

  // Fire DOMContentLoaded
  lifecycle.fireDOMContentLoaded();
  assert(domContentLoadedCount == 1);
  assert(lifecycle.getState() == PageLifecycleState::Interactive);
  std::cout << "DOMContentLoaded: ✓ (state: Interactive)\n";

  // Fire Load
  lifecycle.fireLoad();
  assert(loadCount == 1);
  assert(lifecycle.getState() == PageLifecycleState::Complete);
  std::cout << "Load: ✓ (state: Complete)\n";

  // Fire BeforeUnload
  lifecycle.fireBeforeUnload();
  assert(beforeUnloadCount == 1);
  assert(lifecycle.getState() == PageLifecycleState::Unloading);
  std::cout << "BeforeUnload: ✓ (state: Unloading)\n";

  // Fire Unload
  lifecycle.fireUnload();
  assert(unloadCount == 1);
  std::cout << "Unload: ✓\n";

  // Reset
  lifecycle.reset();
  assert(lifecycle.getState() == PageLifecycleState::Loading);
  std::cout << "Reset to Loading: ✓\n";

  std::cout << "Page Lifecycle Basics test PASSED!\n";
}

void testMultipleListeners() {
  std::cout << "\n=== Testing Multiple Listeners ===\n";

  PageLifecycleManager lifecycle;

  std::vector<int> callOrder;

  lifecycle.addEventListener(PageLifecycleEvent::DOMContentLoaded,
    [&]() { callOrder.push_back(1); });

  lifecycle.addEventListener(PageLifecycleEvent::DOMContentLoaded,
    [&]() { callOrder.push_back(2); });

  lifecycle.addEventListener(PageLifecycleEvent::DOMContentLoaded,
    [&]() { callOrder.push_back(3); });

  lifecycle.fireDOMContentLoaded();

  assert(callOrder.size() == 3);
  assert(callOrder[0] == 1);
  assert(callOrder[1] == 2);
  assert(callOrder[2] == 3);

  std::cout << "Multiple listeners: ✓ (call order preserved)\n";
  std::cout << "Multiple Listeners test PASSED!\n";
}

void testPromiseBinding() {
  std::cout << "\n=== Testing Promise Binding (Microtask Queue) ===\n";

  auto eventLoop = std::make_shared<EventLoop>();
  PromiseBinding::setEventLoop(eventLoop);

  int microTaskCount = 0;

  PromiseBinding::enqueueMicroTask([&]() { microTaskCount++; });
  PromiseBinding::enqueueMicroTask([&]() { microTaskCount++; });

  assert(microTaskCount == 0); // Tasks are queued but not executed yet
  std::cout << "Tasks queued but not executed ✓\n";

  eventLoop->flushMicroTasks();
  assert(microTaskCount == 2);
  std::cout << "Microtasks executed: ✓\n";

  std::cout << "Promise Binding (Microtask Queue) test PASSED!\n";
}

void testEventLoopWithPromiseOrder() {
  std::cout << "\n=== Testing Event Loop Task Ordering ===\n";

  auto eventLoop = std::make_shared<EventLoop>();

  std::vector<std::string> executionOrder;

  // Queue macro tasks
  eventLoop->queueMacroTask([&]() {
    executionOrder.push_back("macro1");
  });
  eventLoop->queueMacroTask([&]() {
    executionOrder.push_back("macro2");
  });

  // Queue micro tasks (should execute *after* each macro task)
  eventLoop->queueMicroTask([&]() {
    executionOrder.push_back("micro1");
  });
  eventLoop->queueMicroTask([&]() {
    executionOrder.push_back("micro2");
  });

  // Tick the loop once
  eventLoop->tick();

  // After first tick, should have: macro1, then all microtasks
  assert(executionOrder.size() == 3);
  assert(executionOrder[0] == "macro1");
  assert(executionOrder[1] == "micro1");
  assert(executionOrder[2] == "micro2");

  std::cout << "First tick: macro then micro ✓\n";

  // Tick again
  eventLoop->tick();

  assert(executionOrder.size() == 4);
  assert(executionOrder[3] == "macro2");

  std::cout << "Second tick: next macro ✓\n";

  std::cout << "Event Loop Task Ordering test PASSED!\n";
}

int main() {
  std::cout << "Running Page Lifecycle and Promise Binding tests\n";
  std::cout << "================================================\n";

  try {
    testPageLifecycleBasics();
    testMultipleListeners();
    testPromiseBinding();
    testEventLoopWithPromiseOrder();

    std::cout << "================================================\n";
    std::cout << "All Page Lifecycle and Promise Binding tests PASSED!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test FAILED: " << e.what() << "\n";
    return 1;
  }
}
