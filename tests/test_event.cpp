#include <iostream>
#include <memory>
#include <vector>

#include "../include/dom/dom.hpp"
#include "../include/dom/event_system.hpp"
#include "../include/script/dom_binding.hpp"
#include "../include/script/event_binding.hpp"
#include "../include/script/script_engine.hpp"
#include "../third_party/quickjs/quickjs.h"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

// Test basic event creation and properties
int test_event_creation() {
  std::cout << "=== Testing Event Creation ===" << std::endl;

  auto event = std::make_shared<Event>("click", true, true);
  bool success = true;

  if (event->type() != "click")
    success = false;
  if (!event->bubbles())
    success = false;
  if (!event->cancelable())
    success = false;

  std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
  return success ? 0 : 1;
}

// Test event flow with a simple DOM hierarchy
int test_event_flow() {
  std::cout << "=== Testing Event Flow ===" << std::endl;

  // Create a simple DOM: document -> body -> div
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("target");
  body->appendChild(div);

  // Create an event that bubbles
  auto event = std::make_shared<Event>("click", true, false);
  EventSystem::dispatchEvent(div, event);

  std::cout << "  Test PASSED (event dispatched)" << std::endl;
  return 0;
}

// Test script engine with events
int test_script_engine_events() {
  std::cout << "=== Testing Script Engine Events ===" << std::endl;

  ScriptEngine engine;
  bool initialized = engine.initialize();
  if (!initialized) {
    std::cout << "  Test FAILED - engine init failed!" << std::endl;
    return 1;
  }

  // Create DOM
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("myDiv");
  body->appendChild(div);

  engine.registerDOM(doc);

  // Test that basic functions work (without crashing)
  engine.evaluate("console.log('Test script loaded');", "test.js");

  std::cout << "  Test PASSED" << std::endl;
  return 0;
}

int main() {
  std::cout << "\n=== Event System Tests ===" << std::endl;

  int failures = 0;
  failures += test_event_creation();
  failures += test_event_flow();
  failures += test_script_engine_events();

  if (failures == 0) {
    std::cout << "\n✅ All Event System Tests Passed!" << std::endl;
    return 0;
  } else {
    std::cout << "\n❌ " << failures << " Event System Tests Failed!"
              << std::endl;
    return 1;
  }
}
