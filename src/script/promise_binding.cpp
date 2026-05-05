#include <script/promise_binding.hpp>
#include <iostream>

namespace xiaopeng {
namespace script {

std::shared_ptr<engine::EventLoop> g_eventLoop = nullptr;

void PromiseBinding::setEventLoop(std::shared_ptr<engine::EventLoop> loop) {
  g_eventLoop = loop;
}

std::shared_ptr<engine::EventLoop> PromiseBinding::getEventLoop() {
  return g_eventLoop;
}

void PromiseBinding::registerBinding(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);

  // queueMicrotask
  JS_SetPropertyStr(ctx, global, "queueMicrotask",
                    JS_NewCFunction(ctx, js_queueMicrotask, "queueMicrotask", 1));

  // Promise constructor and prototype
  JSValue PromiseProto = JS_NewObject(ctx);
  JS_SetPropertyStr(
      ctx, PromiseProto, "then",
      JS_NewCFunction(ctx, js_Promise_then, "then", 2));
  JS_SetPropertyStr(
      ctx, PromiseProto, "catch",
      JS_NewCFunction(ctx, js_Promise_catch, "catch", 1));
  JS_SetPropertyStr(
      ctx, PromiseProto, "finally",
      JS_NewCFunction(ctx, js_Promise_finally, "finally", 1));

  JSValue PromiseCtor = JS_NewCFunction2(
      ctx, js_Promise, "Promise", 1, JS_CFUNC_constructor, 0);
  JS_SetPropertyStr(ctx, PromiseCtor, "resolve",
                    JS_NewCFunction(ctx, js_Promise_resolve, "resolve", 1));
  JS_SetPropertyStr(ctx, PromiseCtor, "reject",
                    JS_NewCFunction(ctx, js_Promise_reject, "reject", 1));
  JS_SetPropertyStr(ctx, PromiseCtor, "all",
                    JS_NewCFunction(ctx, js_Promise_all, "all", 1));
  JS_SetPropertyStr(ctx, PromiseCtor, "race",
                    JS_NewCFunction(ctx, js_Promise_race, "race", 1));
  JS_SetPropertyStr(ctx, PromiseCtor, "prototype", PromiseProto);

  JS_SetPropertyStr(ctx, global, "Promise", PromiseCtor);

  JS_FreeValue(ctx, global);
}

JSValue PromiseBinding::js_queueMicrotask(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
    return JS_EXCEPTION;
  }

  JSValue callback = JS_DupValue(ctx, argv[0]);

  if (g_eventLoop) {
    g_eventLoop->queueMicroTask([ctx, callback]() {
      JSValue ret = JS_Call(ctx, callback, JS_UNDEFINED, 0, nullptr);
      if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        if (str) {
          std::cout << "[queueMicrotask] Exception: " << str << std::endl;
          JS_FreeCString(ctx, str);
        }
        JS_FreeValue(ctx, exc);
      }
      JS_FreeValue(ctx, ret);
      JS_FreeValue(ctx, callback);
    });
  } else {
    // Fallback: call synchronously if no event loop
    JSValue ret = JS_Call(ctx, callback, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(ret)) {
      JS_FreeValue(ctx, ret);
    }
    JS_FreeValue(ctx, callback);
  }

  return JS_UNDEFINED;
}

JSValue PromiseBinding::js_Promise(JSContext *ctx, JSValueConst new_target,
                                   int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
    return JS_EXCEPTION;
  }

  // Create promise object
  JSValue promise = JS_NewObjectClass(ctx, 1000); // Custom class ID for Promise
  if (JS_IsException(promise)) {
    return promise;
  }

  // Create and store state
  PromiseState *state = new PromiseState(ctx);
  JS_SetOpaque(promise, state);

  // Set prototype
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue PromiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
  JSValue PromiseProto = JS_GetPropertyStr(ctx, PromiseCtor, "prototype");
  JS_SetPrototype(ctx, promise, PromiseProto);
  JS_FreeValue(ctx, PromiseProto);
  JS_FreeValue(ctx, PromiseCtor);
  JS_FreeValue(ctx, global);

  // Create resolve and reject functions
  JSValue resolve = JS_NewCFunctionData(
      ctx,
      [](JSContext *c, JSValueConst t, int a, JSValueConst *v, int,
         JSValueConst *d) -> JSValue {
        PromiseState *state = (PromiseState *)JS_GetOpaque(d[0]);
        if (state && state->state == PromiseState::State::Pending) {
          resolvePromise(c, state, a > 0 ? v[0] : JS_UNDEFINED);
        }
        return JS_UNDEFINED;
      },
      "resolve", 1, 1, &promise);

  JSValue reject = JS_NewCFunctionData(
      ctx,
      [](JSContext *c, JSValueConst t, int a, JSValueConst *v, int,
         JSValueConst *d) -> JSValue {
        PromiseState *state = (PromiseState *)JS_GetOpaque(d[0]);
        if (state && state->state == PromiseState::State::Pending) {
          rejectPromise(c, state, a > 0 ? v[0] : JS_UNDEFINED);
        }
        return JS_UNDEFINED;
      },
      "reject", 1, 1, &promise);

  // Call executor
  JSValue argv_exec[] = {resolve, reject};
  JSValue ret = JS_Call(ctx, argv[0], JS_UNDEFINED, 2, argv_exec);
  if (JS_IsException(ret)) {
    // Executor threw an exception, reject the promise
    JSValue exc = JS_GetException(ctx);
    if (state->state == PromiseState::State::Pending) {
      rejectPromise(ctx, state, exc);
    }
    JS_FreeValue(ctx, exc);
  }
  JS_FreeValue(ctx, ret);
  JS_FreeValue(ctx, resolve);
  JS_FreeValue(ctx, reject);

  return promise;
}

JSValue PromiseBinding::js_Promise_then(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
  PromiseState *state = (PromiseState *)JS_GetOpaque(this_val);
  if (!state) {
    return JS_EXCEPTION;
  }

  JSValue onFulfilled =
      argc > 0 && JS_IsFunction(ctx, argv[0]) ? JS_DupValue(ctx, argv[0])
                                               : JS_UNDEFINED;
  JSValue onRejected =
      argc > 1 && JS_IsFunction(ctx, argv[1]) ? JS_DupValue(ctx, argv[1])
                                               : JS_UNDEFINED;

  // Create new promise
  JSValue newPromise = JS_NewObjectClass(ctx, 1000);
  PromiseState *newState = new PromiseState(ctx);
  JS_SetOpaque(newPromise, newState);

  // Set prototype
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue PromiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
  JSValue PromiseProto = JS_GetPropertyStr(ctx, PromiseCtor, "prototype");
  JS_SetPrototype(ctx, newPromise, PromiseProto);
  JS_FreeValue(ctx, PromiseProto);
  JS_FreeValue(ctx, PromiseCtor);
  JS_FreeValue(ctx, global);

  // Create chained handlers
  if (state->state == PromiseState::State::Pending) {
    // Add handlers to pending list
    state->onFulfilled.push_back(onFulfilled);
    state->onRejected.push_back(onRejected);
  } else if (state->state == PromiseState::State::Fulfilled) {
    // Already fulfilled, queue microtask
    if (!JS_IsUndefined(onFulfilled)) {
      enqueueMicrotask(ctx, onFulfilled, JS_UNDEFINED, 1, &state->value);
    } else {
      // Pass-through fulfillment
      resolvePromise(ctx, newState, state->value);
    }
    JS_FreeValue(ctx, onFulfilled);
    JS_FreeValue(ctx, onRejected);
  } else {
    // Already rejected, queue microtask
    if (!JS_IsUndefined(onRejected)) {
      enqueueMicrotask(ctx, onRejected, JS_UNDEFINED, 1, &state->reason);
    } else {
      // Pass-through rejection
      rejectPromise(ctx, newState, state->reason);
    }
    JS_FreeValue(ctx, onFulfilled);
    JS_FreeValue(ctx, onRejected);
  }

  return newPromise;
}

JSValue PromiseBinding::js_Promise_catch(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
  // catch is just then with onFulfilled = undefined
  JSValue onRejected =
      argc > 0 && JS_IsFunction(ctx, argv[0]) ? JS_DupValue(ctx, argv[0])
                                               : JS_UNDEFINED;

  JSValue then_fn = JS_GetPropertyStr(ctx, this_val, "then");
  JSValue args[] = {JS_UNDEFINED, onRejected};
  JSValue result = JS_Call(ctx, then_fn, this_val, 2, args);
  JS_FreeValue(ctx, then_fn);
  JS_FreeValue(ctx, onRejected);
  return result;
}

JSValue PromiseBinding::js_Promise_finally(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
    return JS_NewCFunction(ctx, js_Promise_then, "then", 2);
  }

  JSValue onFinally = JS_DupValue(ctx, argv[0]);

  // Create then handler
  JSValue thenHandler = JS_NewCFunctionData(
      ctx,
      [](JSContext *c, JSValueConst t, int a, JSValueConst *v, int,
         JSValueConst *d) -> JSValue {
        JSValue fn = JS_DupValue(c, d[0]);
        JSValue val = JS_DupValue(c, v[0]);
        if (g_eventLoop) {
          g_eventLoop->queueMicroTask([c, fn, val]() {
            JS_Call(c, fn, JS_UNDEFINED, 0, nullptr);
            JS_FreeValue(c, fn);
          });
        } else {
          JS_Call(c, fn, JS_UNDEFINED, 0, nullptr);
          JS_FreeValue(c, fn);
        }
        return val;
      },
      "finallyThen", 1, 1, &onFinally);

  // Create catch handler
  JSValue catchHandler = JS_NewCFunctionData(
      ctx,
      [](JSContext *c, JSValueConst t, int a, JSValueConst *v, int,
         JSValueConst *d) -> JSValue {
        JSValue fn = JS_DupValue(c, d[0]);
        JSValue reason = JS_DupValue(c, v[0]);
        if (g_eventLoop) {
          g_eventLoop->queueMicroTask([c, fn, reason]() {
            JS_Call(c, fn, JS_UNDEFINED, 0, nullptr);
            JS_FreeValue(c, fn);
          });
        } else {
          JS_Call(c, fn, JS_UNDEFINED, 0, nullptr);
          JS_FreeValue(c, fn);
        }
        JS_Throw(c, reason);
        return JS_EXCEPTION;
      },
      "finallyCatch", 1, 1, &onFinally);

  // Call then
  JSValue then_fn = JS_GetPropertyStr(ctx, this_val, "then");
  JSValue args[] = {thenHandler, catchHandler};
  JSValue result = JS_Call(ctx, then_fn, this_val, 2, args);
  JS_FreeValue(ctx, then_fn);
  JS_FreeValue(ctx, thenHandler);
  JS_FreeValue(ctx, catchHandler);
  return result;
}

JSValue PromiseBinding::js_Promise_resolve(JSContext *ctx, JSValueConst this_val,
                                           int argc, JSValueConst *argv) {
  JSValue promise = JS_NewObjectClass(ctx, 1000);
  PromiseState *state = new PromiseState(ctx);
  JS_SetOpaque(promise, state);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue PromiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
  JSValue PromiseProto = JS_GetPropertyStr(ctx, PromiseCtor, "prototype");
  JS_SetPrototype(ctx, promise, PromiseProto);
  JS_FreeValue(ctx, PromiseProto);
  JS_FreeValue(ctx, PromiseCtor);
  JS_FreeValue(ctx, global);

  resolvePromise(ctx, state, argc > 0 ? argv[0] : JS_UNDEFINED);

  return promise;
}

JSValue PromiseBinding::js_Promise_reject(JSContext *ctx, JSValueConst this_val,
                                          int argc, JSValueConst *argv) {
  JSValue promise = JS_NewObjectClass(ctx, 1000);
  PromiseState *state = new PromiseState(ctx);
  JS_SetOpaque(promise, state);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue PromiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
  JSValue PromiseProto = JS_GetPropertyStr(ctx, PromiseCtor, "prototype");
  JS_SetPrototype(ctx, promise, PromiseProto);
  JS_FreeValue(ctx, PromiseProto);
  JS_FreeValue(ctx, PromiseCtor);
  JS_FreeValue(ctx, global);

  rejectPromise(ctx, state, argc > 0 ? argv[0] : JS_UNDEFINED);

  return promise;
}

JSValue PromiseBinding::js_Promise_all(JSContext *ctx, JSValueConst this_val,
                                        int argc, JSValueConst *argv) {
  if (argc < 1) {
    return JS_EXCEPTION;
  }

  // Simplified Promise.all implementation
  JSValue promise = JS_NewObjectClass(ctx, 1000);
  PromiseState *state = new PromiseState(ctx);
  JS_SetOpaque(promise, state);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue PromiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
  JSValue PromiseProto = JS_GetPropertyStr(ctx, PromiseCtor, "prototype");
  JS_SetPrototype(ctx, promise, PromiseProto);
  JS_FreeValue(ctx, PromiseProto);
  JS_FreeValue(ctx, PromiseCtor);
  JS_FreeValue(ctx, global);

  // For simplicity, just resolve with empty array
  JSValue arr = JS_NewArray(ctx);
  resolvePromise(ctx, state, arr);
  JS_FreeValue(ctx, arr);

  return promise;
}

JSValue PromiseBinding::js_Promise_race(JSContext *ctx, JSValueConst this_val,
                                         int argc, JSValueConst *argv) {
  // Simplified Promise.race
  return js_Promise_all(ctx, this_val, argc, argv);
}

void PromiseBinding::enqueueMicrotask(JSContext *ctx, JSValue callback,
                                      JSValue this_val, int argc,
                                      JSValueConst *argv) {
  JSValue callbackDup = JS_DupValue(ctx, callback);
  std::vector<JSValue> argsDup;
  for (int i = 0; i < argc; i++) {
    argsDup.push_back(JS_DupValue(ctx, argv[i]));
  }

  if (g_eventLoop) {
    g_eventLoop->queueMicroTask([ctx, callbackDup, argsDup, argc]() {
      std::vector<JSValueConst> argsConst(argsDup.begin(), argsDup.end());
      JSValue ret = JS_Call(ctx, callbackDup, JS_UNDEFINED, argc,
                            argc > 0 ? &argsConst[0] : nullptr);
      if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        if (str) {
          std::cout << "[Promise microtask] Exception: " << str << std::endl;
          JS_FreeCString(ctx, str);
        }
        JS_FreeValue(ctx, exc);
      }
      JS_FreeValue(ctx, ret);
      JS_FreeValue(ctx, callbackDup);
      for (auto &val : argsDup) {
        JS_FreeValue(ctx, val);
      }
    });
  } else {
    // Fallback
    JSValue ret = JS_Call(ctx, callbackDup, JS_UNDEFINED, argc, argv);
    if (JS_IsException(ret)) {
      JS_FreeValue(ctx, ret);
    }
    JS_FreeValue(ctx, callbackDup);
  }
}

void PromiseBinding::resolvePromise(JSContext *ctx, PromiseState *state,
                                    JSValue value) {
  if (state->state != PromiseState::State::Pending) {
    return;
  }

  state->state = PromiseState::State::Fulfilled;
  state->value = JS_DupValue(ctx, value);

  // Process all queued fulfill handlers
  for (auto &fn : state->onFulfilled) {
    if (!JS_IsUndefined(fn)) {
      enqueueMicrotask(ctx, fn, JS_UNDEFINED, 1, &value);
    }
  }
}

void PromiseBinding::rejectPromise(JSContext *ctx, PromiseState *state,
                                   JSValue reason) {
  if (state->state != PromiseState::State::Pending) {
    return;
  }

  state->state = PromiseState::State::Rejected;
  state->reason = JS_DupValue(ctx, reason);

  // Process all queued reject handlers
  for (auto &fn : state->onRejected) {
    if (!JS_IsUndefined(fn)) {
      enqueueMicrotask(ctx, fn, JS_UNDEFINED, 1, &reason);
    }
  }
}

} // namespace script
} // namespace xiaopeng
