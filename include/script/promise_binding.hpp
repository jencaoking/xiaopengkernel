#pragma once

#include <engine/event_loop.hpp>
#include <quickjs.h>
#include <memory>

namespace xiaopeng {
namespace script {

// Global event loop instance (shared with browser engine)
extern std::shared_ptr<engine::EventLoop> g_eventLoop;

class PromiseBinding {
public:
  // Register queueMicrotask and Promise API on global
  static void registerBinding(JSContext *ctx);

  // Get or set the global event loop
  static void setEventLoop(std::shared_ptr<engine::EventLoop> loop);
  static std::shared_ptr<engine::EventLoop> getEventLoop();

private:
  // queueMicrotask(callback)
  static JSValue js_queueMicrotask(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv);

  // Promise constructor
  static JSValue js_Promise(JSContext *ctx, JSValueConst new_target,
                           int argc, JSValueConst *argv);

  // Promise.prototype.then
  static JSValue js_Promise_then(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv);

  // Promise.prototype.catch
  static JSValue js_Promise_catch(JSContext *ctx, JSValueConst this_val,
                                   int argc, JSValueConst *argv);

  // Promise.prototype.finally
  static JSValue js_Promise_finally(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv);

  // Promise.resolve
  static JSValue js_Promise_resolve(JSContext *ctx, JSValueConst this_val,
                                     int argc, JSValueConst *argv);

  // Promise.reject
  static JSValue js_Promise_reject(JSContext *ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv);

  // Promise.all
  static JSValue js_Promise_all(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv);

  // Promise.race
  static JSValue js_Promise_race(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv);

  // Internal Promise state
  struct PromiseState {
    enum class State { Pending, Fulfilled, Rejected };
    State state = State::Pending;
    JSValue value = JS_UNDEFINED;
    JSValue reason = JS_UNDEFINED;
    std::vector<JSValue> onFulfilled;
    std::vector<JSValue> onRejected;
    JSContext *ctx;

    PromiseState(JSContext *c) : ctx(c) {}
    ~PromiseState() {
      JS_FreeValue(ctx, value);
      JS_FreeValue(ctx, reason);
      for (auto &fn : onFulfilled) {
        JS_FreeValue(ctx, fn);
      }
      for (auto &fn : onRejected) {
        JS_FreeValue(ctx, fn);
      }
    }
  };

  // Helper to enqueue microtask
  static void enqueueMicrotask(JSContext *ctx, JSValue callback,
                               JSValue this_val, int argc, JSValueConst *argv);

  // Resolve/reject helpers
  static void resolvePromise(JSContext *ctx, PromiseState *state,
                            JSValue value);
  static void rejectPromise(JSContext *ctx, PromiseState *state,
                           JSValue reason);
};

} // namespace script
} // namespace xiaopeng
