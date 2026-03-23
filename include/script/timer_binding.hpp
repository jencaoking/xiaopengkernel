#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <quickjs.h>
#include <vector>


namespace xiaopeng {
namespace script {

class TimerBinding {
public:
  struct TimerEntry {
    uint32_t id;
    JSValue callback; // DupValue'd, must be freed
    JSContext *ctx;
    double fireTimeMs; // Absolute time when timer should fire
    double intervalMs; // >0 for setInterval, 0 for setTimeout
    bool cancelled;
  };

  // Register setTimeout, setInterval, clearTimeout, clearInterval on global
  static void registerBinding(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global, "setTimeout",
                      JS_NewCFunction(ctx, js_setTimeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, global, "setInterval",
                      JS_NewCFunction(ctx, js_setInterval, "setInterval", 2));
    JS_SetPropertyStr(ctx, global, "clearTimeout",
                      JS_NewCFunction(ctx, js_clearTimeout, "clearTimeout", 1));
    JS_SetPropertyStr(
        ctx, global, "clearInterval",
        JS_NewCFunction(ctx, js_clearInterval, "clearInterval", 1));

    JS_FreeValue(ctx, global);
  }

  // Called each frame from main loop. Returns true if any timer fired.
  static bool tick() {
    double now = currentTimeMs();
    bool fired = false;

    // Collect IDs of timers to fire (avoid modifying map during iteration)
    std::vector<uint32_t> toFire;
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      for (auto &pair : s_timers) {
        auto &t = pair.second;
        if (!t.cancelled && now >= t.fireTimeMs) {
          toFire.push_back(t.id);
        }
      }
    }

    for (uint32_t id : toFire) {
      TimerEntry t_copy;
      {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_timers.find(id);
        if (it == s_timers.end())
          continue;
        if (it->second.cancelled)
          continue;
        t_copy = it->second; // Copy to run callback outside lock (optional but
                             // safer for reentrancy?)
                             // Actually we need to update state in map.
                             // Let's keep lock short or just copy?
        // If callback calls setTimeout, it will try to lock s_mutex.
        // IF we hold lock while calling JS_Call, DEADLOCK.
        // So we MUST release lock before JS_Call.
      }

      // But we need to use 't' which refers to map entry...
      // Map iterators might invalidate? No, std::map iterators are stable
      // unless erased. But if we unlock, s_timers might change.

      // Re-acquire logic:
      bool cancelled = false;
      TimerEntry t;
      {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_timers.find(id);
        if (it == s_timers.end() || it->second.cancelled)
          cancelled = true;
        else
          t = it->second;
      }

      if (cancelled)
        continue;

      // Call the JS callback
      JSValue ret = JS_Call(t.ctx, t.callback, JS_UNDEFINED, 0, nullptr);
      if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(t.ctx);
        const char *str = JS_ToCString(t.ctx, exc);
        if (str) {
          std::cout << "[Timer] Exception in handler (id=" << id << "): " << str
                    << std::endl;
          JS_FreeCString(t.ctx, str);
        }
        JS_FreeValue(t.ctx, exc);
      }
      JS_FreeValue(t.ctx, ret);
      fired = true;

      {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_timers.find(id);
        if (it != s_timers.end()) {
          if (t.intervalMs > 0 && !it->second.cancelled) {
            // Reschedule for next interval
            it->second.fireTimeMs = now + t.intervalMs;
          } else {
            // One-shot: remove
            JS_FreeValue(it->second.ctx, it->second.callback);
            s_timers.erase(it);
          }
        }
      }
    }

    return fired;
  }

  // Cleanup all timers (call on shutdown)
  static void cleanup() {
    std::lock_guard<std::mutex> lock(s_mutex);
    for (auto &pair : s_timers) {
      JS_FreeValue(pair.second.ctx, pair.second.callback);
    }
    s_timers.clear();
  }

private:
  static inline std::map<uint32_t, TimerEntry> s_timers;
  static inline uint32_t s_nextId = 1;
  static inline std::mutex s_mutex;

  static double currentTimeMs() {
    using namespace std::chrono;
    return static_cast<double>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
            .count());
  }

  // setTimeout(callback, delayMs) -> timerId
  static JSValue js_setTimeout(JSContext *ctx, JSValueConst this_val, int argc,
                               JSValueConst *argv) {
    (void)this_val;
    if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
      return JS_EXCEPTION;

    double delay = 0;
    if (argc >= 2)
      JS_ToFloat64(ctx, &delay, argv[1]);

    TimerEntry entry;
    entry.id = s_nextId++;
    entry.callback = JS_DupValue(ctx, argv[0]);
    entry.ctx = ctx;
    entry.fireTimeMs = currentTimeMs() + delay;
    entry.intervalMs = 0; // one-shot
    entry.cancelled = false;

    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_timers[entry.id] = entry;
    }
    return JS_NewInt32(ctx, entry.id);
  }

  // setInterval(callback, intervalMs) -> timerId
  static JSValue js_setInterval(JSContext *ctx, JSValueConst this_val, int argc,
                                JSValueConst *argv) {
    (void)this_val;
    if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
      return JS_EXCEPTION;

    double interval = 0;
    if (argc >= 2)
      JS_ToFloat64(ctx, &interval, argv[1]);
    if (interval <= 0)
      interval = 1; // minimum 1ms to prevent infinite loop

    TimerEntry entry;
    entry.id = s_nextId++;
    entry.callback = JS_DupValue(ctx, argv[0]);
    entry.ctx = ctx;
    entry.fireTimeMs = currentTimeMs() + interval;
    entry.intervalMs = interval;
    entry.cancelled = false;

    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_timers[entry.id] = entry;
    }
    return JS_NewInt32(ctx, entry.id);
  }

  // clearTimeout(id)
  static JSValue js_clearTimeout(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1)
      return JS_UNDEFINED;

    int32_t id;
    JS_ToInt32(ctx, &id, argv[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_timers.find(static_cast<uint32_t>(id));
    if (it != s_timers.end()) {
      it->second.cancelled = true;
      JS_FreeValue(it->second.ctx, it->second.callback);
      s_timers.erase(it);
    }
    return JS_UNDEFINED;
  }

  // clearInterval(id) — same as clearTimeout
  static JSValue js_clearInterval(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    return js_clearTimeout(ctx, this_val, argc, argv);
  }
};

} // namespace script
} // namespace xiaopeng
