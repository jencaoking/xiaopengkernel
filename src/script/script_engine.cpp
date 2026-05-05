#include <script/script_engine.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

#ifdef ENABLE_QUICKJS
#include "quickjs-libc.h"
#endif

namespace xiaopeng {
namespace script {

#ifdef ENABLE_QUICKJS

// QuickJS version getter
std::string ScriptEngine::getQuickJSVersion() {
    return "2025-09-13"; // From VERSION file
}

std::string ScriptEngine::getEngineVersion() {
    return "XiaopengKernel 0.1.0 (QuickJS " + getQuickJSVersion() + ")";
}

ScriptEngine::ScriptEngine() : m_initialized(false), m_strict_mode(false) {
}

ScriptEngine::~ScriptEngine() {
    shutdown();
}

void ScriptEngine::shutdown() {
    if (!m_initialized)
        return;

    TimerBinding::cleanup();
    EventBinding::cleanup();
    DOMBinding::cleanup();

    if (m_ctx) {
        JS_FreeContext(m_ctx);
        m_ctx = nullptr;
    }

    if (m_runtime) {
        JS_FreeRuntime(m_runtime);
        m_runtime = nullptr;
    }

    m_initialized = false;
    m_script_cache.clear();
}

bool ScriptEngine::initialize() {
    if (m_initialized)
        return true;

    m_runtime = JS_NewRuntime();
    if (!m_runtime) {
        std::cerr << "Failed to create QuickJS runtime" << std::endl;
        return false;
    }

    JS_SetRuntimeOpaque(m_runtime, this);

    // Set default GC limit to 256MB
    JS_SetMemoryLimit(m_runtime, 256 * 1024 * 1024);

    m_ctx = JS_NewContext(m_runtime);
    if (!m_ctx) {
        std::cerr << "Failed to create QuickJS context" << std::endl;
        JS_FreeRuntime(m_runtime);
        m_runtime = nullptr;
        return false;
    }

    // Initialize stdlib (console, etc.)
    js_std_init_handlers(m_runtime);
    JS_SetModuleLoaderFunc(m_runtime, nullptr, nullptr, nullptr);

    m_initialized = true;

    // Setup builtin objects and global
    registerBuiltins();
    setupGlobalObject();

    // Register Bindings
    ConsoleBinding::registerBinding(m_ctx);
    TimerBinding::registerBinding(m_ctx);
    DOMBinding::registerBinding(m_ctx);

    return true;
}

void ScriptEngine::registerBuiltins() {
    // Add any custom builtins here
}

void ScriptEngine::setupGlobalObject() {
    JSValue global_obj = JS_GetGlobalObject(m_ctx);

    // Set globalThis as alias
    JS_DupValue(m_ctx, global_obj);
    JS_SetPropertyStr(m_ctx, global_obj, "globalThis", global_obj);

    // Set __xiaopeng__ version info
    JSValue version_str = JS_NewString(m_ctx, getEngineVersion().c_str());
    JS_SetPropertyStr(m_ctx, global_obj, "__xiaopeng__", version_str);

    JS_FreeValue(m_ctx, global_obj);
}

void ScriptEngine::evaluate(const std::string &script,
                            const std::string &filename) {
    if (!m_initialized)
        return;

    int eval_flags = 0;
    if (m_strict_mode) {
        eval_flags |= JS_EVAL_FLAG_STRICT;
    }

    JSValue val = JS_Eval(m_ctx, script.c_str(), script.length(),
                          filename.c_str(), eval_flags);

    if (JS_IsException(val)) {
        ScriptResult result = handleException(JS_GetException(m_ctx));
        std::cerr << "JS Error: " << result.error_message << std::endl;
    }

    JS_FreeValue(m_ctx, val);
}

std::string ScriptEngine::stringifyValue(JSValueConst value) {
    if (!m_ctx)
        return "";

    JSValue str_val = JS_ToString(m_ctx, value);
    if (JS_IsException(str_val)) {
        JS_FreeValue(m_ctx, str_val);
        return "[exception]";
    }

    const char *c_str = JS_ToCString(m_ctx, str_val);
    std::string result(c_str ? c_str : "");
    JS_FreeCString(m_ctx, c_str);
    JS_FreeValue(m_ctx, str_val);

    return result;
}

ScriptResult ScriptEngine::handleException(JSValue exception) {
    ScriptResult result;
    result.success = false;

    if (!m_ctx) {
        result.error_message = "No valid context";
        return result;
    }

    // Get error message
    JSValue message_val = JS_GetPropertyStr(m_ctx, exception, "message");
    if (!JS_IsUndefined(message_val)) {
        const char *msg = JS_ToCString(m_ctx, message_val);
        if (msg)
            result.error_message = msg;
        JS_FreeCString(m_ctx, msg);
    } else {
        // Fallback to stringifying the exception
        result.error_message = stringifyValue(exception);
    }

    // Get stack trace if available
    JSValue stack_val = JS_GetPropertyStr(m_ctx, exception, "stack");
    if (!JS_IsUndefined(stack_val)) {
        const char *stack = JS_ToCString(m_ctx, stack_val);
        if (stack)
            result.error_message += "\nStack trace:\n" + std::string(stack);
        JS_FreeCString(m_ctx, stack);
    }

    JS_FreeValue(m_ctx, stack_val);
    JS_FreeValue(m_ctx, message_val);
    JS_FreeValue(m_ctx, exception);

    return result;
}

ScriptResult ScriptEngine::evaluateWithResult(const std::string &script,
                                              const std::string &filename) {
    ScriptResult result;

    if (!m_initialized) {
        result.success = false;
        result.error_message = "Script engine not initialized";
        return result;
    }

    int eval_flags = 0;
    if (m_strict_mode) {
        eval_flags |= JS_EVAL_FLAG_STRICT;
    }

    JSValue val = JS_Eval(m_ctx, script.c_str(), script.length(),
                          filename.c_str(), eval_flags);

    if (JS_IsException(val)) {
        result = handleException(JS_GetException(m_ctx));
    } else {
        result.success = true;
        result.return_value = stringifyValue(val);
    }

    JS_FreeValue(m_ctx, val);
    return result;
}

std::vector<uint8_t>
ScriptEngine::compileToBytecode(const std::string &script,
                                const std::string &filename) {
    if (!m_initialized || !m_ctx)
        return {};

    int eval_flags = JS_EVAL_FLAG_COMPILE_ONLY;
    if (m_strict_mode) {
        eval_flags |= JS_EVAL_FLAG_STRICT;
    }

    JSValue obj = JS_Eval(m_ctx, script.c_str(), script.length(),
                          filename.c_str(), eval_flags);
    if (JS_IsException(obj)) {
        ScriptResult result = handleException(JS_GetException(m_ctx));
        std::cerr << "Compile Error: " << result.error_message << std::endl;
        JS_FreeValue(m_ctx, obj);
        return {};
    }

    size_t out_buf_len;
    uint8_t *out_buf =
        JS_WriteObject(m_ctx, &out_buf_len, obj, JS_WRITE_OBJ_BYTECODE);
    JS_FreeValue(m_ctx, obj);

    if (!out_buf)
        return {};

    std::vector<uint8_t> bytecode(out_buf, out_buf + out_buf_len);
    js_free(m_ctx, out_buf);
    return bytecode;
}

void ScriptEngine::evaluateBytecode(const std::vector<uint8_t> &bytecode) {
    if (!m_initialized || bytecode.empty())
        return;

    JSValue obj = JS_ReadObject(m_ctx, bytecode.data(), bytecode.size(),
                                JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj)) {
        ScriptResult result = handleException(JS_GetException(m_ctx));
        std::cerr << "Bytecode Read Error: " << result.error_message << std::endl;
        JS_FreeValue(m_ctx, obj);
        return;
    }

    JSValue val = JS_EvalFunction(m_ctx, obj);
    if (JS_IsException(val)) {
        ScriptResult result = handleException(JS_GetException(m_ctx));
        std::cerr << "Bytecode Exec Error: " << result.error_message << std::endl;
    }

    JS_FreeValue(m_ctx, val);
}

void ScriptEngine::cacheScript(const std::string &key,
                               const std::string &source) {
    auto bytecode = compileToBytecode(source, key);
    if (!bytecode.empty()) {
        CachedScript entry;
        entry.bytecode = std::move(bytecode);
        entry.timestamp = std::chrono::system_clock::now();
        entry.source = source;
        m_script_cache[key] = std::move(entry);
    }
}

bool ScriptEngine::hasCachedScript(const std::string &key) const {
    return m_script_cache.find(key) != m_script_cache.end();
}

void ScriptEngine::evaluateCached(const std::string &key) {
    auto it = m_script_cache.find(key);
    if (it != m_script_cache.end()) {
        evaluateBytecode(it->second.bytecode);
    }
}

void ScriptEngine::clearScriptCache() {
    m_script_cache.clear();
}

void ScriptEngine::clearOldCache(std::chrono::seconds max_age) {
    auto now = std::chrono::system_clock::now();
    for (auto it = m_script_cache.begin(); it != m_script_cache.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp);
        if (age > max_age) {
            it = m_script_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void ScriptEngine::registerDOM(std::shared_ptr<dom::Document> doc) {
    if (!m_initialized || !doc)
        return;

    JSValue docObj = DOMBinding::wrapDocument(m_ctx, doc);

    JSValue global = JS_GetGlobalObject(m_ctx);
    JS_SetPropertyStr(m_ctx, global, "document", docObj);
    JS_FreeValue(m_ctx, global);
}

void ScriptEngine::setGlobal(const std::string &name, const std::string &value) {
    if (!m_initialized || !m_ctx)
        return;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue js_val = JS_NewString(m_ctx, value.c_str());
    JS_SetPropertyStr(m_ctx, global, name.c_str(), js_val);
    JS_FreeValue(m_ctx, global);
}

void ScriptEngine::setGlobal(const std::string &name, bool value) {
    if (!m_initialized || !m_ctx)
        return;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue js_val = JS_NewBool(m_ctx, value);
    JS_SetPropertyStr(m_ctx, global, name.c_str(), js_val);
    JS_FreeValue(m_ctx, global);
}

void ScriptEngine::setGlobal(const std::string &name, int value) {
    if (!m_initialized || !m_ctx)
        return;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue js_val = JS_NewInt32(m_ctx, value);
    JS_SetPropertyStr(m_ctx, global, name.c_str(), js_val);
    JS_FreeValue(m_ctx, global);
}

void ScriptEngine::setGlobal(const std::string &name, double value) {
    if (!m_initialized || !m_ctx)
        return;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue js_val = JS_NewFloat64(m_ctx, value);
    JS_SetPropertyStr(m_ctx, global, name.c_str(), js_val);
    JS_FreeValue(m_ctx, global);
}

std::string ScriptEngine::getGlobalString(const std::string &name) {
    if (!m_initialized || !m_ctx)
        return "";

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue val = JS_GetPropertyStr(m_ctx, global, name.c_str());
    std::string result = stringifyValue(val);
    JS_FreeValue(m_ctx, val);
    JS_FreeValue(m_ctx, global);
    return result;
}

bool ScriptEngine::getGlobalBool(const std::string &name) {
    if (!m_initialized || !m_ctx)
        return false;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue val = JS_GetPropertyStr(m_ctx, global, name.c_str());
    bool result = JS_ToBool(m_ctx, val);
    JS_FreeValue(m_ctx, val);
    JS_FreeValue(m_ctx, global);
    return result;
}

int ScriptEngine::getGlobalInt(const std::string &name) {
    if (!m_initialized || !m_ctx)
        return 0;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue val = JS_GetPropertyStr(m_ctx, global, name.c_str());
    int32_t result;
    if (JS_ToInt32(m_ctx, &result, val) < 0)
        result = 0;
    JS_FreeValue(m_ctx, val);
    JS_FreeValue(m_ctx, global);
    return result;
}

double ScriptEngine::getGlobalDouble(const std::string &name) {
    if (!m_initialized || !m_ctx)
        return 0.0;

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue val = JS_GetPropertyStr(m_ctx, global, name.c_str());
    double result;
    if (JS_ToFloat64(m_ctx, &result, val) < 0)
        result = 0.0;
    JS_FreeValue(m_ctx, val);
    JS_FreeValue(m_ctx, global);
    return result;
}

ScriptResult ScriptEngine::callFunction(const std::string &function_name,
                                        const std::vector<std::string> &args) {
    ScriptResult result;
    result.success = false;

    if (!m_initialized || !m_ctx) {
        result.error_message = "Engine not initialized";
        return result;
    }

    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue func = JS_GetPropertyStr(m_ctx, global, function_name.c_str());
    JS_FreeValue(m_ctx, global);

    if (!JS_IsFunction(m_ctx, func)) {
        result.error_message = "Not a function: " + function_name;
        JS_FreeValue(m_ctx, func);
        return result;
    }

    // Prepare arguments
    std::vector<JSValue> js_args;
    js_args.reserve(args.size());
    for (const auto &arg : args) {
        js_args.push_back(JS_NewString(m_ctx, arg.c_str()));
    }

    JSValue ret = JS_Call(m_ctx, func, JS_UNDEFINED, js_args.size(),
                          js_args.data());

    // Free args
    for (auto &arg : js_args) {
        JS_FreeValue(m_ctx, arg);
    }
    JS_FreeValue(m_ctx, func);

    if (JS_IsException(ret)) {
        result = handleException(JS_GetException(m_ctx));
    } else {
        result.success = true;
        result.return_value = stringifyValue(ret);
    }

    JS_FreeValue(m_ctx, ret);
    return result;
}

bool ScriptEngine::tickTimers() {
    if (!m_initialized)
        return false;
    return TimerBinding::tick();
}

bool ScriptEngine::tickMicrotasks() {
    if (!m_initialized || !m_ctx)
        return false;

    JSContext *ctx1;
    bool executed = false;

    for (;;) {
        int err = JS_ExecutePendingJob(m_runtime, &ctx1);
        if (err <= 0) {
            if (err < 0) {
                ScriptResult result = handleException(JS_GetException(ctx1));
                std::cerr << "Microtask Error: " << result.error_message << std::endl;
            }
            break;
        }
        executed = true;
    }
    return executed;
}

bool ScriptEngine::tick() {
    bool did_work = false;
    did_work |= tickMicrotasks();
    did_work |= tickTimers();
    return did_work;
}

void ScriptEngine::runGC() {
    if (m_runtime) {
        JS_RunGC(m_runtime);
    }
}

void ScriptEngine::setGCLimit(size_t limit) {
    if (m_runtime) {
        JS_SetMemoryLimit(m_runtime, limit);
    }
}

size_t ScriptEngine::getMemoryUsage() const {
    if (m_runtime) {
        JSMemoryUsage usage;
        JS_ComputeMemoryUsage(m_runtime, &usage);
        return usage.malloc_size + usage.memory_used_size;
    }
    return 0;
}

#else // ENABLE_QUICKJS not defined

ScriptEngine::ScriptEngine() : m_initialized(false) {
}

ScriptEngine::~ScriptEngine() {
}

void ScriptEngine::shutdown() {
}

bool ScriptEngine::initialize() {
    std::cerr << "QuickJS support is disabled in build." << std::endl;
    return false;
}

std::string ScriptEngine::getQuickJSVersion() {
    return "Unknown (QuickJS disabled)";
}

std::string ScriptEngine::getEngineVersion() {
    return "XiaopengKernel 0.1.0 (QuickJS disabled)";
}

void ScriptEngine::evaluate(const std::string &, const std::string &) {
}

ScriptResult ScriptEngine::evaluateWithResult(const std::string &, const std::string &) {
    ScriptResult r;
    r.success = false;
    r.error_message = "QuickJS disabled";
    return r;
}

std::vector<uint8_t> ScriptEngine::compileToBytecode(const std::string &, const std::string &) {
    return {};
}

void ScriptEngine::evaluateBytecode(const std::vector<uint8_t> &) {
}

void ScriptEngine::cacheScript(const std::string &, const std::string &) {
}

bool ScriptEngine::hasCachedScript(const std::string &) const { return false; }

void ScriptEngine::evaluateCached(const std::string &) {
}

void ScriptEngine::clearScriptCache() {
}

void ScriptEngine::clearOldCache(std::chrono::seconds) {
}

void ScriptEngine::registerDOM(std::shared_ptr<dom::Document>) {
}

void ScriptEngine::setGlobal(const std::string &, const std::string &) {
}

void ScriptEngine::setGlobal(const std::string &, bool) {
}

void ScriptEngine::setGlobal(const std::string &, int) {
}

void ScriptEngine::setGlobal(const std::string &, double) {
}

std::string ScriptEngine::getGlobalString(const std::string &) { return ""; }

bool ScriptEngine::getGlobalBool(const std::string &) { return false; }

int ScriptEngine::getGlobalInt(const std::string &) { return 0; }

double ScriptEngine::getGlobalDouble(const std::string &) { return 0.0; }

ScriptResult ScriptEngine::callFunction(const std::string &, const std::vector<std::string> &) {
    ScriptResult r;
    r.success = false;
    return r;
}

bool ScriptEngine::tickTimers() { return false; }

bool ScriptEngine::tickMicrotasks() { return false; }

bool ScriptEngine::tick() { return false; }

void ScriptEngine::runGC() {
}

void ScriptEngine::setGCLimit(size_t) {
}

size_t ScriptEngine::getMemoryUsage() const { return 0; }

#endif // ENABLE_QUICKJS

} // namespace script
} // namespace xiaopeng
