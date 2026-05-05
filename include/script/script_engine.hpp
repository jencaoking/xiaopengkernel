#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

#ifdef ENABLE_QUICKJS
#include "quickjs.h"
#include <script/console_binding.hpp>
#include <script/dom_binding.hpp>
#include <script/timer_binding.hpp>
#endif

#include <dom/dom.hpp>

namespace xiaopeng {
namespace script {

// 脚本执行结果
struct ScriptResult {
    bool success = false;
    std::string error_message;
    std::string return_value;
};

// 脚本缓存条目
struct CachedScript {
    std::vector<uint8_t> bytecode;
    std::chrono::system_clock::time_point timestamp;
    std::string source;
};

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    // Initialize the engine (create Runtime and Context)
    bool initialize();

    // Shutdown the engine
    void shutdown();

    // Evaluate JavaScript code
    void evaluate(const std::string &script,
                  const std::string &filename = "<input>");

    // Evaluate with result
    ScriptResult evaluateWithResult(const std::string &script,
                                    const std::string &filename = "<input>");

    // Compile JS to QuickJS Bytecode
    std::vector<uint8_t>
    compileToBytecode(const std::string &script,
                      const std::string &filename = "<input>");

    // Evaluate QuickJS Bytecode
    void evaluateBytecode(const std::vector<uint8_t> &bytecode);

    // Script caching
    void cacheScript(const std::string &key, const std::string &source);
    bool hasCachedScript(const std::string &key) const;
    void evaluateCached(const std::string &key);
    void clearScriptCache();
    void clearOldCache(std::chrono::seconds max_age = std::chrono::hours(24));

    // Register DOM Document to JS global 'document'
    void registerDOM(std::shared_ptr<dom::Document> doc);

    // Register global value
    void setGlobal(const std::string &name, const std::string &value);
    void setGlobal(const std::string &name, bool value);
    void setGlobal(const std::string &name, int value);
    void setGlobal(const std::string &name, double value);

    // Get global value
    std::string getGlobalString(const std::string &name);
    bool getGlobalBool(const std::string &name);
    int getGlobalInt(const std::string &name);
    double getGlobalDouble(const std::string &name);

    // Call JS function
    ScriptResult callFunction(const std::string &function_name,
                              const std::vector<std::string> &args = {});

    // Process pending timers — call each frame. Returns true if any fired.
    bool tickTimers();

    // Process Microtasks (Promises etc.) - call each frame. Returns true if jobs executed.
    bool tickMicrotasks();

    // Process all pending tasks (microtasks + timers)
    bool tick();

    // GC management
    void runGC();
    void setGCLimit(size_t limit);
    size_t getMemoryUsage() const;

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

    // Enable/disable strict mode
    void setStrictMode(bool strict) { m_strict_mode = strict; }
    bool isStrictMode() const { return m_strict_mode; }

    // Version info
    static std::string getQuickJSVersion();
    static std::string getEngineVersion();

private:
    bool m_initialized = false;
    bool m_strict_mode = false;
    std::function<void(dom::DirtyFlag)> m_onMutation;
    std::unordered_map<std::string, CachedScript> m_script_cache;

#ifdef ENABLE_QUICKJS
    JSRuntime *m_runtime = nullptr;
    JSContext *m_ctx = nullptr;

    // Helper methods
    void registerBuiltins();
    void setupGlobalObject();
    std::string stringifyValue(JSValueConst value);
    ScriptResult handleException(JSValue exception);

public:
    // Expose JSContext for event dispatch from main loop
    JSContext *getContext() const { return m_ctx; }
    JSRuntime *getRuntime() const { return m_runtime; }

    static ScriptEngine *getInstance(JSContext *ctx) {
        return static_cast<ScriptEngine *>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    }
#endif
};

} // namespace script
} // namespace xiaopeng
