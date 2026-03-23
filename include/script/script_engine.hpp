#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifdef ENABLE_QUICKJS
#include "quickjs.h"
#include <script/console_binding.hpp>
#include <script/dom_binding.hpp>
#include <script/timer_binding.hpp>
#endif

#include <dom/dom.hpp>

namespace xiaopeng {
namespace script {

class ScriptEngine {
public:
  ScriptEngine();
  ~ScriptEngine();

  // Initialize the engine (create Runtime and Context)
  bool initialize();

  // Evaluate JavaScript code
  void evaluate(const std::string &script,
                const std::string &filename = "<input>");

  // Compile JS to QuickJS Bytecode Let
  std::vector<uint8_t>
  compileToBytecode(const std::string &script,
                    const std::string &filename = "<input>");

  // Evaluate QuickJS Bytecode
  void evaluateBytecode(const std::vector<uint8_t> &bytecode);

  // Register DOM Document to JS global 'document'
  void registerDOM(std::shared_ptr<dom::Document> doc);

  // Register a global function
  // For simplicity in this stage, we might just have specific binding methods
  // or a generic way to be added later.

  // Process pending timers — call each frame. Returns true if any fired.
  bool tickTimers();

  // Process Microtasks (Promises etc.) - call each frame. Returns true if jobs
  // executed.
  bool tickMicrotasks();

  // Set callback for DOM mutations
  void setOnMutation(std::function<void(dom::DirtyFlag)> cb) {
    m_onMutation = cb;
  }
  void notifyMutation(dom::DirtyFlag hint = dom::DirtyFlag::NeedsLayout) {
    if (m_onMutation)
      m_onMutation(hint);
  }

  // Check if engine is valid
  bool isValid() const { return m_initialized; }

private:
  bool m_initialized = false;
  std::function<void(dom::DirtyFlag)> m_onMutation;

#ifdef ENABLE_QUICKJS
  JSRuntime *m_runtime = nullptr;
  JSContext *m_ctx = nullptr;

public:
  // Expose JSContext for event dispatch from main loop
  JSContext *getContext() const { return m_ctx; }

  static ScriptEngine *getInstance(JSContext *ctx) {
    return static_cast<ScriptEngine *>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
  }
#endif
};

} // namespace script
} // namespace xiaopeng
