#include "test_framework.hpp"

#include <memory>
#include "../include/dom/dom.hpp"
#include "../include/dom/event_system.hpp"
#include "../include/script/script_engine.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

// ── Tests ────────────────────────────────────────────────────

TEST(Event_BasicCreation) {
  auto event = std::make_shared<Event>("click", true, true);
  EXPECT_EQ(event->type(), "click");
  EXPECT_TRUE(event->bubbles());
  EXPECT_TRUE(event->cancelable());
  EXPECT_EQ(event->phase(), EventPhase::None);
}

TEST(Event_NoBubbles) {
  auto event = std::make_shared<Event>("load", false, false);
  EXPECT_FALSE(event->bubbles());
  EXPECT_FALSE(event->cancelable());
}

TEST(Event_StopPropagation) {
  auto event = std::make_shared<Event>("click", true, true);
  EXPECT_FALSE(event->isPropagationStopped());
  event->stopPropagation();
  EXPECT_TRUE(event->isPropagationStopped());
}

TEST(Event_PreventDefault) {
  auto event = std::make_shared<Event>("click", true, true);
  EXPECT_FALSE(event->isDefaultPrevented());
  event->preventDefault();
  EXPECT_TRUE(event->isDefaultPrevented());
}

TEST(Event_DispatchToDocument) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("target");
  body->appendChild(div);

  auto event = std::make_shared<Event>("click", true, false);
  bool dispatched = EventSystem::dispatchEvent(div, event);
  EXPECT_TRUE(dispatched);
  EXPECT_EQ(event->phase(), EventPhase::AtTarget);
}

TEST(Event_BubblePhase) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  body->appendChild(div);

  auto event = std::make_shared<Event>("click", true, false);
  EventSystem::dispatchEvent(div, event);

  // After bubbling, phase should be Bubbling or ended
  EXPECT_TRUE(event->phase() == EventPhase::Bubbling ||
              event->phase() == EventPhase::AtTarget);
}

TEST(ScriptEngine_BasicInit) {
  ScriptEngine engine;
  bool initialized = engine.initialize();
  EXPECT_TRUE(initialized);
}

TEST(ScriptEngine_EvaluateCode) {
  ScriptEngine engine;
  ASSERT_TRUE(engine.initialize());

  // Should not crash
  engine.evaluate("var x = 1 + 2;", "test.js");
  engine.evaluate("console.log('Hello from test');", "test.js");
}

int main() { return xiaopeng::test::runTests(); }
